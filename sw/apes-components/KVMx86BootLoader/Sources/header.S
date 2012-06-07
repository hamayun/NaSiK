/*
 *  Copied and modified from linux (arch/x86/boot/header.S)
 */

#include "boot.h"

BOOTSEG         = 0x07C0                # original address of boot-sector
SYSSEG          = 0x1000                # historical load address >> 4
LOADED_HIGH     = 1                     # If set, the kernel is loaded high
LOAD_PHYSICAL_ADDR = 0x100000

    .code16gcc
    .section ".entrytext", "ax"

    .global _start
_start:

    ljmp $BOOTSEG, $start1

start1:
    movw   %cs, %ax
    movw   %ax, %ds
    movw   %ax, %es
    movw   %ax, %ss
    xorw   %sp, %sp
    sti
    cld

    # make up a new stack
    movw    $_end, %dx
    addw    $STACK_SIZE, %dx
    jnc     2f
    xorw    %dx, %dx                        # Prevent wraparound

2:  
    # Now %dx should point to the end of our stack space
    andw    $~3, %dx                        # dword align (might as well...)
    jnz     3f
    movw    $0xfffc, %dx                    # Make sure we're not zero
    
3:
    movzwl  %dx, %esp                       # Clear upper half of %esp; Move Zero Extended Word to Long
    sti                                     # Now we should have a working stack

    # Zero the bss
    movw    $__bss_start, %di               # start of bss
    movw    $_end + 3, %cx
    xorl    %eax, %eax                      # Zero out eax
    subw    %di, %cx                        # number of bytes in bss
    shrw    $2, %cx                         # divide by 4
    rep     stosl                           # store string with contents of eax; Store T to edi from accT.
                                            # Repeat, the number of times is specified in ecx.

    movw    $kernel_version, %si
msg_loop:
    lodsb
    andb    %al, %al
    jz      after_msg
    movb    $0xe, %ah
    movw    $7, %bx
    int     $0x10
    jmp     msg_loop
after_msg:

    # Jump to C code (should not return)
    calll   main

   .globl   die
   .type    die, @function
die:
    hlt
    jmp     die
    .size   die, . - die

   .section ".initdata", "a"
kernel_version:
    .string "DNA-OS for KVM (x86)\n"

