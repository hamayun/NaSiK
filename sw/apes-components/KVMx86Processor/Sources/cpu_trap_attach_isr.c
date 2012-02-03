#include <stdint.h>
#include <Processor/Processor.h>

void cpu_trap_attach_isr (int32_t cpuid, interrupt_id_t id,
    uint32_t mode, interrupt_handler_t isr)
{
  PLATFORM_AICU_BASE -> handlers[id] = (uint32_t) isr;
}

