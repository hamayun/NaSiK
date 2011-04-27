#include <Processor/Context.h>

#define CPU_STACK_PTR_REG                 0
#define SET_RETURN_ADDRESS(task, address)\
                          (*(unsigned int *)task->reg[0] = address)
#define NUMBER_OF_REGISTERS               8


/* The stack parameter is assumed to be an (unsigned int) */
void cpu_context_init (cpu_context_t * ctx, void * sp, int32_t ssize, void * entry, void * arg)
{
	/* Setup the stack pointer */
	uintptr_t *stack_ptr = (uintptr_t *)(sp + ssize);

	/* push argument */
	*--stack_ptr = (uintptr_t)arg;
	
	/* push task entry function return pointer ???? */
	*--stack_ptr = (uintptr_t)0;
	
	/* push execution pointer */
	*--stack_ptr = (uintptr_t)entry;  /* EIP */

	/* push default flags ?? */
	*--stack_ptr = (uintptr_t)0x00000046;  /* EFLAGS */

	/* general purpose registers default values */
	*--stack_ptr = (uintptr_t)0;		/* EDI */
	*--stack_ptr = (uintptr_t)0;		/* ESI */
	*--stack_ptr = (uintptr_t)0;		/* EBP */
	*--stack_ptr = (uintptr_t)0;		/* ESP (ignored ??) */
	*--stack_ptr = (uintptr_t)0;		/* EBX */
	*--stack_ptr = (uintptr_t)0;		/* EDX */
	*--stack_ptr = (uintptr_t)0;		/* ECX */
	*--stack_ptr = (uintptr_t)entry;		/* EAX */

	/* push tls segment index */
	//*--stack_ptr = tls_seg << 3;		/* GS */

	ctx[0] = stack_ptr;
}

void cpu_context_save (cpu_context_t * ctx, uint32_t *entry)
{
	__asm__ volatile (
		/* save execution pointer */
		/* "	call	1f			\n" */
		/* "1:						\n" */
		/* EIP */
		/*"	push	??" */
		"	add		%%esp, 4	\n"
		"	push	%1			\n"
		/* save flags */
		"	pushf				\n"
		"	cli					\n"
		/* save general purpose registers on stack */
		/*"	movl	%1, %%eax	\n"*/
		"	pusha				\n"
		/* remove gs???*/
		/* switch stack pointer */
		"	movl	%%esp, %0	\n"	
		: "=m,m" (ctx[0])
		: "m,m" (entry)
	);
}

void cpu_context_load (cpu_context_t * ctx)
{
	__asm__ volatile (
		/* Load stack */
		"	movl	%0, %%esp	\n"
		/* restore tls ??*/
		/* pop gs ?? */
		/* restore general purpose registers*/
		"	popa				\n"
		/* restore flags */
		"	popf				\n"
		/* restore execution pointer */
		"	ret					\n"
		/*"	jmp %%eax			\n"*/
		: "=r,m" (ctx[0])
	);
}
