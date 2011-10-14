
.extern system_kickstart

.bss

.section .init
    mov  $0x5000,%dx        /* Write the I/O Port to dx register, on which we
                               will send the command (Erase Memory or Load Target Binary) */
    mov  $0x8,%eax          /* Write the Command ID to eax register */
    out  %eax,%dx           /* Actually Write the Command to I/O Port */
	call system_kickstart   /* Call the OS Start Function */
	out %eax, $0xf4

