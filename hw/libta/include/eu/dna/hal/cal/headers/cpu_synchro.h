#ifndef CPU_SYNCHRO_H
#define CPU_SYNCHRO_H

extern long int __dnaos_hal_test_and_set(volatile long int * spinlock);
extern long int __dnaos_hal_compare_and_swap (volatile long int * p_val, long int oldval, long int newval);

#define CPU_TEST_AND_SET(spinlock)                __dnaos_hal_test_and_set((volatile long int*)spinlock)
#define CPU_COMPARE_AND_SWAP(p_val,oldval,newval) __dnaos_hal_compare_and_swap((volatile long int*)p_val,oldval,newval)

#endif

