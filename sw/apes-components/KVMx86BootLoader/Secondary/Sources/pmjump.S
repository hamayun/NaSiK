#include "boot.h"
#include "processor-flags.h"
#include "segment.h"

    .text
    .code16

/*
 * secondary_pm_jump (void);
 */

GLOBAL(secondary_pm_jump)
    xorl    %ebx, %ebx
    movw    %cs, %bx
    shll    $4, %ebx
    addl    $in_pm32, %ebx
    movl    %ebx,2f
    jmp     1f                  # Short jump to serialize on 386/486

1:
    movw    $__BOOT_DS, %cx
    movw    $__BOOT_TSS, %di

    movl    %cr0, %edx
    orb     $X86_CR0_PE, %dl    # Protected mode
    movl    %edx, %cr0

    # Transition to 32-bit mode
    .byte    0x66, 0xea         # ljmpl opcode
2:  .long    in_pm32            # offset
    .word    __BOOT_CS          # segment
ENDPROC(secondary_pm_jump)

    .code32
    .section ".text32","ax"
GLOBAL(in_pm32)
    # Set up data segments for flat 32-bit mode
    xorl    %ebx, %ebx
    movw    %ss, %bx
    shll    $4, %ebx

    movl    %ecx, %ds
    movl    %ecx, %es
    movl    %ecx, %fs
    movl    %ecx, %gs
    movl    %ecx, %ss
    # The 32-bit code sets up its own stack, but this way we do have
    # a valid stack if some debugging hack wants to use it.
    addl    %ebx, %esp

    # Clear registers to allow for future extensions to the
    # 32-bit boot protocol
    xorl    %ecx, %ecx
    xorl    %edx, %edx
    xorl    %ebx, %ebx
    xorl    %ebp, %ebp
    xorl    %edi, %edi

    # Set up LDTR to make Intel VT happy
    lldt    %cx

    mov     $0x100000, %eax
    jmpl    *%eax            # Jump to the 32-bit entrypoint
ENDPROC(in_pm32)

