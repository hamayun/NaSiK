#ifndef _NATIVE_CPU_ENDIAN_H_
#define _NATIVE_CPU_ENDIAN_H_

#include <platform.h>

#define SWITCH_ENDIAN_32(x)                                     \
	x = ((x & 0x000000ff) << 24) | ((x & 0x0000ff00) << 8)  |     \
((x & 0x00ff0000) >> 8)  | ((x & 0xff000000) >> 24)       

#define SWITCH_ENDIAN_16(x) x = ((x & 0x00ff) << 8) | ((x & 0xff00) >> 8)         

#define CPU_DATA_IS_BIGENDIAN(type,data) SWITCH_ENDIAN_##type(data)
#define CPU_DATA_IS_LITTLEENDIAN(type,data)

#if defined(PLATFORM_IS_BIG_ENDIAN)
#define CPU_PLATFORM_TO_CPU(type,value) SWITCH_ENDIAN_##type(data)
#define CPU_CPU_TO_PLATFORM(type,value) SWITCH_ENDIAN_##type(data)
#elif defined(PLATFORM_IS_LITTLE_ENDIAN)
#define CPU_PLATFORM_TO_CPU(type,value)
#define CPU_CPU_TO_PLATFORM(type,value)
#else
#error "ERROR : Platform endianness not set."
#endif

#define CPU_SHIFT_AND_CONCAT(size,result,lvalue,hvalue) { \
	result = (hvalue << size) | lvalue;                     \
}

#define CPU_SPLIT_AND_SHIFT(size,value,lvalue,hvalue) { \
	hvalue = value >> size;                               \
	lvalue = value - (hvalue << size);                    \
}

#endif

