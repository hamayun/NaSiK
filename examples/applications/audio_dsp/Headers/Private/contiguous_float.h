/* --------------------------------------------------------------------------
 * Project Name : Contiguous Memory
 * File Name : contiguous_float.h
 * Date : 11/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Allocation library to build contiguous memory data structure
 * -------------------------------------------------------------------------- */

#ifndef _CONTIGUOUS_FLOAT
#define _CONTIGUOUS_FLOAT

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>

/* ----- Project Dependencies of this file ---------------------------------- */

/* ----- Macros exported to the project ------------------------------------- */

/* ----- Types exported to the project -------------------------------------- */

typedef struct contiguous_float_container_s {
    float nfloat;
} contiguous_float_container_t;

typedef struct contiguous_float_pt_s {
    float nfloat;
    float delta;
} contiguous_float_pt_t;

/* ----- Function Prototypes exported to the project ------------------------ */

int16_t contiguous_float_alloc(uint32_t nfloat, contiguous_float_container_t ** contiguous_float_container);
int16_t contiguous_float_free(contiguous_float_container_t ** contiguous_float_container);

int16_t contiguous_float_realloc(uint32_t nfloat, contiguous_float_pt_t * contiguous_float_pt, contiguous_float_container_t ** contiguous_float_container);

float *contiguous_float_get_pt(contiguous_float_pt_t * contiguous_float_pt);
float *contiguous_float_get_pt_(contiguous_float_pt_t * contiguous_float_pt);

#endif

/* ----- End Of File -------------------------------------------------------- */
