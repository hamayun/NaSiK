/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : fourier_obj.h
 * Date : 10/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Fourier Analysis/Synthesis implementation
 * -------------------------------------------------------------------------- */

#ifndef _FOURIER_OBJ
#define _FOURIER_OBJ

/* ----- System Dependencies of this file ----------------------------------- */

#include <stdint.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "contiguous_float.h"

/* ----- Macros exported to the project ------------------------------------- */

/* ----- Types exported to the project -------------------------------------- */

typedef enum fourier_obj_mode_e {
    analysis = 0, synthesis
} fourier_obj_mode_t;

typedef struct fourier_obj_s {
    contiguous_float_container_t cfc;
    float mode;
    float frame_size;
    float fft_size;
    contiguous_float_pt_t temp;
    contiguous_float_pt_t Tw;
} fourier_obj_t;

/* ----- Function Prototypes exported to the project ------------------------ */

int16_t fourier_obj_constructor(uint32_t frame_size, uint32_t fft_size, fourier_obj_mode_t mode, fourier_obj_t ** fourier_obj);
int16_t fourier_obj_destructor(fourier_obj_t ** fourier_obj);
#ifdef HARTES
#pragma map call_hw dsp 1
#endif
int fourier_obj_cycle(float *real, float *imag, float *fourier_obj_fpt);

#endif

/* ----- End Of File -------------------------------------------------------- */
