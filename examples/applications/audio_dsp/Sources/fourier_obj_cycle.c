/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : fourier_obj.c
 * Date : 10/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Fourier Analysis/Synthesis implementation
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/contiguous_float.h"
#include "Private/fourier_obj.h"
#include "Private/lib_fft.h"
#include "Private/twiddles_cf.h"

#ifdef HARTES
#include "hartes_tricks.h"
#endif

/* ----- Macros declared for this file -------------------------------------- */

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

int fourier_obj_cycle(float *real, float *imag, float *fourier_obj_fpt)
{
    int16_t err = -1;
    int32_t L2;
    float *temp = NULL;
    Twiddles_cf_t *twiddles_cf;

    fourier_obj_t *fourier_obj = (fourier_obj_t *) fourier_obj_fpt;

    if (real != NULL && imag != NULL && fourier_obj != NULL) {
        L2 = (int32_t) fourier_obj->frame_size / 2;
        temp = (float *) contiguous_float_get_pt_(&(fourier_obj->temp));
        twiddles_cf = (Twiddles_cf_t *) contiguous_float_get_pt_(&fourier_obj->Tw);
        if (temp && twiddles_cf) {
            switch (((fourier_obj_mode_t) fourier_obj->mode)) {
            case analysis:
                memset(temp, 0, ((uint32_t) fourier_obj->fft_size) * sizeof(float));
                memcpy(temp, real + L2, L2 * sizeof(float));
                memcpy(temp + ((uint32_t) fourier_obj->fft_size) - L2, real, L2 * sizeof(float));
                memcpy(real, temp, ((uint32_t) fourier_obj->fft_size) * sizeof(float));
                memset(imag, 0, ((uint32_t) fourier_obj->fft_size) * sizeof(float));
                //DFT_cplx_dfft(real, imag, ((uint32_t)fourier_obj->fft_size), temp);
                //FFT_dft_rdx2(real, imag, ((uint32_t) fourier_obj->fft_size));
                //FFT_dfto_rdx2(real, imag, twiddles, ((uint32_t)fourier_obj->fft_size));
                FFT_dfto_rdx2_cf(real, imag, twiddles_cf, ((uint32_t) fourier_obj->fft_size));
                break;
            case synthesis:
                FFT_sym(real, imag, ((uint32_t) fourier_obj->fft_size));
                //DFT_cplx_ifft(real, imag, ((uint32_t)fourier_obj->fft_size), temp);
                //FFT_idft_rdx2(real, imag, ((uint32_t) fourier_obj->fft_size));
                //FFT_idfto_rdx2(real, imag, twiddles, ((uint32_t)fourier_obj->fft_size));
                FFT_idfto_rdx2_cf(real, imag, twiddles_cf, ((uint32_t) fourier_obj->fft_size));
                memcpy(temp + L2, real, L2 * sizeof(float));
                memcpy(temp, real + ((uint32_t) fourier_obj->fft_size) - L2, L2 * sizeof(float));
                memset(real, 0, ((uint32_t) fourier_obj->fft_size) * sizeof(float));
                memcpy(real, temp, ((uint32_t) fourier_obj->fft_size) * sizeof(float));
                memset(imag, 0, ((uint32_t) fourier_obj->fft_size) * sizeof(float));
                break;
            }
            err = 0;
        }
    }

    return (int) err;
}

/* ----- End Of File -------------------------------------------------------- */
