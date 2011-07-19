
#include <stdio.h>
#include <string.h>
#include "custom_mem.h"

void init_mem(custom_memory_t *pmem)
{
    int index = 0;
    for (; index < MEM_SIZE; index++)
    {
        pmem->buffer[index].is_free = 1;
        memset(&pmem->buffer[index].qitem, 0x0, sizeof(QITEM));
    }

    pmem->avail_count  = MEM_SIZE;
    pmem->malloc_index = 0;
    pmem->peak_usage = 0;
    return;
}

QITEM * alloc_mem(custom_memory_t *pmem)
{
    QITEM * ptr = NULL;
    int     ref_index = pmem->malloc_index;
    
    while(pmem->avail_count)
    {
        if(pmem->buffer[pmem->malloc_index].is_free)
        {
            ptr = & pmem->buffer[pmem->malloc_index].qitem;
            pmem->buffer[pmem->malloc_index].is_free = 0;
            pmem->avail_count--;

            if((MEM_SIZE - pmem->avail_count) > pmem->peak_usage)
                pmem->peak_usage = (MEM_SIZE - pmem->avail_count);
        }

        pmem->malloc_index = (pmem->malloc_index + 1) % MEM_SIZE;
        if(ptr || ref_index == pmem->malloc_index)
            break;
    }
    
    if(ptr == NULL){
        printf("Error: Allocating Memory, Available Slots = %d, Ref Index = %d, Malloc Index = %d\n", 
                pmem->avail_count, ref_index, pmem->malloc_index);
        return NULL;
    }

    return ptr;    
}

void free_mem(custom_memory_t *pmem, QITEM * ptr)
{
    void *pqitem = (void *) ptr;
    int  *pfree_flag = (int *) (pqitem - sizeof(int));
    
    *pfree_flag = 1;
    pmem->avail_count++;
    ptr = NULL;
}

void print_mem_state(custom_memory_t *pmem)
{
    int index = 0; 

    for (; index < MEM_SIZE; index++)
    {
        printf("[%d]: ", index);
        if(pmem->buffer[index].is_free)
            printf("FREE");
        else
            printf("BUSY");
        if(pmem->malloc_index == index)
            printf(" <-- Malloc Index");
        printf("\n");
    }

    printf("Available Slots: %d, Peak Usage = %d\n", pmem->avail_count, pmem->peak_usage);
}

#if 0
int main()
{
    QITEM * ptr[20] = {NULL}; 
    custom_memory_t custom_mem;

    init_mem(& custom_mem);
    print_mem_state(& custom_mem);
    
    ptr[0] = alloc_mem(& custom_mem);
    ptr[1] = alloc_mem(& custom_mem);
    ptr[2] = alloc_mem(& custom_mem);
    ptr[3] = alloc_mem(& custom_mem);
    ptr[4] = alloc_mem(& custom_mem);

    print_mem_state(& custom_mem);

    free_mem (& custom_mem, ptr[1]);
    free_mem (& custom_mem, ptr[3]);

    print_mem_state(& custom_mem);

    ptr[5] = alloc_mem(& custom_mem);
    ptr[6] = alloc_mem(& custom_mem);
    ptr[7] = alloc_mem(& custom_mem);
    ptr[8] = alloc_mem(& custom_mem);
    ptr[9] = alloc_mem(& custom_mem);

    print_mem_state(& custom_mem);

    ptr[10] = alloc_mem(& custom_mem);
    ptr[11] = alloc_mem(& custom_mem);

    ptr[12] = alloc_mem(& custom_mem);

    print_mem_state(& custom_mem);
   
    return 0;
}
#endif
