#include <stdio.h>
#include <stdlib.h>
#include <Processor/Profile.h>

#define NUM_SIZES 10

int main(void)
{
    int     block_sizes[NUM_SIZES] = {1, 2, 4, 8, 16, 64, 256, 512, 1024, 4096};
    int     index, bytesread, byteswritten;
    char   *buffer[NUM_SIZES] = {NULL};

    // Allocate Buffers.
    for(index = 0; index < NUM_SIZES; index++)
    {
        buffer[index] = malloc (block_sizes[index] * sizeof(char));
        if(!buffer[index])
        {
            printf("Error: Allocating Memory for Buffer Size %d\n", block_sizes[index]);
            return (-1);
        }
    }

    CPU_PROFILE_TIME_DELTA();
    for(index = 0; index < NUM_SIZES; index++)
    {
	    FILE *read_file = fopen("/devices/disk/simulator/0", "r");
	    if(read_file == NULL){
		    printf ("Error: File Open Error (For Reading)\r\n");
		    return(-2); 	
	    }

	    FILE *write_file = fopen("/devices/disk/simulator/2", "w");
	    if(write_file == NULL){
		    printf ("Error: File Open Error (For Writing)\r\n");
		    return(-2); 	
	    }

	    while (!feof(read_file))
	    {
		    bytesread = fread(buffer[index], sizeof(char), block_sizes[index], read_file);
            if(bytesread != block_sizes[index])
            {
		        printf ("Error: bytesread = %d, block_sizes[%d] = %d\r\n", bytesread, index, block_sizes[index]);
		        break;
            }

		    byteswritten = fwrite(buffer[index], sizeof(char), bytesread, write_file); 
            if(byteswritten != block_sizes[index])
            {
		        printf ("Error: byteswritten = %d, block_sizes[%d] = %d\r\n", byteswritten, index, block_sizes[index]);
		        break;
            }
	    }

        fclose(read_file);
        read_file = NULL;
        fclose(write_file);
        write_file = NULL;	

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
