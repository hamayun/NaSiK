

.extern system_kickstart

.bss

.section .init
	call system_kickstart
	push %eax
	call exit


