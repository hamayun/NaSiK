/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : lib_fft.c
 * Date : 04/2007
 * Author(s) : Francois Capman
 * Copyright : THALES Communications
 * Description : FFT/IFFT library
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/lib_fft.h"
#include "Private/twiddles.h"

#ifdef HARTES
#include "hartes_tricks.h"
#endif

/* ----- Macros declared for this file -------------------------------------- */

#ifndef log2f
#define log2f(x) (((x) > 0.0) ? log10f(x)/log10f(2) : -FLT_MAX)
#endif
#ifndef log2
#define log2(x) (((x) > 0.0) ? log10(x)/log10(2) : -DBL_MAX)
#endif

#ifndef M_PI
#define M_PI           3.14159265358979323846
#endif

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */


/* ----- End Of File -------------------------------------------------------- */

/* SD * ------------------------------------------------------------------------
		Name:		FFT_allocate_twiddles
		Purpose:	Memory allocation of "twiddles" arrays.
		Arguments:	1 - (int) N: FFT size.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_allocate_twiddles(Twiddles * twiddles, int32_t N)
{
    int32_t m;

    m = (int32_t) log2(N);

    twiddles->m = m;
    twiddles->n = N;

    twiddles->tw_ptr = (float *) calloc(sizeof(float), m);
    twiddles->tw = (float *) calloc(sizeof(float), N + N / 4);

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_deallocate_twiddles
		Purpose:	performs memory deallocation of "twiddles" arrays.
		Arguments:	1 - (int) N: number of samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_deallocate_twiddles(Twiddles * twiddles, int32_t N)
{
    int32_t m;

    m = (int32_t) log2(N);

    free(twiddles->tw_ptr);
    free(twiddles->tw);

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_generate_twiddles
		Purpose:	generates table of twiddles.
		Arguments:	1 - (int) N: number of samples.
					2 - (int) m: power of 2 (N = 2^m)
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_generate_twiddles(Twiddles * twiddles, int32_t N)
{
    int32_t i, j, k;
    int32_t m;
    int32_t order, half_order;
    float u_r, u_i;
    float v_r, v_i;
    float w_r, w_i;
    float **tw_r;
    float **tw_i;

    m = (int) log2(N);

    tw_r = (float **) calloc(m, sizeof(float *));
    for (i = 0; i < m; i++)
        tw_r[i] = (float *) calloc(N, sizeof(float));
    tw_i = (float **) calloc(m, sizeof(float *));
    for (i = 0; i < m; i++)
        tw_i[i] = (float *) calloc(N, sizeof(float));

    order = 1;

    for (k = 0; k < m; k++) {
        twiddles->tw_ptr[m - k - 1] = (float) order;

        half_order = order;
        order *= 2;

        w_r = +cosf(2.0 * M_PI / (float) order);
        w_i = -sinf(2.0 * M_PI / (float) order);

        u_r = 1.0;
        u_i = 0.0;

        tw_r[k][0] = u_r;
        tw_i[k][0] = u_i;

        for (j = 1; j < half_order; j++) {
            v_r = u_r;
            v_i = u_i;

            u_r = (v_r * w_r) - (v_i * w_i);
            u_i = (v_i * w_r) + (v_r * w_i);

            tw_r[k][j] = u_r;
            tw_i[k][j] = u_i;
        }

    }

    memcpy(twiddles->tw + 0, &tw_r[m - 1][0] + 0, ((N / 2 - 1) - 0 + 1) * sizeof(float));
    memcpy(twiddles->tw + (N / 2) + 0, &tw_i[m - 1][N / 4] + 0, ((N / 4 - 1) - 0 + 1) * sizeof(float));

    for (i = 0; i < m; i++)
        free(tw_r[i]);
    free(tw_r);
    for (i = 0; i < m; i++)
        free(tw_i[i]);
    free(tw_i);

    return;
}

/*--------------------------------------------------------------------- * EOF */
