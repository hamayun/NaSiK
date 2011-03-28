#ifndef PROCESSOR_SYNCHRONIZATION_H
#define PROCESSOR_SYNCHRONIZATION_H

extern long int __dnaos_hal_test_and_set(volatile long int * spinlock);
extern long int __dnaos_hal_compare_and_swap (volatile long int * p_val, long int oldval, long int newval);

/* static inline long int cpu_test_and_set (volatile long int * spinlock) { */
#define cpu_test_and_set(spinlock)  \
  __dnaos_hal_test_and_set(spinlock)

/* static inline long int cpu_compare_and_swap (volatile long int * p_val, long int oldval, long int newval) { */
#define cpu_compare_and_swap(p_val, oldval, newval) \
  __dnaos_hal_compare_and_swap(p_val, oldval, newval)

#endif

