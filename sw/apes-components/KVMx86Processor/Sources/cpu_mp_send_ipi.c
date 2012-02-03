#include <Processor/Processor.h>
#include <Platform/Platform.h>

void cpu_mp_send_ipi (int32_t target, int32_t command, void * data)
{
  cpu_write (UINT32, & (PLATFORM_IPI_BASE[target] . data), ((uint32_t)data));
  cpu_write (UINT32, & (PLATFORM_IPI_BASE[target] . command), command);
}

