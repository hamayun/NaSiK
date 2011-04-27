
MEMORY
{
	text :    ORIGIN = 1M,    LENGTH = 30M
}

PHDRS
{
	reset PT_LOAD FILEHDR PHDRS ;
	excep PT_LOAD ;
	text PT_LOAD ;
	data PT_LOAD ;
	heap PT_NULL ;
	stack PT_NULL ;
	debug PT_NULL ;
	bss PT_LOAD ;
}


SECTIONS
{
	.init 0x00100000: { stext = .; *(.init) } > text :text
    .text ALIGN(8): { *(.text) *(.text.*) } > text :text
	.fini ALIGN(8): { *(.fini*) } > text :text
    
    .data ALIGN(4K) : { *(.data) *(.glue_7*) *(.eh_frame*) } > text :data
	.os_config ALIGN(0x8): {
		OS_N_DRIVERS = .; LONG(0x5)
		OS_DRIVERS_LIST = .; LONG(soclib_platform_module) LONG(rdv_module)
                     LONG(fdaccess_module) LONG(soclib_fb_module) LONG(tg_module)
	
		OS_N_FILESYSTEMS = .; LONG(0x2)
		OS_FILESYSTEMS_LIST = .; LONG(devfs_module) LONG(rootfs_module)

		OS_N_MODULES = .; LONG(0x0)
		OS_MODULES_LIST = .;

		OS_THREAD_STACK_SIZE = .; LONG(0x8000)

		OS_KERNEL_HEAP_ADDRESS = .; LONG(ADDR(.kheap))
		OS_KERNEL_HEAP_SIZE = .; LONG(0x100000)

		OS_USER_HEAP_ADDRESS = .; LONG(ADDR(.uheap))
		OS_EXTENSIONS_LIST = .; LONG(0x0)
		OS_N_EXTENSIONS = .; LONG(0x0)
	} > text :data
	.hal ALIGN(0x8): {
		PLATFORM_DEBUG_CHARPORT = .; LONG(0xC0100000)
		APP_ENTRY_POINT = .; LONG(main);

		CPU_MIPS32_COUNT = .; LONG(0x2)
		CPU_SVC_STACK_ADDR  = ABSOLUTE(ADDR(.sysstack));

		CPU_BSS_START = .; LONG(ADDR(.bss))
		CPU_BSS_END = .; LONG(__hal_bss_end)

		PLATFORM_TIMER_INTN = .; LONG (2)
		PLATFORM_TIMER_BASE = .; LONG(0xC1000000)

		PLATFORM_IPI_INTN = .; LONG(7)
		PLATFORM_IPI_BASE = .; LONG(0xC2000000)

		SOCLIB_TTY_NDEV = .; LONG(0x1)
		SOCLIB_TTY_DEVICES = .; LONG(3) LONG(0XC0000000)

		SOCLIB_FB_NDEV = .; LONG(0x1)
		SOCLIB_FB_DEVICES = .; LONG(256) LONG(144) LONG(0XC4000000)

		SOCLIB_FDACCESS_NDEV = .; LONG(0x1)
		SOCLIB_FDACCESS_DEVICES = .; LONG(2) LONG(5) LONG(0xC3000000)

		CHANNEL_RDV_NDEV = .; LONG(24);

		PLATFORM_AICU_BASE = .; LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0)
		EXTFIFO_CHANNEL_NDEV = .; LONG(0x0)
		EXTFIFO_CHANNELS_PTR = .; LONG(0x0)
	} > text :data

    .rodata ALIGN(16) : { *(.rodata) *(.rodata.str1.*) } > text :data

	.sysstack ALIGN(0x8) + 0x10000 : { } > text :stack

    .bss ALIGN(16): {
		*(.bss)  *(.rel*) *(COMMON) __hal_bss_end = .; 
	} > text :bss

	.kheap ALIGN(0x8) : {} > text :heap

	.uheap ALIGN(0x8) + 0x100000: {_end = .;}  > text :heap


    edata = .;

	. = 10M;
	.note : { *(.note) *(.note.gnu.build-id) *(.comment) }
}

