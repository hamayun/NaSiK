/*
 * minimal smp bootstrap to set up flat 32-bit protected mode
 */

#define APIC_BASE 0xFEE00000

#define APIC_REG_NCPU        0x00
#define APIC_REG_ID          0x04
#define APIC_REG_SIPI_ADDR   0x08
#define APIC_REG_SEND_SIPI   0x0c
#define APIC_REG_IPI_VECTOR  0x10
#define APIC_REG_SEND_IPI    0x14

#define	APIC_ICR            0x300
#define	APIC_ICR_BUSY		0x01000

realCS = 0x1000
stack_top = 0x8000          /* Match stack_top in kvm.c (BOOT_LOADER_SP) */

.code16
.global _start

_start:
    ljmp $realCS, $start        # This does not change the Linear Address but Normalizes it;

start:
	mov	%cs, %ax		# address program arena
	mov	%ax, %ss		# using SS register
	mov	$stack_top, %sp		# establish local stack

    call    allow_4GB_addressing
    call    broadcast_AP_startup
    call	delay_until_APs_halt

//    call smp_init

    /* Enable Protected Mode */
/*	mov %cr0, %eax
	or $1, %eax
	mov %eax, %cr0 */

#    ljmpl $8, $pmode + realCS

#------------------------------------------------------------------
# We must briefly enter protected-mode to arrange for APIC access
#
theGDT:
    .quad 0x0000000000000000 // NULL Descriptor
    .quad 0x00CF9B000000FFFF // flat 32-bit code segment
    .quad 0x00CF93000000FFFF // flat 32-bit data segment
	.equ	limGDT, (.-theGDT)-1	# our GDT's segment-limit
#------------------------------------------------------------------
regGDT:	.word	limGDT, theGDT, 0x0001	# register-image for GDTR
#------------------------------------------------------------------
#------------------------------------------------------------------
allow_4GB_addressing:
#
# Here we setup register FS, so it can access the APIC registers.
#
	pushf				# preserve current flags
	cli				    # and disable interrupts

	mov	%cr0, %eax		# get CPU's register CR0
	bts	$0, %eax		# turn on PE-bit's image
	mov	%eax, %cr0		# enter 'protected-mode'

	lgdt %cs:regGDT		# setup GDT for this CPU

	mov	$0x10, %ax		# flat-segment's selector
	mov	%ax, %fs		# raises FS segment-limit

	mov	%cr0, %eax		# get CPU's register CR0
	btr	$0, %eax		# reset image for PE-bit
	mov	%eax, %cr0		# go back to 'real-mode'

	xor	%ax, %ax		# load base-address zero
	mov	%ax, %fs		# for our 'flat' segment

	popf				# restore previous flags
	ret
#------------------------------------------------------------------
n_APs:	.word	0               # count of awakened APs
n_fin:	.word	0               # count of finished APs
newSS:	.word	0x2000			# stack segment-address
#------------------------------------------------------------------
delay_EAX_microseconds:
#
# This helper-function will implement the timed delays which are
# specified in Intel's 'Multiprocessor Initialization Protocol',
# where the delay-duration (in microseconds) is in register EAX.
#
	pushal

	mov	%eax, %ecx              # copy microseconds count

	# enable the 8254 Channel-2 counter
	in	$0x61, %al              # get PORT_B settings
	and	$0x0D, %al              # turn PC speaker off
	or	$0x01, %al              # turn on Gate2 input
	out	%al, $0x61              # output new settings

	# program channel-2 for one-shot countdown
	mov	$0xB0, %al              # chan2,LSB/MSB,one-shot
	out	%al, $0x43              # output command to PIT

	# compute value for channel-2 latch-register
	mov	$1193182, %eax          # input-pulses-per-second
	mul	%ecx                    # * number of microseconds
	mov	$1000000, %ecx          # microseconds-per-second
	div	%ecx                    # division by doubleword

	# write latch-resister value to channel-2
	out	%al, $0x42
	mov	%ah, %al
	out	%al, $0x42

	# wait for channel-2 countdown to conclude
nxpoll:	
    in      $0x61, %al
	test	$0x20, %al
	jz	nxpoll

	# disable the 8254 Channel-2 counter
	in	$0x61, %al		# get PORT_B settings
	and	$0x0C, %al		# turn off channel-2
	out	%al, $0x61		# output new settings

	popal
	ret
#------------------------------------------------------------------
delay_EAX_us:
# A function trying to implement udelay ...

    pushal
	mov	%eax, %ecx              # copy microseconds count

delay_loop:
    mul %edx
    sub $1, %ecx
    jnz delay_loop

    popal
    ret
#------------------------------------------------------------------
delay_until_APs_halt:
#
# Here the BootStrap Processor waits for all the APs to finish.
#
	sti                 # allow timer interrupts
