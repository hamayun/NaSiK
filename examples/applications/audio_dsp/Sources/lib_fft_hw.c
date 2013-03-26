/* --------------------------------------------------------------------------
 * Project Name : HARTES
 * File Name : lib_fft_hw.c
 * Date : 12/2009
 * Author(s) : Simon Thabuteau
 * Copyright : THALES Communications
 * Description : Implementation of hArtes compliant fft
 * -------------------------------------------------------------------------- */

/* ----- System Dependencies of this file ----------------------------------- */

/* ----- Project Dependencies of this file ---------------------------------- */

#include "Private/lib_fft_hw.h"

/* ----- Macros declared for this file -------------------------------------- */

/* ----- Types declared for this file --------------------------------------- */

/* ----- Variables declared for the project --------------------------------- */

/* ----- Function definition ------------------------------------------------ */

void fft_hw(float *xr, float *xi, int m, float *twiddles_tw, float *twiddles_twptr, int N)
{
    int i, j, k, lr, li;
    int order, half_order, nb_stages;
    int i0, i1, i2, i3;

    float u_r, u_i, tmp_r, tmp_i;
    float tmp_r0, tmp_r1, tmp_r2, tmp_r3;
    float tmp_i0, tmp_i1, tmp_i2, tmp_i3;

    int weight;
    float tmp;

    j = N / 2;

    for (i = 1; i < N - 1; i++) {
        if (i < j) {
            tmp = xr[i];
            xr[i] = xr[j];
            xr[j] = tmp;
            tmp = xi[i];
            xi[i] = xi[j];
            xi[j] = tmp;
        }

        weight = N / 2;

        while (weight <= j) {
            j -= weight;
            weight /= 2;
        }

        j += weight;
    }

    for (i = 0; i < N; i += 4) {
        i0 = i;
        i1 = i + 1;
        i2 = i + 2;
        i3 = i + 3;

        tmp_r0 = (xr[i0] + xr[i1]) + (xr[i2] + xr[i3]);
        tmp_r1 = (xr[i0] - xr[i1]) + (xi[i2] - xi[i3]);
        tmp_r2 = (xr[i0] + xr[i1]) - (xr[i2] + xr[i3]);
        tmp_r3 = (xr[i0] - xr[i1]) - (xi[i2] - xi[i3]);

        tmp_i0 = (xi[i0] + xi[i1]) + (xi[i2] + xi[i3]);
        tmp_i1 = (xi[i0] - xi[i1]) - (xr[i2] - xr[i3]);
        tmp_i2 = (xi[i0] + xi[i1]) - (xi[i2] + xi[i3]);
        tmp_i3 = (xi[i0] - xi[i1]) + (xr[i2] - xr[i3]);

        xr[i0] = tmp_r0;
        xr[i1] = tmp_r1;
        xr[i2] = tmp_r2;
        xr[i3] = tmp_r3;

        xi[i0] = tmp_i0;
        xi[i1] = tmp_i1;
        xi[i2] = tmp_i2;
        xi[i3] = tmp_i3;
    }

    order = 4;

    for (nb_stages = 3; nb_stages <= m; nb_stages++) {
        half_order = order;
        order *= 2;

        lr = 0;
        li = N / 4;

        for (j = 1; j <= half_order; j++) {
            u_r = (float) (twiddles_tw[lr]);
            u_i = (float) (twiddles_tw[li]);

            for (i = j - 1; i < N; i += order) {
                k = i + half_order;

                tmp_r = (u_r * xr[k]) - (u_i * xi[k]);
                tmp_i = (u_i * xr[k]) + (u_r * xi[k]);

                xr[k] = xr[i] - tmp_r;
                xi[k] = xi[i] - tmp_i;
                xr[i] = xr[i] + tmp_r;
                xi[i] = xi[i] + tmp_i;
            }

            lr = lr + (int) twiddles_twptr[nb_stages - 1];
            li = li + (int) twiddles_twptr[nb_stages - 1];
        }
    }

}

void ifft_hw(float * xr, float * xi, int m, float * twiddles_tw, float * twiddles_twptr, int N)
{
    int i, j, k, lr, li;
    int order, half_order, nb_stages;
    int i0, i1, i2, i3;

    float u_r, u_i, tmp_r, tmp_i;

    int weight;
    float tmp;

    j = N / 2;

    for (i = 1; i < N - 1; i++) {
        if (i < j) {
            tmp = xr[i];
            xr[i] = xr[j];
            xr[j] = tmp;
            tmp = xi[i];
            xi[i] = xi[j];
            xi[j] = tmp;
        }

        weight = N / 2;

        while (weight <= j) {
            j -= weight;
            weight /= 2;
        }

        j += weight;
    }

    for (i = 0; i < N; i += 4) {
        i0 = i;
        i1 = i + 1;
        i2 = i + 2;
        i3 = i + 3;

        tmp_r = xr[i1];
        tmp_i = xi[i1];

        xr[i1] = xr[i0] - tmp_r;
        xi[i1] = xi[i0] - tmp_i;
        xr[i0] = xr[i0] + tmp_r;
        xi[i0] = xi[i0] + tmp_i;

        tmp_r = xr[i3];
        tmp_i = xi[i3];

        xr[i3] = xr[i2] - tmp_r;
        xi[i3] = xi[i2] - tmp_i;
        xr[i2] = xr[i2] + tmp_r;
        xi[i2] = xi[i2] + tmp_i;

        tmp_r = xr[i2];
        tmp_i = xi[i2];

        xr[i2] = xr[i0] - tmp_r;
        xi[i2] = xi[i0] - tmp_i;
        xr[i0] = xr[i0] + tmp_r;
        xi[i0] = xi[i0] + tmp_i;

        tmp_r = -xi[i3];
        tmp_i = xr[i3];

        xr[i3] = xr[i1] - tmp_r;
        xi[i3] = xi[i1] - tmp_i;
        xr[i1] = xr[i1] + tmp_r;
        xi[i1] = xi[i1] + tmp_i;
    }

    order = 4;

    for (nb_stages = 3; nb_stages <= m; nb_stages++) {
        half_order = order;
        order *= 2;

        lr = 0;
        li = N / 4;

        for (j = 1; j <= half_order; j++) {
            u_r = (float) (twiddles_tw[lr]);
            u_i = (float) (twiddles_tw[li]);

            for (i = j - 1; i < N; i += order) {
                k = i + half_order;

                tmp_r = (u_r * xr[k]) + (u_i * xi[k]);
                tmp_i = (u_r * xi[k]) - (u_i * xr[k]);

                xr[k] = xr[i] - tmp_r;
                xi[k] = xi[i] - tmp_i;
                xr[i] = xr[i] + tmp_r;
                xi[i] = xi[i] + tmp_i;
            }

            lr = lr + (int) twiddles_twptr[nb_stages - 1];
            li = li + (int) twiddles_twptr[nb_stages - 1];
        }
    }

    for (k = 0; k < N; k++) {
        xr[k] /= (float) (N);
        xi[k] /= (float) (N);
    }

}

/* ----- End Of File -------------------------------------------------------- */
