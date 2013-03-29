/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : lib_fft.c
 * Date : 04/2007
 * Author(s) : Francois Capman
 * Copyright : THALES Communications
 * Description : FFT/IFFT library
 * -------------------------------------------------------------------------- */

#ifndef _LIB_FFT
#define _LIB_FFT

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "twiddles.h"
#include "twiddles_cf.h"

/* ----- Macros exported to the project ------------------------------------- */

/* ----- Types exported to the project -------------------------------------- */

/* ----- Function Prototypes exported to the project ------------------------ */

void FFT_allocate_twiddles(Twiddles * twiddles, int32_t N);

void FFT_deallocate_twiddles(Twiddles * twiddles, int32_t N);

void FFT_generate_twiddles(Twiddles * twiddles, int32_t N);

void FFT_bit_reverse(float xr[], float xi[], int32_t N);

void FFT_dft_rdx2(float xr[], float xi[], int32_t N);
void FFT_dfto_rdx2(float xr[], float xi[], Twiddles tw, int32_t N);
void FFT_dfto_rdx2_cf(float xr[], float xi[], Twiddles_cf_t * tw_cf_t, int32_t N);

void FFT_idft_rdx2(float xr[], float xi[], int32_t N);
void FFT_idfto_rdx2(float xr[], float xi[], Twiddles tw, int32_t N);
void FFT_idfto_rdx2_cf(float xr[], float xi[], Twiddles_cf_t * tw_cf, int32_t N);

void FFT_sym(float sr[], float si[], int32_t N);

void FFT_psd(float sr[], float si[], float s_psd[], int32_t N);

void FFT_mag(float sr[], float si[], float s_mod[], int32_t N);

void FFT_log(float sr[], float si[], float s_log[], int32_t N);

void FFT_phi(float sr[], float si[], float s_phi[], int32_t N);

void FFT_logdB(float sr[], float si[], float s_log[], int32_t N);

void FFT_apply_window(float s[], float w[], float ws[], int32_t N);

void FFT_wx2xr(float wx[], float xr[], int32_t nfft, int32_t nwin);

void FFT_xr2wx(float xr[], float wx[], int32_t nfft, int32_t nwin);

void FFT_center_xr(float xr[], int32_t nfft);

#endif

/* ----- End Of File -------------------------------------------------------- */
