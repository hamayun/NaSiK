#include <Processor/Processor.h>

void cpu_trap_disable (interrupt_id_t id)
{
  PLATFORM_AICU_BASE -> slot[cpu_mp_id ()] . mask &= ~(1 << id);
}

