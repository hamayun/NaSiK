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
#include "Private/twiddles_cf.h"

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
		Name:		FFT_bit_reverse
		Purpose:	performs bit-reverse re-ordering
		Arguments:	1 - (float[]) xr: fft real part.
					2 - (float[]) xi: fft imaginary part.
					3 - (int) N: number of samples.
					4 - (int) m: power of 2 (N = 2^m)
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_bit_reverse(float xr[], float xi[], int32_t N)
{
    int32_t i, j, weight;
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

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_dft_rdx2
		Purpose:	performs Radix-2 DFT (algorithm from Cooley & Tuckey[1965])
		Arguments:	1 - (float[]) xr: fft real part.
					2 - (float[]) xi: fft imaginary part.
					3 - (int) N: number of samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_dft_rdx2(float xr[], float xi[], int32_t N)
{
    int32_t i, j, k;
    int m;
    int order, half_order, nb_stages;
    float w_r, w_i, u_r, u_i, tmp_r, tmp_i;

    m = (int32_t) log2(N);

    FFT_bit_reverse(xr, xi, N);

    order = 1;

    for (nb_stages = 1; nb_stages <= m; nb_stages++) {
        half_order = order;
        order *= 2;

        w_r = +cosf(2.0 * M_PI / (float) order);
        w_i = -sinf(2.0 * M_PI / (float) order);

        u_r = 1.0f;
        u_i = 0.0f;

        for (j = 1; j <= half_order; j++) {
            for (i = j - 1; i < N; i += order) {
                k = i + half_order;

                tmp_r = (u_r * xr[k]) - (u_i * xi[k]);
                tmp_i = (u_i * xr[k]) + (u_r * xi[k]);

                xr[k] = xr[i] - tmp_r;
                xi[k] = xi[i] - tmp_i;
                xr[i] = xr[i] + tmp_r;
                xi[i] = xi[i] + tmp_i;
            }

            tmp_r = u_r;

            u_r = (u_r * w_r) - (u_i * w_i);
            u_i = (u_i * w_r) + (tmp_r * w_i);
        }
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_dfto_rdx2
		Purpose:	performs Radix-2 DFT (algorithm from Cooley & Tuckey[1965])
		Arguments:	1 - (float[]) xr: fft real part.
					2 - (float[]) xi: fft imaginary part.
					3 - (int) N: number of samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_dfto_rdx2(float xr[], float xi[], Twiddles twiddles, int32_t N)
{
    int32_t i, j, k, lr, li;
    int32_t m;
    int32_t order, half_order, nb_stages;
    int32_t i0, i1, i2, i3;

    float u_r, u_i, tmp_r, tmp_i;
    float tmp_r0, tmp_r1, tmp_r2, tmp_r3;
    float tmp_i0, tmp_i1, tmp_i2, tmp_i3;

    m = (int32_t) log2(N);

    FFT_bit_reverse(xr, xi, N);

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
            u_r = (float) (twiddles.tw[lr]);
            u_i = (float) (twiddles.tw[li]);

            for (i = j - 1; i < N; i += order) {
                k = i + half_order;

                tmp_r = (u_r * xr[k]) - (u_i * xi[k]);
                tmp_i = (u_i * xr[k]) + (u_r * xi[k]);

                xr[k] = xr[i] - tmp_r;
                xi[k] = xi[i] - tmp_i;
                xr[i] = xr[i] + tmp_r;
                xi[i] = xi[i] + tmp_i;
            }

            lr = lr + (int32_t) twiddles.tw_ptr[nb_stages - 1];
            li = li + (int32_t) twiddles.tw_ptr[nb_stages - 1];
        }
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_idft_rdx2
		Purpose:	performs Radix-2 IDFT (algorithm from Cooley & Tuckey[1965])
		Arguments:	1 - (float[]) xr: fft real part.
					2 - (float[]) xi: fft imaginary part.
					3 - (int) N: number of samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_idft_rdx2(float xr[], float xi[], int32_t N)
{
    int32_t i, j, k;
    int32_t m;
    int32_t order, half_order, nb_stages;
    float w_r, w_i, u_r, u_i, tmp_r, tmp_i;

    m = (int32_t) log2(N);

    FFT_bit_reverse(xr, xi, N);

    order = 1;

    for (nb_stages = 1; nb_stages <= m; nb_stages++) {
        half_order = order;
        order *= 2;

        w_r = cosf(2.0 * M_PI / (float) (order));
        w_i = sinf(2.0 * M_PI / (float) (order));

        u_r = 1.0f;
        u_i = 0.0f;

        for (j = 1; j <= half_order; j++) {
            for (i = j - 1; i < N; i += order) {
                k = i + half_order;

                tmp_r = (u_r * xr[k]) - (u_i * xi[k]);
                tmp_i = (u_i * xr[k]) + (u_r * xi[k]);

                xr[k] = xr[i] - tmp_r;
                xi[k] = xi[i] - tmp_i;
                xr[i] = xr[i] + tmp_r;
                xi[i] = xi[i] + tmp_i;
            }

            tmp_r = u_r;

            u_r = (u_r * w_r) - (u_i * w_i);
            u_i = (u_i * w_r) + (tmp_r * w_i);
        }
    }

    for (k = 0; k < N; k++) {
        xr[k] /= (float) (N);
        xi[k] /= (float) (N);
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_idfto_rdx2
		Purpose:	performs Radix-2 IDFT (algorithm from Cooley & Tuckey[1965])
		Arguments:	1 - (float[]) xr: fft real part.
					2 - (float[]) xi: fft imaginary part.
					3 - (int) N: number of samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_idfto_rdx2(float xr[], float xi[], Twiddles twiddles, int32_t N)
{
    int32_t i, j, k, lr, li;
    int32_t m;
    int32_t order, half_order, nb_stages;
    int32_t i0, i1, i2, i3;

    float u_r, u_i, tmp_r, tmp_i;

    m = (int32_t) log2(N);

    FFT_bit_reverse(xr, xi, N);

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
            u_r = (float) (twiddles.tw[lr]);
            u_i = (float) (twiddles.tw[li]);

            for (i = j - 1; i < N; i += order) {
                k = i + half_order;

                tmp_r = (u_r * xr[k]) + (u_i * xi[k]);
                tmp_i = (u_r * xi[k]) - (u_i * xr[k]);

                xr[k] = xr[i] - tmp_r;
                xi[k] = xi[i] - tmp_i;
                xr[i] = xr[i] + tmp_r;
                xi[i] = xi[i] + tmp_i;
            }

            lr = lr + (int32_t) twiddles.tw_ptr[nb_stages - 1];
            li = li + (int32_t) twiddles.tw_ptr[nb_stages - 1];
        }
    }

    for (k = 0; k < N; k++) {
        xr[k] /= (float) (N);
        xi[k] /= (float) (N);
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_sym
		Purpose:	FFT symmetry using the real input assumption.
		Arguments:	1 - (float[]) sr:.
					2 - (float[]) si:.
					3 - (int) N:.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_sym(float sr[], float si[], int32_t N)
{
    int32_t n;

    /* --- FFT symmetry ----------------------------------------------------- */

    for (n = 1; n < N / 2; n++) {
        sr[N - n] = sr[n];
        si[N - n] = -si[n];
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_psd
		Purpose:	FFT-based power spectrum density.
		Arguments:	1 - (float[]) sr:.
					2 - (float[]) si:.
					3 - (float[]) s_psd:.
					4 - (int) N:.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_psd(float sr[], float si[], float s_psd[], int32_t N)
{
    int32_t n;

    /* --- FFT power spectrum density --------------------------------------- */

    for (n = 0; n <= N / 2; n++) {
        s_psd[n] = (sr[n] * sr[n]) + (si[n] * si[n]);
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_mag
		Purpose:	FFT-based magnitude spectrum.
		Arguments:	1 - (float[]) sr:.
					2 - (float[]) si:.
					3 - (float[]) s_mod:.
					4 - (int) N:.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_mag(float sr[], float si[], float s_mod[], int32_t N)
{
    int32_t n;

    /* --- FFT magnitude spectrum ------------------------------------------- */

    for (n = 0; n <= N / 2; n++) {
        s_mod[n] = sqrtf((sr[n] * sr[n]) + (si[n] * si[n]));
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_log
		Purpose:	FFT-based log-spectrum.
		Arguments:	1 - (float[]) sr:.
					2 - (float[]) si:.
					3 - (float[]) s_log:.
					4 - (int) N:.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_log(float sr[], float si[], float s_log[], int32_t N)
{
    int32_t n;
    float s_mod;

    /* --- FFT log-spectrum ------------------------------------------------- */

    for (n = 0; n <= N / 2; n++) {
        s_mod = sqrtf((sr[n] * sr[n]) + (si[n] * si[n]));

        s_log[n] = (s_mod > 1.0) ? logf(s_mod) : 0.0f;
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_logdB
		Purpose:	FFT-based log-spectrum.
		Arguments:	1 - (float[]) sr:.
					2 - (float[]) si:.
					3 - (float[]) s_log:.
					4 - (int) N:.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_logdB(float sr[], float si[], float s_log[], int32_t N)
{
    int32_t n;
    float s_psd;

    /* --- FFT log-spectrum ------------------------------------------------- */

    for (n = 0; n <= N / 2; n++) {
        s_psd = (sr[n] * sr[n]) + (si[n] * si[n]);

        s_log[n] = (s_psd > 1.0) ? 10.0f * log10f(s_psd) : 0.0f;
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_phi
		Purpose:	FFT-based phase spectrum.
		Arguments:	1 - (float[]) sr:.
					2 - (float[]) si:.
					3 - (float[]) s_phi:.
					4 - (int) N:.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_phi(float sr[], float si[], float s_phi[], int32_t N)
{
    int32_t n;

    /* --- FFT phase spectrum ----------------------------------------------- */

    for (n = 0; n <= N / 2; n++) {
        if (si[n] != 0.0f) {
            s_phi[n] = atanf(sr[n] / si[n]);
        } else {
            s_phi[n] = 0.0f;
        }
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_apply_window
		Purpose:	FFT-based phase spectrum.
		Arguments:	1 - (float[]) s:.
					2 - (float[]) w:.
					3 - (float[]) ws:.
					4 - (int) N: .
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_apply_window(float s[], float w[], float ws[], int32_t N)
{
    int32_t n;

    /* --- Frame Windowing -------------------------------------------------- */

    for (n = 0; n < N; n++) {
        ws[n] = w[n] * s[n];
    }

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_wx2xr
		Purpose:	this function performs samples reordering.
		Arguments:	1 - (float[]) wx: input weighted frame.
					2 - (float[]) xr: reordered fft real part.
					3 - (int) nfft: fft size in samples.
					4 - (int) nwin: frame size in samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_wx2xr(float wx[], float xr[], int32_t nfft, int32_t nwin)
{
    //VEC_cpy_fvector(wx + nwin / 2, xr, 0, nwin / 2 - 1);
    memcpy(xr + 0, wx + nwin / 2 + 0, (nwin / 2 - 1 - 0 + 1) * sizeof(float));
    //VEC_cpy_fvector(wx, xr + nfft - nwin / 2, 0, nwin / 2 - 1);
    memcpy(xr + nfft - nwin / 2 + 0, wx + 0, (nwin / 2 - 1 - 0 + 1) * sizeof(float));

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_xr2wx
		Purpose:	this function performs samples reordering.
		Arguments:	1 - (float[]) xr: reordered fft real part.
					2 - (float[]) wx: input weighted frame.
					3 - (int) nfft: fft size in samples.
					4 - (int) nwin: frame size in samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_xr2wx(float xr[], float wx[], int32_t nfft, int32_t nwin)
{
    //VEC_cpy_fvector(xr, wx + nwin / 2, 0, nwin / 2 - 1);
    memcpy(wx + nwin / 2 + 0, xr + 0, (nwin / 2 - 1 - 0 + 1) * sizeof(float));
    //VEC_cpy_fvector(xr + nfft - nwin / 2, wx, 0, nwin / 2 - 1);
    memcpy(wx + 0, xr + nfft - nwin / 2 + 0, (nwin / 2 - 1 - 0 + 1) * sizeof(float));

    return;
}

/* SD * ------------------------------------------------------------------------
		Name:		FFT_center_xr
		Purpose:	this function performs samples reordering.
		Arguments:	1 - (float[]) xr: reordered fft real part.
					2 - (int) nfft: fft size in samples.
		Return:		R - none.
------------------------------------------------------------------------ * ED */

void FFT_center_xr(float xr[], int32_t nfft)
{
    int32_t k;
    float tmp;
    float *px1, *px2;

    px1 = xr;
    px2 = xr + nfft / 2;

    for (k = 0; k < nfft / 2; k++, px1++, px2++) {
        tmp = *px1;
        *px1 = *px2;
        *px2 = tmp;
    }

    return;
}

/*--------------------------------------------------------------------- * EOF */
