#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Processor/Profile.h>
#define UNLIMIT
#define MAXARRAY 60000 /* this number, if too large, will cause a seg. fault!! */

struct myStringStruct {
  char qstring[128];
};

int compare(const void *elem1, const void *elem2)
{
  int result;
  
  result = strcmp((*((struct myStringStruct *)elem1)).qstring, (*((struct myStringStruct *)elem2)).qstring);

  return (result < 0) ? 1 : ((result == 0) ? 0 : -1);
}

struct myStringStruct array[MAXARRAY];

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
    for (app_repeat_count = 0; app_repeat_count < 100; app_repeat_count++)
        real_main(argc, argv, app_repeat_count);

    CPU_PROFILE_FLUSH_DATA();
#endif
    return 0;
}

int
real_main(int argc, char *argv[], int app_repeat_count) {
    FILE *fp;
    FILE *fp1;
    FILE *fp2;
    int i,count=0;

    printf("QSORT: In main function : For %d time\n", app_repeat_count);
#if 0
  if (argc<2) {
    fprintf(stderr,"Usage: qsort_small <file>\n");
    exit(-1);
  }
  else {
    fp = fopen(argv[1],"r");
#endif
    fp  = fopen("/devices/disk/simulator/0", "r");
    if(!fp)
    {
	    printf("\nError Opening /devices/disk/simulator/0\n");
	    return (-1);
    }

    fp1 = fopen("/devices/disk/simulator/1", "r");
    if(!fp1)
    {
	    printf("\nError Opening /devices/disk/simulator/1\n");
	    return (-1);
    }
 
    fp2 = fopen("/devices/disk/simulator/2", "w");
    if(!fp2)
    {
        printf("\nError Opening /devices/disk/simulator/2\n");
        return (-1);
    }
 
    CPU_PROFILE_IO_START();
    while((fscanf(fp, "%s", &array[count].qstring) == 1) && (count < MAXARRAY)) {
        count++;
    }
    CPU_PROFILE_IO_END();
#if 0
  }
#endif
  printf("\nSorting %d Elements.\n",count);
  
  while(1);

  CPU_PROFILE_COMP_START();
  qsort(array,count,sizeof(struct myStringStruct),compare);
  CPU_PROFILE_COMP_END();

  printf("Sorted ... Writing to File\n");
  CPU_PROFILE_IO_START();
  for(i=0;i<count;i++)
  {
    fprintf(fp2, "%s\n", array[i].qstring);
    printf("%3d ", i);
    fflush(stdout);
  }
  CPU_PROFILE_IO_END();

  fclose(fp);
  fclose(fp1);
  fclose(fp2);

  printf("Done\n");
  return 0;
}
