/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : twiddles_cf.h
 * Date : 10/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Twiddles contiguous memory allocation / handling functions
 * -------------------------------------------------------------------------- */

#ifndef _TWIDDLES_CF
#define _TWIDDLES_CF

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "contiguous_float.h"
#include "twiddles.h"

/* ----- Macros exported to the project ------------------------------------- */

/* ----- Types exported to the project -------------------------------------- */

typedef struct Twiddles_cf_s {
    contiguous_float_container_t cfc;
    float n;
    float m;
    contiguous_float_pt_t tw_ptr;
    contiguous_float_pt_t tw;
} Twiddles_cf_t;

/* ----- Function Prototypes exported to the project ------------------------ */

int16_t twiddles_cf_allocation(uint32_t fft_size, Twiddles_cf_t ** twiddles_cf);
int16_t twiddles_cf_get(Twiddles * twiddles, Twiddles_cf_t * twiddles_cf);
int16_t twiddles_cf_get_(Twiddles * twiddles, Twiddles_cf_t * twiddles_cf);

#endif

/* ----- End Of File -------------------------------------------------------- */
