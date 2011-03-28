#ifndef PROCESSOR_CONTEXT_H
#define PROCESSOR_CONTEXT_H

#include <stdint.h>

typedef uint32_t * cpu_context_t;
#define CPU_CONTEXT_SIZE __dnaos_hal_context_size()

extern void __dnaos_hal_context_init(cpu_context_t *, void *, int32_t, void *, void *);
extern void __dnaos_hal_context_save (cpu_context_t *, uint32_t *);
extern void __dnaos_hal_context_load (cpu_context_t *);

#define cpu_context_init(ctx,sp,ssize,entry,arg) __dnaos_hal_context_init(ctx, sp, ssize, entry, arg)
#define cpu_context_save(from,label) __dnaos_hal_context_save(from, label) 
#define cpu_context_load(to) __dnaos_hal_context_load(to)

#endif

