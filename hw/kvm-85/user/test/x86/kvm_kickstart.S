
.extern kvm_kickstart
.extern CPU_STACK_START

.bss

.section .init
    mov  CPU_STACK_START, %esp  /* Set the Stack to Appropriate Location; Refer to the ldscript */
    mov  $0x5000,%dx            /* Write the I/O Port to dx register, on which we
                                   will send the command (Erase Memory or Load Target Binary) */
    mov  $0x8,%eax              /* Write the Command ID to eax register */
    out  %eax,%dx               /* Actually Write the Command to I/O Port */
    call kvm_kickstart          /* Call the Start Function */
    out %eax, $0xf4

