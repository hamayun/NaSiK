#include <stdio.h>
#include <stdlib.h>
#include <Processor/Profile.h>

#define NUM_SIZES 10
#define PRINT_LIMIT 1024*1024

int main(void)
{
    int     string_sizes[NUM_SIZES] = {1, 2, 4, 8, 16, 64, 256, 512, 1024, 4096};
    int     index, print_count;
    char   *buffer[NUM_SIZES] = {NULL};

    // Allocate Buffers.
    for(index = 0; index < NUM_SIZES; index++)
    {
        int i;
        char next_byte = 'a';

        buffer[index] = malloc ((string_sizes[index] * sizeof(char)) + 1);
        if(!buffer[index])
        {
            printf("Error: Allocating Memory for Buffer Size %d\n", string_sizes[index]);
            return (-1);
        }

        // Fill Buffers With Strings.
        for (i = 0; i < string_sizes[index]; i++)
        {
            buffer[index][i] = next_byte++;
            if(next_byte > 'z')
                next_byte = 'a';
        }
        buffer[index][i] = '\0';
    }

    CPU_PROFILE_TIME_DELTA();
    for(index = 0; index < NUM_SIZES; index++)
    {
        int num_times = PRINT_LIMIT / string_sizes[index];
	    for (print_count = 0; print_count < num_times; print_count++)
	    {
            fputs(buffer[index], stdout);
            fflush(stdout);
	    }

        fputs("\n", stdout);
        CPU_PROFILE_TIME_DELTA();
    }

    // Free Buffers.
    for(index = 0; index < NUM_SIZES; index++)
    {
        free(buffer[index]);
        buffer[index] = NULL;
    }

    CPU_PROFILE_FLUSH_DATA();
    return 0;
}
