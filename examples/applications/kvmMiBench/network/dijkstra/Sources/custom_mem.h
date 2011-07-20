
#ifndef CUSTOM_MEM_H
#define CUSTOM_MEM_H

//#define USE_CUSTOM_MEM

struct _QITEM
{
  int iNode;
  int iDist;
  int iPrev;
  struct _QITEM *qNext;
};
typedef struct _QITEM QITEM;

typedef struct mem_buffer
{
    int     is_free;
    QITEM   qitem;
} mem_buffer_t;

#define MEM_SIZE 512
typedef struct custom_memory
{
    int avail_count;
    int malloc_index;
    int peak_usage;
    mem_buffer_t buffer[MEM_SIZE];
} custom_memory_t;

void init_mem(custom_memory_t *pmem);
QITEM * alloc_mem(custom_memory_t *pmem);
void free_mem(custom_memory_t *pmem, QITEM * ptr);
void print_mem_state(custom_memory_t *pmem);

#endif /* CUSTOM_MEM_H */
