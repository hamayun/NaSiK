/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : obj_app.c
 * Date : 10/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Main file containing myapp source code
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/fourier_obj.h"
#include "Private/obj_app.h"

/* ----- Macros declared for this file -------------------------------------- */

#ifdef HARTES
#include "hartes_tricks.h"
#endif

#define var_printf(format, var) printf( #var " = " #format "\n", var)

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

extern volatile int  *shared_lock;
extern volatile void *shared_buffer;

// #define DEBUG_APP

int main(int argc, char *argv[])
{
    // declaration of local objects
    float *xr, *xi;
    int16_t err = 0;
	unsigned long count = 0;

    fourier_obj_t *fourier_A = NULL;
    fourier_obj_t *fourier_S = NULL;

        // initialisation of local objects and allocation of local buffers
        err += fourier_obj_constructor(320, 1024, analysis, &fourier_A);
        err += fourier_obj_constructor(320, 1024, synthesis, &fourier_S);
        xr = (float *) malloc(1024 * sizeof(float));
        xi = (float *) malloc(1024 * sizeof(float));

        var_printf(%f, fourier_A->cfc.nfloat);
        var_printf(%f, fourier_S->cfc.nfloat);

        assert(err == 0);

        if (err == 0 && xr && xi) {
            printf("Beginning processing\n");
            printf("Address of shared_lock is 0x%x\n", shared_lock);
            printf("Address of shared_buffer is 0x%x\n", shared_buffer);
            // execution of local objects
            do {
				while(*shared_lock == 0);
#ifdef DEBUG_APP
            	printf("DSP: Begin Block %ld\n", count);
#endif
                memcpy(xr, (void *)shared_buffer, 320 * sizeof(float));
                err += fourier_obj_cycle(xr, xi, (float *) fourier_A);
                err += fourier_obj_cycle(xr, xi, (float *) fourier_S);
                memcpy((void *)shared_buffer, xr, 320 * sizeof(float));
#ifdef DEBUG_APP
            	printf("DSP: Ending Block %ld\n", count++);
#endif
				*shared_lock = 0;
            }
            while (err == 0);
            printf("Ending processing\n");
        } else {
            printf("An error occured during initialization.\n");
        }

        if (xr)
            free(xr);
        if (xi)
            free(xi);

    return 0;
}

/* ----- End Of File -------------------------------------------------------- */
