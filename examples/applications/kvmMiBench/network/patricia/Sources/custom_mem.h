
#ifndef CUSTOM_MEM_H
#define CUSTOM_MEM_H

//#define USE_CUSTOM_MEM

typedef struct mem_buffer
{
    int     is_free;
    char    item[20];
} mem_buffer_t;

#define MEM_SIZE 1000000
typedef struct custom_memory
{
    int avail_count;
    int malloc_index;
    int peak_usage;
    mem_buffer_t buffer[MEM_SIZE];
} custom_memory_t;

void init_mem(custom_memory_t *pmem);
void *alloc_mem(custom_memory_t *pmem);
void free_mem(custom_memory_t *pmem, void * ptr);
void print_mem_state(custom_memory_t *pmem);

#endif /* CUSTOM_MEM_H */
