/*
 * minimal bootstrap to set up flat 32-bit protected mode
 */

#define APIC_BASE 0x1000
//#define APIC_BASE 0xFEE00000
#define APIC_REG_NCPU        0x00
#define APIC_REG_ID          0x04
#define APIC_REG_SIPI_ADDR   0x08
#define APIC_REG_SEND_SIPI   0x0c
#define APIC_REG_IPI_VECTOR  0x10
#define APIC_REG_SEND_IPI    0x14

#.equ APIC_BASE, 0xFEE00000

/* bstart = 0xf0000 */
boot_sect_start = 0x7C00

.code16

/* Match stack_top in kvm.c (BOOT_LOADER_SP) */
stack_top = 0x8000
/* Dummy Value for cpu_up */
cpu_up = 0x1000
/* Dummy Value for cpu_up_pmode */
cpu_up_pmode = 0x1004

pmode_stack_start = 0x10000
pmode_stack_shift = 16
pmode_stack_size = (1 << pmode_stack_shift)

ipi_vec = 0xf0

.code16gcc
.section ".entrytext", "ax"
.global _start

_start:
        ljmp $0x07C0, $start

start:
    call smp_init

	cs lidtl idt_desc
	cs lgdtl gdt_desc

        /* Enable Protected Mode */
	mov %cr0, %eax
	or $1, %eax
	mov %eax, %cr0

        ljmpl $8, $pmode + boot_sect_start

smp_init:
	mov $ipi_vec, %eax
	mov $(APIC_BASE + APIC_REG_IPI_VECTOR), %dx
	out %eax, %dx
	movw $ap_switch_to_pmode, ipi_vec*4
	movw %cs, %ax
	mov %ax, ipi_vec*4+2
	mov $sipi, %eax
	mov $(APIC_BASE + APIC_REG_SIPI_ADDR), %dx
	outl %eax, %dx
	mov $(APIC_BASE + APIC_REG_NCPU), %dx
	inl %dx, %eax
	mov %eax, %ecx
	mov $1, %esi
smp_loop:
	cmp %esi, %ecx
	jbe smp_done
        mov $0xDEADABCD, %eax
	mov %esi, %eax
	mov $(APIC_BASE + APIC_REG_SEND_SIPI), %dx
	outl %eax, %dx
wait_for_cpu:
	cmp cpu_up, %esi
	jne wait_for_cpu
	mov %esi, %eax
	mov $(APIC_BASE + APIC_REG_SEND_IPI), %dx
	out %eax, %dx
wait_for_cpu_pmode:
	cmp cpu_up_pmode, %esi
	jne wait_for_cpu_pmode

	inc %esi
	jmp smp_loop
smp_done:
	ret

sipi:
	mov $(APIC_BASE + APIC_REG_ID), %dx
	inl %dx, %eax
	mov %eax, cpu_up
	shl $12, %eax
	addl $stack_top, %eax
	movl %eax, %esp
	sti
	nop
1:	hlt
	jmp 1b

ap_switch_to_pmode:
	cs lidtl idt_desc
	cs lgdtl gdt_desc
	mov %cr0, %eax
	or $1, %eax
	mov %eax, %cr0
	ljmpl $8, $ap_pmode + boot_sect_start

.code32
/* Application Processor (AP) Protected Mode */
ap_pmode:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	mov $(APIC_BASE + APIC_REG_ID), %dx
	in %dx, %eax
	mov %eax, cpu_up_pmode
	shl $pmode_stack_shift, %eax
	lea pmode_stack_start + pmode_stack_size(%eax), %esp
	sti
	nop
ap_pmode_wait:
	hlt
	jmp ap_pmode_wait

/* Boot Strap Processsor (BSP) Protected Mode; Starts Here */
pmode:
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	mov $pmode_stack_start + pmode_stack_size, %esp
	ljmp $8, $0x00100000

.align 16

idt_desc:
	.word 8*256-1
	.long 0

gdt_desc:
	.word gdt_end - gdt - 1
	.long gdt + boot_sect_start

.align 16

gdt:
	.quad 0
	.quad 0x00cf9b000000ffff // flat 32-bit code segment
	.quad 0x00cf93000000ffff // flat 32-bit data segment
gdt_end:

.align 65536

