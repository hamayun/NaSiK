/* --------------------------------------------------------------------------
 * Project Name : Contiguous Memory
 * File Name : contiguous_float.c
 * Date : 11/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Allocation library to build contiguous memory data structure
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>
#include <stdlib.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/contiguous_float.h"

#ifdef HARTES
#include "hartes_tricks.h"
#endif

/* ----- Macros declared for this file -------------------------------------- */

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

float *contiguous_float_get_pt_(contiguous_float_pt_t * contiguous_float_pt)
{
    float *pt;

    if (contiguous_float_pt != NULL && contiguous_float_pt->delta > 0)
        pt = (float *) contiguous_float_pt + (uint32_t) contiguous_float_pt->delta;
    else
        pt = NULL;

    return pt;
}

/* ----- End Of File -------------------------------------------------------- */
