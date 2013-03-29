/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : lib_fft.c
 * Date : 04/2007
 * Author(s) : Francois Capman
 * Copyright : THALES Communications
 * Description : FFT/IFFT library
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/lib_fft.h"
#include "Private/twiddles.h"
#include "Private/twiddles_cf.h"
#include "Private/lib_fft_hw.h"

/* ----- Macros declared for this file -------------------------------------- */

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

/* SD * ------------------------------------------------------------------------
		Name:		FFT_dfto_rdx2
		Purpose:	performs Radix-2 DFT (algorithm from Cooley & Tuckey[1965])
		Arguments:	1 - (float[]) xr: fft real part.
					2 - (float[]) xi: fft imaginary part.
					3 - (int) N: number of samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_dfto_rdx2_cf(float xr[], float xi[], Twiddles_cf_t * twiddles_cf, int32_t N)
{

    Twiddles twiddles;

    twiddles_cf_get_(&twiddles, twiddles_cf);

    fft_hw(xr, xi, twiddles.m, twiddles.tw, twiddles.tw_ptr, N);

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_idfto_rdx2
		Purpose:	performs Radix-2 IDFT (algorithm from Cooley & Tuckey[1965])
		Arguments:	1 - (float[]) xr: fft real part.
					2 - (float[]) xi: fft imaginary part.
					3 - (int) N: number of samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_idfto_rdx2_cf(float xr[], float xi[], Twiddles_cf_t * twiddles_cf, int32_t N)
{
    Twiddles twiddles;

    twiddles_cf_get_(&twiddles, twiddles_cf);

    ifft_hw(xr, xi, twiddles.m, twiddles.tw, twiddles.tw_ptr, N);

    return;
}

/* ----- End Of File -------------------------------------------------------- */
