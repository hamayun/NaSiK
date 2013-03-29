/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : twiddles_cf.c
 * Date : 10/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Twiddles handling functions
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

int16_t twiddles_cf_allocation(uint32_t fft_size, Twiddles_cf_t ** twiddles_cf)
{
    int16_t err = -1;

    if (fft_size > 0 && twiddles_cf != NULL && *twiddles_cf == NULL) {
        if (contiguous_float_alloc(sizeof(Twiddles_cf_t) / sizeof(float), (contiguous_float_container_t **) twiddles_cf) == 0) {
            (*twiddles_cf)->n = (float) fft_size;
            (*twiddles_cf)->m = (float) log2(fft_size);
            err = 0;
            err += contiguous_float_realloc(((uint32_t) (*twiddles_cf)->m), (contiguous_float_pt_t *) & ((*twiddles_cf)->tw_ptr), (contiguous_float_container_t **) twiddles_cf);
            err += contiguous_float_realloc((fft_size + fft_size / 4), (contiguous_float_pt_t *) & ((*twiddles_cf)->tw), (contiguous_float_container_t **) twiddles_cf);
        }
    }

    assert(err == 0);
    return err;
}

int16_t twiddles_cf_get(Twiddles * twiddles, Twiddles_cf_t * twiddles_cf)
{
    int16_t err = -1;

    if (twiddles && twiddles_cf) {
        twiddles->n = (uint32_t) twiddles_cf->n;
        twiddles->m = (uint32_t) twiddles_cf->m;
        twiddles->tw_ptr = (float *) contiguous_float_get_pt(&(twiddles_cf->tw_ptr));
        twiddles->tw = (float *) contiguous_float_get_pt(&(twiddles_cf->tw));
        err = 0;
    }

    assert(err == 0);
    return err;
}

/* ----- End Of File -------------------------------------------------------- */
