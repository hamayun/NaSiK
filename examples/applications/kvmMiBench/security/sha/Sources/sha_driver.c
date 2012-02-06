/* NIST Secure Hash Algorithm */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sha.h"
#include <Processor/Profile.h>

#ifdef  MEASURE_ACCURACY
    /* Copied the following definitions from qemu_wrapper_cts.h & kvm_cpu_wrapper.h */
    #define QEMU_ADDR_BASE                              0x82000000
    #define KVM_ADDR_BASE                               0xE0000000
    #define LOG_DELTA_STATS                             0x0058
#ifdef PLATFORM_QEMU
    #define ADDR_BASE                                   QEMU_ADDR_BASE
#else
    #define ADDR_BASE                                   KVM_ADDR_BASE
#endif
#endif

int main(int argc, char *argv[]) 
{
#ifdef MEASURE_ACCURACY
    volatile int *LOG_ADDR = ADDR_BASE + LOG_DELTA_STATS;
    *LOG_ADDR = 1;
    real_main(argc, argv, 0);
    *LOG_ADDR = 0;     /* Writing Zero to this Address will cause QEMU/KVM to exit */
#elif DISABLE_APP_REPEAT /* This option is useful in case we want to execute once or We want to use Analyzer */
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

    char *myargv[2];
    myargv[0] = "sha";
    myargv[1] = "/devices/disk/simulator/1";

    printf("SHA: In main function : For %d time\n\n", app_repeat_count);
    fin = fopen(myargv[1], "rb");
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
