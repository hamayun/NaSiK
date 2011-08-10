/* +++Date last modified: 05-Jul-1997 */

/*
**  BITCNTS.C - Test program for bit counting functions
**
**  public domain by Bob Stout & Auke Reitsma
*/

#include <stdio.h>
#include <stdlib.h>
#include "conio.h"
#include <time.h>
#include <float.h>
#include "bitops.h"
#include <math.h>
#include <Processor/Profile.h>

#define FUNCS  7
#define IO_ON

#ifdef  MEASURE_QEMU_ACCURACY
    /* Copied the following definitions from qemu_wrapper_cts.h */
    #define QEMU_ADDR_BASE                              0x82000000
    #define LOG_DELTA_STATS                             0x0058
#endif

static int bit_shifter(long int x);

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
    for (app_repeat_count = 0; app_repeat_count < 5; app_repeat_count++)
        real_main(argc, argv, app_repeat_count);

    CPU_PROFILE_FLUSH_DATA();
#endif
    return 0;
}

int real_main(int argc, char *argv[], int app_repeat_count)
{
  clock_t start, stop;
  double ct, cmin = DBL_MAX, cmax = 0;
  int i;
  int cminix = 0;
  int cmaxix = 0;
  long j, n, seed;
  int iterations;
  static int (* pBitCntFunc[FUNCS])(long) = {
    bit_count,
    bitcount,
    ntbl_bitcnt,
    ntbl_bitcount,
    /*            btbl_bitcnt, DOESNT WORK*/
    BW_btbl_bitcount,
    AR_btbl_bitcount,
    bit_shifter
  };
#ifdef IO_ON
  static char *text[FUNCS] = {
    "Optimized 1 bit/loop counter",
    "Ratko's mystery algorithm",
    "Recursive bit count by nybbles",
    "Non-recursive bit count by nybbles",
    /*            "Recursive bit count by bytes",*/
    "Non-recursive bit count by bytes (BW)",
    "Non-recursive bit count by bytes (AR)",
    "Shift and count bits"
  };
#endif
//  if (argc<2) {
//    fprintf(stderr,"Usage: bitcnts <iterations>\n");
//    exit(-1);
//	}
//  iterations=atoi(argv[1]);

  printf("BITCOUNT: In main function : For %d time\n\n", app_repeat_count);
  iterations=1125000; // runme_large
  puts("Bit counter algorithm benchmark\n");

  for (i = 0; i < FUNCS; i++) {
    CPU_PROFILE_COMP_START();
    start = clock();

    for (j = n = 0, seed = rand(); j < iterations; j++, seed += 13)
	 n += pBitCntFunc[i](seed);

    stop = clock();
    ct = (stop - start) / (double)CLOCKS_PER_SEC;
    if (ct < cmin) {
	 cmin = ct;
	 cminix = i;
    }
    if (ct > cmax) {
	 cmax = ct;
	 cmaxix = i;
    }
    CPU_PROFILE_COMP_END();
    
#ifdef IO_ON
    CPU_PROFILE_IO_START();
    //printf("%-38s> Time: %7.3f sec.; Bits: %ld\n", text[i], ct, n);
    printf("%-38s> Time: %d sec.; Bits: %ld\n", text[i], (int)ct, n);
    CPU_PROFILE_IO_END();
#endif
  }

#ifdef IO_ON
  CPU_PROFILE_IO_START();
  printf("\nBest  > %s\n", text[cminix]);
  printf("Worst > %s\n", text[cmaxix]);
  CPU_PROFILE_IO_END();
#endif

  printf("\nDone\n");
  return 0;
}

static int bit_shifter(long int x)
{
  int i, n;

  for (i = n = 0; x && (i < (sizeof(long) * CHAR_BIT)); ++i, x >>= 1)
    n += (int)(x & 1L);
  return n;
}
