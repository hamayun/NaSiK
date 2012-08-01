.code16gcc
.section ".text16","ax"
.align 16

.globl secondary_boot
secondary_boot:
    # mov     $0xB800, %ax
    # mov     %ax, %es
    # movb    $'Z', %es:0

    mov     %cs, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss
    mov     $0xF000, %sp
    
    call    goto_pm_secondary

