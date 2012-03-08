
#include <Processor/IO.h>

extern char * PLATFORM_DEBUG_CHARPORT;

void platform_debug_puts (char * string)
{
    char * p = string;

    while (*p != '\0')
    {
        cpu_write (UINT8, PLATFORM_DEBUG_CHARPORT, *p++);
    }
}

void platform_debug_puts_len (char * string, int len)
{
    char * p = string;

    for(int index = 0; index < len; index++)
    {
        cpu_write (UINT8, PLATFORM_DEBUG_CHARPORT, p[index]);
    }
}
