#include "stdint.h"

uint32_t OS_N_DRIVERS __attribute__  ((section (".data")));
uint32_t OS_DRIVERS_LIST[32] __attribute__  ((section (".data")));

uint32_t OS_N_FILESYSTEMS __attribute__  ((section (".data")));
uint32_t OS_FILESYSTEMS_LIST[32] __attribute__  ((section (".data")));

uint32_t OS_THREAD_STACK_SIZE __attribute__  ((section (".data")));

uint32_t OS_KERNEL_HEAP_ADDRESS __attribute__  ((section (".data")));
uint32_t OS_KERNEL_HEAP_SIZE __attribute__  ((section (".data")));

uint32_t OS_USER_HEAP_ADDRESS __attribute__  ((section (".data")));

uint32_t PLATFORM_N_NATIVE __attribute__  ((section (".data")));
uint32_t APP_ENTRY_POINT __attribute__  ((section (".data")));

uint32_t CPU_OS_ENTRY_POINT __attribute__  ((section (".data")));
uint32_t CPU_SVC_STACK_ADDR __attribute__  ((section (".data")));

uint32_t CPU_BSS_START __attribute__  ((section (".data")));
uint32_t CPU_BSS_END __attribute__  ((section (".data")));

uint32_t SOCLIB_TIMER_NDEV __attribute__  ((section (".data")));
uint32_t SOCLIB_TIMER_DEVICES[32] __attribute__  ((section (".data")));

uint32_t SOCLIB_TTY_NDEV __attribute__  ((section (".data")));
uint32_t SOCLIB_TTY_DEVICES[32] __attribute__  ((section (".data")));

uint32_t SOCLIB_FDACCESS_NDEV __attribute__  ((section (".data")));
uint32_t SOCLIB_FDACCESS_DEVICES[32] __attribute__  ((section (".data")));

uint32_t CHANNEL_RDV_NDEV __attribute__  ((section (".data")));

uint32_t SOCLIB_FB_BASE __attribute__ ((section(".data"))); 
