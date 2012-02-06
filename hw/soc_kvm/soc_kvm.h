#ifndef __SOC_KVM_H__
#define __SOC_KVM_H__

#include <stdint.h>

#define MAX_VCPUS 4

typedef struct _soc_kvm_data
{
	int				ncpus;
	uint64_t		memory_size;
	unsigned char*	vm_mem;
} soc_kvm_data;

#endif
