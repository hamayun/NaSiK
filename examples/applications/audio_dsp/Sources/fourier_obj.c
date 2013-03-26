/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : fourier_obj.c
 * Date : 10/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Fourier Analysis/Synthesis implementation
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/contiguous_float.h"
#include "Private/fourier_obj.h"
#include "Private/lib_fft.h"
#include "Private/twiddles.h"
#include "Private/twiddles_cf.h"

/* ----- Macros declared for this file -------------------------------------- */

#ifndef log2f
#define log2f(x) (((x) > 0.0) ? log10f(x)/log10f(2) : -FLT_MAX)
#endif
#ifndef log2
#define log2(x) (((x) > 0.0) ? log10(x)/log10(2) : -DBL_MAX)
#endif

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

int16_t fourier_obj_constructor(uint32_t frame_size, uint32_t fft_size, fourier_obj_mode_t mode, fourier_obj_t ** fourier_obj)
{
    int16_t err = -1;
    Twiddles_cf_t *twiddles_cf = NULL, *obj_twiddles_cf;
    Twiddles twiddles;

    if (frame_size > 0 && fft_size > 0 && fourier_obj != NULL && *fourier_obj == NULL) {
        if (contiguous_float_alloc(sizeof(fourier_obj_t) / sizeof(float), (contiguous_float_container_t **) fourier_obj) == 0) {
            (*fourier_obj)->mode = (float) mode;
            (*fourier_obj)->frame_size = (float) frame_size;
            (*fourier_obj)->fft_size = (float) fft_size;
            err = 0;
            err += twiddles_cf_allocation(fft_size, &twiddles_cf);
            err += twiddles_cf_get(&twiddles, twiddles_cf);
            FFT_generate_twiddles(&twiddles, fft_size);
            err += contiguous_float_realloc(twiddles_cf->cfc.nfloat, (contiguous_float_pt_t *) & ((*fourier_obj)->Tw), (contiguous_float_container_t **) fourier_obj);
            obj_twiddles_cf = (Twiddles_cf_t *) contiguous_float_get_pt(&((*fourier_obj)->Tw));
            memcpy(obj_twiddles_cf, twiddles_cf, twiddles_cf->cfc.nfloat * sizeof(float));
            err += contiguous_float_realloc((2 * fft_size + 1), (contiguous_float_pt_t *) & ((*fourier_obj)->temp), (contiguous_float_container_t **) fourier_obj);
            err += contiguous_float_free((contiguous_float_container_t **) & twiddles_cf);
        }
    }

    assert(err == 0);
    return err;
}

int16_t fourier_obj_destructor(fourier_obj_t ** fourier_obj)
{
    int16_t err = -1;

    if (fourier_obj != NULL) {
        err = 0;
        err += contiguous_float_free((contiguous_float_container_t **) fourier_obj);
    }

    assert(err == 0);
    return err;
}

/* ----- End Of File -------------------------------------------------------- */
