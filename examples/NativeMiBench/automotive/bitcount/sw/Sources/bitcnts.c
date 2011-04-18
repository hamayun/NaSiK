/* +++Date last modified: 05-Jul-1997 */

/*
**  BITCNTS.C - Test program for bit counting functions
**
**  public domain by Bob Stout & Auke Reitsma
*/

#include <stdio.h>
#include <stdlib.h>
#include "conio.h"
#include <limits.h>
#include <time.h>
#include <float.h>
#include "bitops.h"
#include <math.h>

#define FUNCS  7

static int CDECL bit_shifter(long int x);

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

int main(int argc, char *argv[])
{
  clock_t start, stop;
  double ct, cmin = DBL_MAX, cmax = 0;
  int i;
  int cminix = 0;
  int cmaxix = 0;
  long j, n, seed;
  int iterations;
  static int (* CDECL pBitCntFunc[FUNCS])(long) = {
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
  
  /* Dump Host Time to File
   */
  int checkpoint = 1;
  DumpHostTime(checkpoint++);

  iterations=1125000; // runme_large
  
  puts("Bit counter algorithm benchmark\n");
  
  for (i = 0; i < FUNCS; i++) {
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
    
#ifdef IO_ON
    printf("%-38s> Time: %7.3f sec.; Bits: %ld\n", text[i], ct, n);
#endif
  }
#ifdef IO_ON
  printf("\nBest  > %s\n", text[cminix]);
  printf("Worst > %s\n", text[cmaxix]);
#endif

  DumpHostTime(checkpoint++);
  return 0;
}

static int CDECL bit_shifter(long int x)
{
  int i, n;
  
  for (i = n = 0; x && (i < (sizeof(long) * CHAR_BIT)); ++i, x >>= 1)
    n += (int)(x & 1L);
  return n;
}