check:	hlt				# halt until interrupted
	mov	n_APs, %ax		# number of awakened APs
	sub	n_fin, %ax		# equals number finished?
	jnz	check			# no, check again later

    mov $0xAAAAAAAA, %eax
mysleep:
    cli                             # do not awaken this CPU
	hlt                             # 'fetch-execute' ceases
	jmp	mysleep                     # just-in-case of an NMI

	ret
#------------------------------------------------------------------
broadcast_AP_startup:
#
# This procedure is executed only by the BootStrap Processor, to
# awaken the Auxilliary Processors so that they each can display
# their Local-APIC ID-number (and their CR0 register's value, so
# we can verify that the cache-related bits are setup properly).
# We use code here which follows the MP Initialization Protocol.
#
	# point FS:EBX to the Local-APIC's memory-mapped page
	xor	%ax, %ax                    # address segment zero
	mov	%ax, %fs                    #   with FS register
	mov	$APIC_BASE, %ebx            # APIC address in EBX

	# compute the page-number (where each AP should start)
	mov	$realCS, %edx               # arena segment-address
	shl	$4, %edx                    # multiplied by sixteen
	add	$tos, %edx                  #  plus entry's offset
	shr	$12, %edx                   # divided by page-size
	and	$0xFF, %edx                 # must be in bottom 1MB

	# issue an 'INIT' Inter-Processor Interrupt command
	mov	$0x000C4500, %eax           # broadcast INIT-IPI
	mov	%eax, %fs:APIC_ICR(%ebx)       # to all-except-self; (MMH: OK in KVM-Module)

	# do ten-millisecond delay, enough time for APs to awaken
	#mov	$10000, %eax                # ten-thousand microseconds
	#call	delay_EAX_microseconds	# execute programmed delay
	mov	 $10000, %eax               # number of microseconds
	call delay_EAX_us               # for a programmed delay

	# wait for indication of the command's completion
spin1:
    bt	$12, %fs:APIC_ICR(%ebx)     # command-in-progress?
	jc	spin1                       # yes, spin until done

	#--------------------------------------------------------
	# now we complete the Intel 'MP Initialization Protocol'
	#--------------------------------------------------------
	mov	$2, %ecx                    # protocol's repetitions
	
nxIPI:
	# issue a 'Startup' Inter-Processor Interrupt command
	mov	$0x000C4600, %eax           # issue 'Startup-IPI'
	mov	%dl, %al                    # page is the vector
	mov	%eax, %fs:APIC_ICR(%ebx)    # to all-except-self

	# delay for 200 microseconds
	#mov	$200, %eax              # number of microseconds
	#call	delay_EAX_microseconds	# for a programmed delay
	mov	 $200, %eax                 # number of microseconds
	call delay_EAX_us               # for a programmed delay

	# wait for indication of the command's completion
spin2:	bt	$12, %fs:APIC_ICR(%ebx)	# command-in-progress?
	jc	spin2                       # yes, spin until done

	# repeat this 'Statup-IPI' step twice (per the protocol)
	loop	nxIPI                   # again for MP protocol

	ret

#------------------------------------------------------------------
initAP:
#
# This procedure will be executed by each Application Processor as
# it is awakened by the BootStrap Processor sending Startup-IPI's.
# In order that each processor can call subroutines, it requires a
# private stack-area, which we setup sequentially using the 'xadd'
# instruction (to guarantee that stack-areas are non-overlapping).
# But until its stack is ready, this CPU cannot handle interrupts.
#
	cli                             # disable interrupts

	mov	%cs, %ax                    # address program arena
	mov	%ax, %ds                    #   using DS register
	mov	%ax, %es                    #    and ES register

	# increment the count of processors that have awakened
	lock                            # insure 'atomic' update
	incw	n_APs                   # increment the AP count

	# setup an exclusive stack-region for this processor

	mov	$0x1000, %ax                # paragraphs in segment
	xadd %ax, newSS                 # 'atomic' xchg-and-add
	mov	%ax, %ss                    # segment-address to SS
	xor	%esp, %esp                  # top-of-stack into ESP

    call allow_4GB_addressing

	# increment the count of processors that have finished
	lock                            # insure 'atomic' update
	incw	n_fin                   # when modifying counter

	# now put this processor to sleep
sleep:	
    cli                             # do not awaken this CPU
	hlt                             # 'fetch-execute' ceases
	jmp	sleep                       # just-in-case of an NMI

#------------------------------------------------------------------
	.org	4096                    # insures page-alignment
tos:
    mov $0xBBBBBBBB, %eax
    ljmp $realCS, $initAP           # initialize awakened AP
#------------------------------------------------------------------
	.end				# nothing else to assemble


/*
	mov	$0xDEB00000, %ebx           # Debug MMIO Address
	mov	%eax, %fs:0x0(%ebx)
    mov $0xBBBBBBBB, %eax
*/
