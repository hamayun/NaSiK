#ifndef __SOC_KVM_H__
#define __SOC_KVM_H__

#include <stdint.h>

typedef struct _soc_kvm_data
{
	int				ncpus;
	uint64_t		memory_size;
	unsigned char*	vm_mem;
} soc_kvm_data;

//#define MMIO_TRACE_OPTION
#ifdef MMIO_TRACE_OPTION
typedef struct mmio_trace
{
    uint64_t addr;
    uint8_t  is_write;
} mmio_trace_t;

#define MMIO_TRACE_ENTRIES 10*1024*1024
#define MMIO_TRACE_BUFFER_SIZE (MMIO_TRACE_ENTRIES*sizeof(mmio_trace_t))
#endif

//#define PMIO_TRACE_OPTION
#ifdef PMIO_TRACE_OPTION
typedef struct pmio_trace
{
    uint32_t db_addr;
    uint64_t rip;
}pmio_trace_t;
#define PMIO_TRACE_ENTRIES 10*1024*1024
#define PMIO_TRACE_BUFFER_SIZE (PMIO_TRACE_ENTRIES*sizeof(pmio_trace_t))
#endif

#endif
