
MEMORY
{
	mem :    ORIGIN = 0M,    LENGTH = 256M
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
	gdtdesc = 0x1000;
	gdt = 0x1000;
	idtdesc = 0x1100;
	idt = 0x1110;

	.init 0x0100000: { stext = .; *(.reset) } > mem :text
	.text ALIGN(8): { *(.text) *(.text.*) } > mem :text
	.fini ALIGN(8): { *(.fini*) } > mem :text
    
	.data ALIGN(0x8) : { *(.data*) *(.glue_7*) *(.eh_frame*) ../../../sw/apes-components/KVMx86BootLoader/Secondary/bin/KVMx86SecondaryBoot.o } > mem :data

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
		OS_KERNEL_HEAP_SIZE = .; LONG(0x1000000)

		OS_USER_HEAP_ADDRESS = .; LONG(ADDR(.uheap))
		OS_EXTENSIONS_LIST = .; LONG(0x0)
		OS_N_EXTENSIONS = .; LONG(0x0)
	} > mem :data
	.hal ALIGN(0x8): {
		PLATFORM_DEBUG_CHARPORT = .; LONG(0xC0000000)
		APP_ENTRY_POINT = .; LONG(_main);

		CPU_SYS_STACK_ADDR  = .; LONG(ADDR(.sysstack));

		CPU_BSS_START = .; LONG(ADDR(.bss))
		CPU_BSS_END = .; LONG(__hal_bss_end)

		i8259_VECTOR_OFFSET = .; LONG(0x20)
		IO_APIC_VECTOR_OFFSET = .; LONG(0x80)

		PLATFORM_TIMER_INTN = .; LONG (2)
		PLATFORM_TIMER_BASE = .; LONG(0xC1000000)

		PLATFORM_IPI_INTN = .; LONG(7)
		PLATFORM_IPI_BASE = .; LONG(0xC2000000)

		SOCLIB_TTY_NDEV = .; LONG(0x1)
		SOCLIB_TTY_DEVICES = .; LONG(0x25) LONG(0XC0000000) /* i8259_VECTOR_OFFSET + 5 is the IRQ Number */

		SOCLIB_FB_NDEV = .; LONG(0x1)
        /* Important: If in SystemC Framebuffer Component, the base address is 0XC4000000; we have to add an offset of 0x1000 here */
		SOCLIB_FB_DEVICES = .; LONG(256) LONG(144) LONG(0XC4001000)

		SOCLIB_FDACCESS_NDEV = .; LONG(0x1)
		SOCLIB_FDACCESS_DEVICES = .; LONG(2) LONG(5) LONG(0xC3000000)

		CHANNEL_RDV_NDEV = .; LONG(24);

		PLATFORM_AICU_BASE = .; LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0) LONG(0x0)
		EXTFIFO_CHANNEL_NDEV = .; LONG(0x0)
		EXTFIFO_CHANNELS_PTR = .; LONG(0x0)

		HOSTTIME_BASEPORT    = .; SHORT(0x5000)
	} > mem :data

    .rodata ALIGN(16) : { *(.rodata) *(.rodata.str1.*) *(.rodata.cst*) *(.rodata.str2.*) } > mem :data

	.sysstack ALIGN(0x8) + 0x10000 : { } > mem :stack

    .bss ALIGN(16): {
		*(.bss)  *(.rel*) *(COMMON) __hal_bss_end = .; 
	} > mem :bss

	.kheap ALIGN(0x8) : {} > mem :heap

	.uheap ALIGN(0x8) + 0x1000000: {_end = .;}  > mem :heap

	edata = .;

	.note : { *(.note) *(.note.gnu.build-id) *(.comment) }
}

