#ifndef PROCESSOR_CONTEXT_H
#define PROCESSOR_CONTEXT_H

#include <stdint.h>
#include <stdlib.h>

//#define DEBUG_PROCESSOR_CONTEXT

typedef volatile uintptr_t cpu_context_t [32];
#define CPU_CONTEXT_SIZE sizeof(cpu_context_t)

void cpu_context_init (cpu_context_t * ctx, void * sp, int32_t ssize, void * entry, void * arg);
void cpu_context_save (cpu_context_t * ctx, uint32_t *entry);
void cpu_context_load (cpu_context_t * ctx);

extern volatile uint32_t current_thread_context;

#ifdef DEBUG_PROCESSOR_CONTEXT
void print_context(cpu_context_t *ctx);
void print_context_mem(cpu_context_t *ctx);
void print_stack(void * stack_base, int32_t stack_size, int32_t n);
#endif

/*
void cpu_context_save (cpu_context_t * ctx, uint32_t *entry);
*/
#define cpu_context_save(ctx,addr) \
    do{ \
        __asm__ volatile (\
            "push %1 \n"\
            "push %0 \n"\
            "push %%ebp \n"\
            "mov %%esp, %%ebp \n"\
            "\n"\
            "mov 0x4(%%ebp), %%esp \n" /* switch to ctx*/\
            "push (%%ebp) \n"        /* ebp at the beginning of this macro*/ \
            "push 0x8(%%ebp) \n"     /* address at which we should return*/ \
            "push %%ebp \n"          /* current position of the ebp*/ \
            "pushf \n"\
            "cli\n"\
            "pusha \n"\
            "mov 0x4(%%ebp), %%eax \n"\
            "mov %%esp, (%%eax) \n"   /* Update to first entry of context (the push/pop location)*/\
            "\n"\
            "leave\n"\
            "add $0x8, %%esp"\
        :: "r" ((unsigned long) (ctx)), "r" ((unsigned long) (addr)):"%eax");\
    } while (0)

/*
void cpu_context_load (cpu_context_t * ctx);
*/
#define cpu_context_load(ctx) \
    do{ \
        current_thread_context = (uint32_t) ctx;\
        __asm__ volatile (\
            "mov %0, %%esp \n"   /*switch to ctx*/\
            "\n"\
            "pop %%esp \n"  /* This moves the esp to the correct poping location */\
            "popa \n"\
            "popf \n"\
            "pop %%ebp \n"\
            "mov 0x4(%%esp), %%eax \n"  /* get the ebp_o of save_context into eax */\
            "mov %%eax, (%%ebp) \n"\
            "mov (%%esp), %%eax \n"     /* get the return address */\
            "mov %%eax, 0x04(%%ebp) \n"\
            "mov %%esp, %%eax\n"      /* Update the push/pop address (The first entry of context)*/\
            "add $0x08, %%eax\n"\
            "mov %%eax, %0\n"\
            "\n"\
            "leave \n"\
            "ret $0x4 \n"\
        ::"r" ((unsigned long) (ctx)):"%eax");\
    } while (0)

#endif

