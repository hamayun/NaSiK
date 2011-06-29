/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sha.h"
#include <Processor/Profile.h>

int main(int argc, char **argv)
{
    FILE *fin;
    SHA_INFO sha_info;

    int myargc = 2;
    char **myargv = {"sha", "/devices/disk/simulator/0"};

    fin = fopen("/devices/disk/simulator/0", "rb");
    if (fin == NULL)
    {
        printf("error opening %s for reading\n", *myargv);
        return(-1);
    }
    else
    {
        sha_stream(&sha_info, fin);

        CPU_PROFILE_IO_START();
        sha_print(&sha_info);
        CPU_PROFILE_IO_END();
    }

    fclose(fin);
    CPU_PROFILE_FLUSH_DATA();
    return(0);
}
