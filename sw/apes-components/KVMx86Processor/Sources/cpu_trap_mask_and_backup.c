#include <Processor/Processor.h>

interrupt_status_t cpu_trap_mask_and_backup (void)
{
  interrupt_status_t cpsr = 0;

  __asm__ volatile (
	"cli"
      );

  return cpsr;
}

