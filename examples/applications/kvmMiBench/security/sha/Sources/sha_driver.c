/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sha.h"
#include <Processor/Profile.h>

#ifdef  MEASURE_QEMU_ACCURACY
    /* Copied the following definitions from qemu_wrapper_cts.h */
    #define QEMU_ADDR_BASE                              0x82000000
    #define LOG_DELTA_STATS                             0x0058
#endif

int main(int argc, char *argv[])
{
#ifdef MEASURE_QEMU_ACCURACY
    volatile int *QEMU_LOG_ADDR = QEMU_ADDR_BASE + LOG_DELTA_STATS;
    *QEMU_LOG_ADDR = 1;
    real_main(argc, argv, 0);
    *QEMU_LOG_ADDR = 0;     /* Writing Zero to this Address will cause QEMU to exit */
#elif DISABLE_APP_REPEAT
    real_main(argc, argv, 0);
#else
    int app_repeat_count;
    for (app_repeat_count = 0; app_repeat_count < 20; app_repeat_count++)
        real_main(argc, argv, app_repeat_count);

    CPU_PROFILE_FLUSH_DATA();
#endif
    return 0;
}

int real_main(int argc, char **argv, int app_repeat_count)
{
    FILE *fin;
    SHA_INFO sha_info;

    int myargc = 2;
    char **myargv = {"sha", "/devices/disk/simulator/0"};

    printf("SHA: In main function : For %d time\n\n", app_repeat_count);
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

    printf("\nDone\n");
    return(0);
}
