#include <stdlib.h>
#include <Processor/Profile.h>

int main(void)
{
    int count = 0;
    char *buffer = NULL;

    CPU_PROFILE_CURRENT_TIME();
    for(count = 0; count < 1000000; count++)
    {
        CPU_PROFILE_COMP_START();
        buffer = malloc (1024 * sizeof(char));
        if(!buffer)
        {
            printf("Error: Memory Allocating\n");
            return 1;
        }
    
        memset(buffer, 0xFF, sizeof(char) * 1024);

        free (buffer);
        buffer = NULL;
        CPU_PROFILE_COMP_END();
    }
    CPU_PROFILE_CURRENT_TIME();
    CPU_PROFILE_FLUSH_DATA();
    return 0;
}


