/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sha.h"

int main(int argc, char **argv)
{
    FILE *fin;
    SHA_INFO sha_info;

    int myargc = 2;
    char **myargv = {"sha", "/devices/disk/simulator/0"};

    /* Dump Host Time to File
    */
    int checkpoint = 0;
    volatile int *htime;
    htime = (int *)0xCE000000;  

//    if (argc < 2) {
//	fin = stdin;
//	sha_stream(&sha_info, fin);
//	sha_print(&sha_info);
//    } else {
//	while (--argc) {
//	    fin = fopen(*(++argv), "rb");

       *htime = checkpoint++;

	    fin = fopen("/devices/disk/simulator/0", "rb");
	    if (fin == NULL) 
		{
			printf("error opening %s for reading\n", *myargv);
	    } 
		else 
		{
			sha_stream(&sha_info, fin);
			sha_print(&sha_info);

            *htime = checkpoint++;

			fclose(fin);
	    }
//	}
//    }
    return(0);
}
