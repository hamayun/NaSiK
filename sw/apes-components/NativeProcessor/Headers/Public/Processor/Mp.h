#ifndef PROCESSOR_MP_H
#define PROCESSOR_MP_H

#include <stdint.h>
#include <Platform/Platform.h>

/* extern volatile int32_t cpu_mp_synchro; */
extern volatile unsigned long int cpu_mp_synchro;

extern unsigned int __dnaos_hal_get_proc_id(void);

extern uint32_t PLATFORM_N_NATIVE; 
#define PLATFORM_MP_CPU_COUNT(what) PLATFORM_N_##what

#define CPU_IPI_ALL   0xFFFFFFFF 

/* extern int32_t cpu_mp_id (void) */
#define cpu_mp_id() \
  __dnaos_hal_get_proc_id()

/* extern int32_t cpu_mp_count (void); */
#define cpu_mp_count() \
  PLATFORM_MP_CPU_COUNT(NATIVE)

/* extern void cpu_mp_wait (void); */
#define cpu_mp_wait() \
 while(!__dnaos_hal_read_uint32((uint32_t*)&cpu_mp_synchro))

/* extern void cpu_mp_proceed (void); */
#define cpu_mp_proceed() \
  __dnaos_hal_write_uint32((uint32_t*)&cpu_mp_synchro,1)

/* extern void cpu_mp_send_ipi (int32_t id, int32_t command, void * arguments); */
#define cpu_mp_send_ipi(id, command, arguments) \
{}

#endif

