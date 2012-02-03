#include <stdint.h>
#include <Processor/Processor.h>

void cpu_vector_transfer (void * source, 
	void * destination, uint32_t count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		*((volatile uint8_t *)destination++) = *((uint8_t *)source++);
	}
}
