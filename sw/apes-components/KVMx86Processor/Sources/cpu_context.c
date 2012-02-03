#include <Processor/Context.h>
#include <DnaTools/C.h>
#include <string.h>

// This is used by AnnotationManager
volatile uint32_t current_thread_context = 0xFFFFFFFF;

void cpu_context_init(cpu_context_t * ctx, void * sp, int32_t ssize, void * entry, void * arg)
{
    uintptr_t *pnewstack, *pctx, *ebp_orig;
    uintptr_t ctx_save_start = 0;

    memset(ctx, 0, sizeof (cpu_context_t));
    ctx_save_start = ((uintptr_t) ctx + CPU_CONTEXT_SIZE - sizeof(uintptr_t)) & ~0xF;

    // The first entry gives us the current location of push/pop pointer on context.
    *((uintptr_t *) ctx) = ctx_save_start;

    // Get the 16-bit Aligned Address for our Stack Top.
    pnewstack = (uintptr_t *) (((uintptr_t) sp + ssize - sizeof(uintptr_t)) & ~0xF);
    ebp_orig = pnewstack;

    /* Get the context push/pop location; the first entry of context has it */
    pctx = (uintptr_t *) *((uintptr_t *) ctx);

    // "push (%%ebp) \n"                /* The ebp of calling thread's stack (ebp_orig) */
    *--pctx = (uintptr_t) ebp_orig;     /* We consider the start of new stack is our original ebp */

    // "push 0x8(%%ebp) \n"             /* The (entry) address at which we should return; when the context is loaded */
    *--pctx = (uintptr_t) entry;

    // "push %%ebp \n"                  /* Current position of the ebp_temp, its different from the thread stack's original ebp (ebp_orig) */
    *--pctx = (uintptr_t) 0;            /* We dont know this yet, will patch it later into the context, see below */

    /* Push default flags ?? */
    *--pctx = (uintptr_t) 0x00000046; /* EFLAGS */

    /* general purpose registers default values */
    *--pctx = (uintptr_t) 0; /* EDI */
    *--pctx = (uintptr_t) 0; /* ESI */
    *--pctx = (uintptr_t) 0; /* EBP */
    *--pctx = (uintptr_t) 0; /* ESP; Ignored here */
    *--pctx = (uintptr_t) 0; /* EBX */
    *--pctx = (uintptr_t) 0; /* EDX */
    *--pctx = (uintptr_t) 0; /* ECX */
    *--pctx = (uintptr_t) 0; /* EAX */

    // Write (not push) the Argument on the stack's ebp_orig
    *pnewstack = (uintptr_t) arg;

    // The Return Address in the New Stack; Obviousely it should be invalid.
    *--pnewstack = 0xDEADBEAF;

    // Now move the stack pointer down to make space for @entry, @ctx & ebp_orig;
    // These will be restored by cpu_context_load from the context.
    pnewstack -= 3;

    // Patch the New Stack's address in the context as the context needs to know it for our ebp_temp (!ebp_orig)
    *((uintptr_t *) ctx_save_start - 3) = (uintptr_t) pnewstack;

    // Update our push/pop pointer in the first entry of our context
    *((uintptr_t *) ctx) = (uintptr_t) pctx;

#ifdef DEBUG_PROCESSOR_CONTEXT
    print_context(ctx);
    print_context_mem(ctx);
    print_stack(sp, ssize, 10);
#endif

    return;
}

#ifdef DEBUG_PROCESSOR_CONTEXT
void print_context(cpu_context_t *ctx)
{
    uintptr_t ctx_save_start = ((uintptr_t) ctx + CPU_CONTEXT_SIZE - sizeof(uintptr_t)) & ~0xF;     // Same as in context_init
    uintptr_t *pctx = (uintptr_t *) ctx_save_start;
    int i;

    dna_printf("--------------------------------------------------------------\n");
    dna_printf("Context Address (ctx) = 0x%x, ctx_save_start = 0x%x\n", ctx, ctx_save_start);
    dna_printf("--------------------------------------------------------------\n");
    pctx--;
    dna_printf("[0x%x (ebp_orig)]: 0x%x\n", (uintptr_t) pctx, (uintptr_t) * pctx); pctx--;
    dna_printf("[0x%x (entry   )]: 0x%x\n", (uintptr_t) pctx, (uintptr_t) * pctx); pctx--;
    dna_printf("[0x%x (ebp_temp)]: 0x%x\n", (uintptr_t) pctx, (uintptr_t) * pctx); pctx--;
    dna_printf("[0x%x (flags   )]: 0x%x\n", (uintptr_t) pctx, (uintptr_t) * pctx); pctx--;
    for (i = 0; i < 8; i++) {
        dna_printf("[0x%x (reg_%d   )]: 0x%x\n", (uintptr_t) pctx, i, (uintptr_t) * pctx); pctx--;
    }
    dna_printf("[0x%x (esp_pop )]: 0x%x\n", (uintptr_t) ((uintptr_t *) ctx), *((uintptr_t *) ctx));   // Its the first entry of context
    dna_printf("--------------------------------------------------------------\n");
}

void print_stack(void * stack_base, int32_t stack_size, int32_t n)
{
    uintptr_t *pstack = (uintptr_t *) (stack_base + stack_size);
    int i;
    dna_printf("--------------------------------------------------------------\n");
    dna_printf("Thread Stack (Base = 0x%x, Size = 0x%x, N = %d)\n", stack_base, stack_size, n);
    dna_printf("--------------------------------------------------------------\n");

    for (i = 0; i < n; i++) {
        pstack--;
        dna_printf("[0x%x]: 0x%x\n", (uintptr_t) pstack, (uintptr_t) *pstack);
    }
    dna_printf("--------------------------------------------------------------\n");
}

void print_context_mem(cpu_context_t *ctx)
{
    uintptr_t *pctx = (uintptr_t *) ctx;
    uintptr_t *pctx_end = (uintptr_t *)((uint8_t *) ctx + CPU_CONTEXT_SIZE );

    dna_printf("--------------------------------------------------------------\n");
    dna_printf("CONTEXT MEMORY CONTENTS\n");
    dna_printf("--------------------------------------------------------------\n");

    while (pctx < pctx_end) {
        dna_printf("[0x%x]: 0x%x\n", (uintptr_t) pctx, (uintptr_t) * pctx);
        pctx++;
    }
    dna_printf("--------------------------------------------------------------\n");
}
#endif
