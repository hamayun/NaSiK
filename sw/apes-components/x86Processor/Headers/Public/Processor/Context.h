#ifndef PROCESSOR_CONTEXT_H
#define PROCESSOR_CONTEXT_H

#include <stdint.h>

typedef uint32_t * cpu_context_t;



#define CPU_CONTEXT_SIZE 0x8

extern void cpu_context_init (cpu_context_t * ctx, void * sp, int32_t ssize, void * entry, void * arg);
extern void cpu_context_save (cpu_context_t *, uint32_t *);
extern void cpu_context_load (cpu_context_t *);

#endif

