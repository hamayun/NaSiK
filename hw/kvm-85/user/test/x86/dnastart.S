

.extern system_kickstart

.bss

.section .init
	call system_kickstart
	out %eax, $0xf4


