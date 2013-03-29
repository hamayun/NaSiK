/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : twiddles.h
 * Date : May 5th, 2001
 * Author(s) : Francois Capman
 * Copyright : THALES Communications
 * Description :
 * -------------------------------------------------------------------------- */

#ifndef _TWIDDLES
#define _TWIDDLES

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>

/* ----- Project Dependencies of this file ---------------------------------- */

/* ----- Macros exported to the project ------------------------------------- */

/* ----- Types exported to the project -------------------------------------- */

typedef struct {
    int32_t n;
    int32_t m;
    float *tw_ptr;
    float *tw;
} Twiddles;

/* ----- Function Prototypes exported to the project ------------------------ */


#endif

/* ----- End Of File -------------------------------------------------------- */
