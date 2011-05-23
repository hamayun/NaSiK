
SECTIONS
{
    . = 1M;
    stext = .;
    .text : { *(.init) *(.text) *(.text.*) }
    . = ALIGN(4K);
    .data : { *(.data) }
    . = ALIGN(16);
    .rodata : { *(.rodata) *(.rodata.str1.*) }
    . = ALIGN(16);
    .bss : { *(.bss) }
    . = ALIGN(4K);
    edata = .;
	. = 2M;
	.note : { *(.note) *(.note.gnu.build-id) *(.comment) }
}

