/* --------------------------------------------------------------------------
 * Project Name : Contiguous Memory
 * File Name : contiguous_float.c
 * Date : 11/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Allocation library to build contiguous memory data structure
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/contiguous_float.h"

/* ----- Macros declared for this file -------------------------------------- */

#ifdef HARTES
#include "hartes_tricks.h"
#endif

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

int16_t contiguous_float_alloc(uint32_t nfloat, contiguous_float_container_t ** contiguous_float_container)
{
    int16_t err = -1;

    if (nfloat > 0 && contiguous_float_container != NULL && *contiguous_float_container == NULL) {
        *contiguous_float_container = (contiguous_float_container_t *) calloc(nfloat, sizeof(float));

        if (*contiguous_float_container != NULL) {
            (*contiguous_float_container)->nfloat = nfloat;
            err = 0;
        }
    }

    assert(err == 0);
    return err;
}

int16_t contiguous_float_free(contiguous_float_container_t ** contiguous_float_container)
{
    int16_t err = -1;

    if (contiguous_float_container != NULL && *contiguous_float_container != NULL) {
        free(*contiguous_float_container);
        *contiguous_float_container = NULL;
        err = 0;
    }

    assert(err == 0);
    return err;
}

int16_t contiguous_float_realloc(uint32_t nfloat, contiguous_float_pt_t * contiguous_float_pt, contiguous_float_container_t ** contiguous_float_container)
{
    int16_t err = -1;
    uint32_t old_container_nfloat;
    uint32_t pt_relative_location;
    contiguous_float_container_t *new_container;
    contiguous_float_pt_t *new_pt;

    if (nfloat > 0 && contiguous_float_pt != NULL && contiguous_float_container != NULL && *contiguous_float_container != NULL) {
        old_container_nfloat = (*contiguous_float_container)->nfloat;
        pt_relative_location = (float *) contiguous_float_pt - (float *) *contiguous_float_container;
        new_container = realloc(*contiguous_float_container, (old_container_nfloat + nfloat) * sizeof(float));
        if (new_container != NULL) {
            *contiguous_float_container = new_container;
            (*contiguous_float_container)->nfloat += nfloat;
            new_pt = (contiguous_float_pt_t *) ((float *) new_container + pt_relative_location);
            new_pt->nfloat = nfloat;
            new_pt->delta = old_container_nfloat - pt_relative_location;
            memset((float *) new_pt + (uint32_t) new_pt->delta, 0, nfloat * sizeof(float));
            err = 0;
        }
    }

    assert(err == 0);
    return err;
}

float *contiguous_float_get_pt(contiguous_float_pt_t * contiguous_float_pt)
{
    float *pt;

    if (contiguous_float_pt != NULL && contiguous_float_pt->delta > 0)
        pt = (float *) contiguous_float_pt + (uint32_t) contiguous_float_pt->delta;
    else
        pt = NULL;

    return pt;
}

/* ----- End Of File -------------------------------------------------------- */
