/*************************************************************************************
 * File   : libu.c, file for JPEG-JFIF Multi-thread decoder    
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :  Alexandre Chagoya, alexandre.chagoya-garzon@imag.fr (dol porting)    
 *              Patrice, GERIN patrice.gerin@imag.fr
 * Bug Fixer(s) :   
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/times.h>
#include <dpn.h>
#include <cpu.h>

#include "mjpeg.h"
#include "utils.h"

#define MCU_INDEX(ptr, index) (ptr + ((index) * MCU_sx * MCU_sy))
#define MCU_LINE(ptr,n) (ptr + ((n) * MCU_sx))

#define FB_Y_LINE(ptr,n) (ptr + ((n) * MCU_sx * NB_MCU_X))
#define FB_Y_INDEX(ptr,x,y) (ptr + ((y) * MCU_sy * MCU_sx * NB_MCU_X) + ((x) * MCU_sx))

#define FB_UV_LINE(ptr,n) (ptr + (((n) * MCU_sx * NB_MCU_X) >> 1))
#define FB_U_INDEX(ptr,x,y) (ptr + (((y) * MCU_sy * SOF_section . width) >> 1)	\
		+ (((x) * MCU_sx >> 1)) + (SOF_section . width * SOF_section . height))

#define FB_V_INDEX(ptr,x,y) (ptr + (((y) * MCU_sy * SOF_section . width >> 1))	\
		+ (((x) * MCU_sx >> 1)) + ((SOF_section . width * SOF_section . height * 3) >> 1))

extern uint32_t * SOCLIB_FB_BASE;

int dispatch_process (Channel * c[NB_IDCT + 1]) {

	uint8_t * MCU_YCbCr = NULL;
	uint8_t * picture = NULL;
	uint8_t * CELLS = NULL, * Y_SRC = NULL, * Y_DST = NULL;
	uint8_t * U_SRC = NULL, * U_DST = NULL;
	uint8_t * V_SRC = NULL, * V_DST = NULL;
	uint8_t * uv_line_src = NULL, * uv_line_dst = NULL;
	uint8_t idct_index = 0;

	uint16_t NB_MCU_X = 0;
	uint16_t NB_MCU_Y = 0;
	uint16_t NB_CELLS = 0;
	uint16_t NB_MCU = 0;

	uint32_t flit_bytes = 0,  flit_size = 0;
	uint32_t YV = 0, YH = 0;
	uint32_t LB_X = 0, LB_Y = 0;
	uint32_t * y_line_dst = NULL, * y_line_src = NULL;

#ifdef PROGRESS
	char progress_tab[4] = {'/', '-', '\\', '|'};
	uint32_t imageCount = 1;
	uint32_t block_index = 0;
#endif

	SOF_section_t SOF_section;

	channelRead (c[0], (unsigned char*) & SOF_section, sizeof (SOF_section));
	channelRead (c[0], (unsigned char*) & YV, sizeof (uint32_t));
	channelRead (c[0], (unsigned char*) & YH, sizeof (uint32_t));
	channelRead (c[0], (unsigned char*) & flit_size, sizeof (uint32_t));

	flit_bytes = flit_size * MCU_sx * MCU_sy * sizeof (uint8_t);

	NB_MCU_X = intceil(SOF_section.width, MCU_sx);
	NB_MCU_Y = intceil(SOF_section.height, MCU_sy);
	NB_CELLS = YV * YH + 2;

	picture = (uint8_t *) malloc (SOF_section . width * SOF_section . height * 2);
	if (picture == NULL) printf ("%s,%d: malloc failed\n", __FILE__, __LINE__);

	MCU_YCbCr = (uint8_t *) malloc(flit_bytes);
	if (MCU_YCbCr == NULL) printf ("%s,%d: malloc failed\n", __FILE__, __LINE__);

	NB_MCU = NB_MCU_Y * NB_MCU_X;

#ifdef PROGRESS
	printf ("Image %lu : |", imageCount++);
	fflush (stdout);
#endif

	while (1) {
		channelRead (c[idct_index + 1], MCU_YCbCr, flit_bytes);
		idct_index = (idct_index + 1) % NB_IDCT;

		for (int flit_index = 0; flit_index < flit_size; flit_index += NB_CELLS) {
			CELLS = MCU_INDEX(MCU_YCbCr, flit_index);

			for (int cell_y_index = 0; cell_y_index < YV; cell_y_index += 1) {
				for (int cell_x_index = 0; cell_x_index < YH; cell_x_index += 1) {
					Y_SRC = MCU_INDEX(CELLS, (YH * cell_y_index + cell_x_index));
					Y_DST = FB_Y_INDEX(picture, LB_X + cell_x_index, LB_Y + cell_y_index);

					for (int line_index = 0; line_index < MCU_sy; line_index += 1) {
						y_line_src = (uint32_t *) MCU_LINE(Y_SRC, line_index);
						y_line_dst = (uint32_t *) FB_Y_LINE(Y_DST, line_index);
						*y_line_dst++ = *y_line_src++; *y_line_dst++ = *y_line_src++;
					}
				}

				U_SRC = MCU_INDEX(CELLS, (YH * YV));
				U_DST = FB_U_INDEX(picture, LB_X, LB_Y + cell_y_index);
				
				for (int line_index = 0; line_index < MCU_sy; line_index += 1) {
					uv_line_src = MCU_LINE(U_SRC, line_index);
					uv_line_dst = FB_UV_LINE(U_DST, line_index);

					for (int i = 0; i < (MCU_sx / (3 - YH)); i += 1) uv_line_dst[i] = uv_line_src[i * (3 - YH)];
				}

				V_SRC = MCU_INDEX(CELLS, (YH * YV + 1));
				V_DST = FB_V_INDEX(picture, LB_X, LB_Y + cell_y_index);
				
				for (int line_index = 0; line_index < MCU_sy; line_index += 1) {
					uv_line_src = MCU_LINE(V_SRC, line_index);
					uv_line_dst = FB_UV_LINE(V_DST, line_index);

					for (int i = 0; i < (MCU_sx / (3 - YH)); i += 1) uv_line_dst[i] = uv_line_src[i * (3 - YH)];
				}
			}

			LB_X = (LB_X + YH) % NB_MCU_X;

#ifdef PROGRESS
			printf ("\033[1D%c", progress_tab[block_index++ % 4]);
			fflush (stdout);
#endif

			if (LB_X == 0) {
				LB_Y = (LB_Y + YV) % NB_MCU_Y;
				if (LB_Y == 0) memcpy ((void *) SOCLIB_FB_BASE, picture,
            SOF_section . width * SOF_section . height * 2);
			}

			if (LB_X == 0 && LB_Y == 0) {
        volatile uint32_t tmp;
        CPU_READ(UINT32,SOCLIB_FB_BASE,tmp);
      }

#ifdef PROGRESS
			if (LB_X == 0 && LB_Y == 0) {
				printf ("\033[1Ddone\n");
				printf ("Image %lu : |", imageCount++);
				fflush (stdout);
			}
#endif
		}
	}

	return 0;
}
