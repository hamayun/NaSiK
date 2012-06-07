
.extern cpu_init_pmode_idt
.extern system_kickstart

.bss

.section .init
    //mov  $0x5000,%dx        /* Write the I/O Port to dx register, on which we
    //                           will send the command (Erase Memory or Load Target Binary) */
    //mov  $0x8,%eax          /* Write the Command ID to eax register */
    //out  %eax,%dx           /* Actually Write the Command to I/O Port */
    /* Observation; This seems to be a bad place for loading target vliw binary;
       because some of the code in 0x00000010 - 0x0001FFFC gets overwritten during
       the DNAOS boot process; Thats why the above code has been commented; A better 
       place to load target vliw binary is in the application main function. */

    //call cpu_init_pmode_idt  /* Install proper ISR handlers (For Exceptions) */

    //int $0x3                 /* Now Sync With the GDB Server */

	call system_kickstart   /* Call the OS Start Function */
	out %eax, $0xf4

