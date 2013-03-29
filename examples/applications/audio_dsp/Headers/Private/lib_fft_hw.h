/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : lib_fft_hw.c
 * Date : 12/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : implementation of the hartes compliant fft
 * -------------------------------------------------------------------------- */

#ifndef _FFT_HW_H
#define _FFT_HW_H

/* ----- System Dependencies of this file ----------------------------------- */

/* ----- Project Dependencies of this file ---------------------------------- */

/* ----- Macros exported to the project ------------------------------------- */

/* ----- Types exported to the project -------------------------------------- */

/* ----- Function Prototypes exported to the project ------------------------ */

void fft_hw(float *xr, float *xi, int m, float *twiddles_tw, float *twiddles_twptr, int N);
void ifft_hw(float * xr, float * xi, int m, float * twiddles_tw, float * twiddles_twptr, int N);

#endif

/* ----- End Of File -------------------------------------------------------- */
