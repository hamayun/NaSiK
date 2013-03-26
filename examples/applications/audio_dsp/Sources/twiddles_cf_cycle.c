/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : twiddles_cf_cycle.c
 * Date : 10/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Twiddles handling functions
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/contiguous_float.h"
#include "Private/twiddles.h"
#include "Private/twiddles_cf.h"

/* ----- Macros declared for this file -------------------------------------- */

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

int16_t twiddles_cf_get_(Twiddles * twiddles, Twiddles_cf_t * twiddles_cf)
{
    int16_t err = -1;

    if (twiddles && twiddles_cf) {
        twiddles->n = (uint32_t) twiddles_cf->n;
        twiddles->m = (uint32_t) twiddles_cf->m;
        twiddles->tw_ptr = (float *) contiguous_float_get_pt_(&(twiddles_cf->tw_ptr));
        twiddles->tw = (float *) contiguous_float_get_pt_(&(twiddles_cf->tw));
        err = 0;
    }

    return err;
}

/* ----- End Of File -------------------------------------------------------- */
