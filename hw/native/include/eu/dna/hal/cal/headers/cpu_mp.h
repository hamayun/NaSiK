#ifndef _NATIVE_CPU_MP_H_
#define _NATIVE_CPU_MP_H_

#include <platform.h>

extern volatile unsigned long int cpu_mp_synchro;

extern unsigned int __dnaos_hal_get_proc_id(void);
#define CPU_MP_ID() __dnaos_hal_get_proc_id()
#define CPU_MP_COUNT PLATFORM_MP_CPU_COUNT(NATIVE)

#define CPU_MP_PROCEED() __dnaos_hal_write_uint32((uint32_t*)&cpu_mp_synchro,1)
#define CPU_MP_WAIT() while(__dnaos_hal_read_uint32((uint32_t*)&cpu_mp_synchro))

#endif


