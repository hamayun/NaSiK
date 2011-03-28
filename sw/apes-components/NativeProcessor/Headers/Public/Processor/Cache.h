#ifndef PROCESSOR_CACHE_H
#define PROCESSOR_CACHE_H

#include <stdint.h>

typedef enum cpu_cache
{
  CPU_CACHE_INSTRUCTION,
  CPU_CACHE_DATA
}
cpu_cache_t;

#define CPU_ICACHE_SIZE_LOG2 3
#define CPU_DCACHE_SIZE_LOG2 3

#define CPU_CACHE_ALL 0xFFFFFFFF

#define cpu_cache_sync()      \
{                             \
}

#define cpu_cache_invalidate(cache_type, address, words)	\
{															\
}

#endif

