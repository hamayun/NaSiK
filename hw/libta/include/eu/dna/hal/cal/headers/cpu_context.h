#ifndef _NATIVE_CPU_CONTEXT_H_
#define _NATIVE_CPU_CONTEXT_H_

typedef uintptr_t context_t;
#define CPU_CONTEXT_T context_t  

extern void __dnaos_hal_context_init(CPU_CONTEXT_T *ctx, void *sp, int32_t ssize, void *entry, void *arg);
extern void __dnaos_hal_context_save(CPU_CONTEXT_T *from, uintptr_t *label);
extern void __dnaos_hal_context_load(CPU_CONTEXT_T *to);
extern void __dnaos_hal_context_commute(CPU_CONTEXT_T *from, CPU_CONTEXT_T *to);

#define CPU_CONTEXT_INIT(ctx,sp,ssize,entry,arg) __dnaos_hal_context_init(ctx, sp, ssize, entry, arg)
#define CPU_CONTEXT_SAVE(from,label) __dnaos_hal_context_save(from, label)
#define CPU_CONTEXT_LOAD(to) __dnaos_hal_context_load(to)
#define CPU_CONTEXT_SWITCH(from,to) __dnaos_hal_context_commute(from,to)

#endif


