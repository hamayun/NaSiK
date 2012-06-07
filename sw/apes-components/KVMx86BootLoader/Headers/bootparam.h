#ifndef _BOOTPARAM_H_
#define _BOOTPARAM_H_

#include "basetypes.h"

struct setup_header
{
	__u8	setup_sects;
	__u16	root_flags;
	__u32	syssize;
	__u16	ram_size;
#define RAMDISK_IMAGE_START_MASK	0x07FF
#define RAMDISK_PROMPT_FLAG		0x8000
#define RAMDISK_LOAD_FLAG		0x4000
	__u16	vid_mode;
	__u16	root_dev;
	__u16	boot_flag;
	__u16	jump;
	__u32	header;
	__u16	version;
	__u32	realmode_swtch;
	__u16	start_sys;
	__u16	kernel_version;
	__u8	type_of_loader;
	__u8	loadflags;
#define LOADED_HIGH	(1<<0)
#define QUIET_FLAG	(1<<5)
#define KEEP_SEGMENTS	(1<<6)
#ifndef CAN_USE_HEAP
#define CAN_USE_HEAP	(1<<7)
#endif
	__u16	setup_move_size;
	__u32	code32_start;
	__u32	ramdisk_image;
	__u32	ramdisk_size;
	__u32	bootsect_kludge;
	__u16	heap_end_ptr;
	__u8	ext_loader_ver;
	__u8	ext_loader_type;
	__u32	cmd_line_ptr;
	__u32	initrd_addr_max;
	__u32	kernel_alignment;
	__u8	relocatable_kernel;
	__u8	_pad2[3];
	__u32	cmdline_size;
	__u32	hardware_subarch;
	__u64	hardware_subarch_data;
	__u32	payload_offset;
	__u32	payload_length;
	__u64	setup_data;
} __attribute__((packed));

#endif

