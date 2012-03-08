
#include <Processor/IO.h>

extern uint32_t SOCLIB_TTY_DEVICES[];

void platform_console_puts (char * string)
{
    char * p = string;

    while (*p != '\0')
    {
        cpu_write (UINT8, SOCLIB_TTY_DEVICES[1], *p++);
    }
}

void platform_console_puts_len (char * string, int len)
{
    char * p = string;

    for(int index = 0; index < len; index++)
    {
        cpu_write (UINT8, SOCLIB_TTY_DEVICES[1], p[index]);
    }
}
