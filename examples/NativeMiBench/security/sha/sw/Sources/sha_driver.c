/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sha.h"

/*
 * Send command to the hosttime component to
 * Dump System Time at this particuler instant i.e. checkpoint.
 */
#include <Processor/IO.h>
extern unsigned int PLATFORM_HOSTTIME_BASE;
void DumpHostTime(int checkpoint)
{
    if(PLATFORM_HOSTTIME_BASE){
        cpu_write (UINT32, PLATFORM_HOSTTIME_BASE, checkpoint);
    }
    else
    {
        printf(stderr, "Invalid Address PLATFORM_HOSTTIME_BASE\n");
        exit(1); 
    }
}

int main(int argc, char **argv)
{
    FILE *fin;
    SHA_INFO sha_info;

    int myargc = 2;
    char **myargv = {"sha", "/devices/disk/simulator/0"};

	/* Dump Host Time to File
	*/
	int checkpoint = 1;

//    if (argc < 2) {
//	fin = stdin;
//	sha_stream(&sha_info, fin);
//	sha_print(&sha_info);
//    } else {
//	while (--argc) {
//	    fin = fopen(*(++argv), "rb");

		DumpHostTime(checkpoint++);

	    fin = fopen("/devices/disk/simulator/0", "rb");
	    if (fin == NULL) 
		{
			printf("error opening %s for reading\n", *myargv);
	    } 
		else 
		{
			sha_stream(&sha_info, fin);
			sha_print(&sha_info);

			DumpHostTime(checkpoint++);

			fclose(fin);
	    }
//	}
//    }
    return(0);
}
