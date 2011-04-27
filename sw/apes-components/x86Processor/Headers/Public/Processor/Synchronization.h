#ifndef PROCESSOR_SYNCHRONIZATION_H
#define PROCESSOR_SYNCHRONIZATION_H

/* static inline long int cpu_test_and_set (volatile long int * spinlock) { */
static inline int cpu_test_and_set (volatile long int * spinlock)
{
	// It's not ok with smp. Use cmpxchg instead.
	int oldValue;
	oldValue = *spinlock;
	*spinlock = 1;
	return oldValue;
}

/* static inline long int cpu_compare_and_swap (volatile long int * p_val, long int oldval, long int newval) { */
static inline int32_t cpu_compare_and_swap (int32_t * p_val, int32_t oldval, int32_t newval)
{
	// It's not ok with smp. Use cmpxchg instead.
	long int old_reg_val = *p_val;
	if (old_reg_val == oldval)
		*p_val = newval;
//	return old_reg_val;
	return (!old_reg_val);		// MMH: New DNAOS expects the inverse value. 
}

#if 0 // Original Code; that doesn't work with Latest DNAOS
/* static inline long int cpu_test_and_set (volatile long int * spinlock) { */
static inline int cpu_test_and_set (volatile long int * spinlock)
{
	// It's not ok with smp. Use cmpxchg instead.
	int oldValue;
	oldValue = *spinlock;
	*spinlock = 1;
	return oldValue;
}

/* static inline long int cpu_compare_and_swap (volatile long int * p_val, long int oldval, long int newval) { */
static inline int32_t cpu_compare_and_swap (int32_t * p_val, int32_t oldval, int32_t newval)
{
	// It's not ok with smp. Use cmpxchg instead.
	long int old_reg_val = *p_val;
	if (old_reg_val == oldval)
		*p_val = newval;
	return old_reg_val;
}
#endif 

#endif

