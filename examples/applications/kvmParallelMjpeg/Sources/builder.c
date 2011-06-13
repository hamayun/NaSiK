/*
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Patrice GERIN patrice.gerin@imag.fr
 * Bug Fixer(s) :   Xavier GUERIN xavier.guerin@imag.fr
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/times.h>
#include <Private/Builder.h>

void builder ( SOF_section_t SOF_section, uint32_t YV, uint32_t YH,
    uint32_t flit_size, uint8_t * MCU_YCbCr, uint8_t * picture,
    uint32_t * LB_X, uint32_t * LB_Y)
{
	uint8_t * CELLS = NULL, * Y_SRC = NULL, * Y_DST = NULL;
	uint8_t * U_SRC = NULL, * U_DST = NULL;
	uint8_t * V_SRC = NULL, * V_DST = NULL;
	uint8_t * uv_line_src = NULL, * uv_line_dst = NULL;

	uint16_t NB_MCU_X = 0, NB_MCU_Y = 0;
	uint16_t NB_CELLS = 0;

	uint32_t flit_bytes = 0;
	uint32_t * y_line_dst = NULL, * y_line_src = NULL;

#ifdef PROGRESS
	char progress_tab[4] = {'/', '-', '\\', '|'};
	uint32_t imageCount = 1;
	uint32_t block_index = 0;
#endif

	flit_bytes = flit_size * MCU_sx * MCU_sy * sizeof (uint8_t);
	NB_MCU_X = intceil(SOF_section.width, MCU_sx);
	NB_MCU_Y = intceil(SOF_section.height, MCU_sy);
	NB_CELLS = YV * YH + 2;

#ifdef PROGRESS
	printf ("Image %lu : |", imageCount++);
	fflush (stdout);
#endif

  for (int flit_index = 0; flit_index < flit_size; flit_index += NB_CELLS)
  {
    CELLS = MCU_INDEX(MCU_YCbCr, flit_index);

    for (int cell_y_index = 0; cell_y_index < YV; cell_y_index += 1)
    {
      for (int cell_x_index = 0; cell_x_index < YH; cell_x_index += 1)
      {
        Y_SRC = MCU_INDEX(CELLS, (YH * cell_y_index + cell_x_index));
        Y_DST = FB_Y_INDEX(picture, *LB_X + cell_x_index, *LB_Y + cell_y_index);

        for (int line_index = 0; line_index < MCU_sy; line_index += 1)
        {
          y_line_src = (uint32_t *) MCU_LINE(Y_SRC, line_index);
          y_line_dst = (uint32_t *) FB_Y_LINE(Y_DST, line_index);
          *y_line_dst++ = *y_line_src++; *y_line_dst++ = *y_line_src++;
        }
      }

      U_SRC = MCU_INDEX(CELLS, (YH * YV));
      U_DST = FB_U_INDEX(picture, *LB_X, *LB_Y + cell_y_index);

      for (int line_index = 0; line_index < MCU_sy; line_index += 1)
      {
        uv_line_src = MCU_LINE(U_SRC, line_index);
        uv_line_dst = FB_UV_LINE(U_DST, line_index);

        for (int i = 0; i < (MCU_sx / (3 - YH)); i += 1)
        {
          uv_line_dst[i] = uv_line_src[i * (3 - YH)];
        }
      }

      V_SRC = MCU_INDEX(CELLS, (YH * YV + 1));
      V_DST = FB_V_INDEX(picture, *LB_X, *LB_Y + cell_y_index);

      for (int line_index = 0; line_index < MCU_sy; line_index += 1)
      {
        uv_line_src = MCU_LINE(V_SRC, line_index);
        uv_line_dst = FB_UV_LINE(V_DST, line_index);

        for (int i = 0; i < (MCU_sx / (3 - YH)); i += 1)
        {
          uv_line_dst[i] = uv_line_src[i * (3 - YH)];
        }
      }
    }

    *LB_X = (*LB_X + YH) % NB_MCU_X;

#ifdef PROGRESS
    fputs ("\033[1D", stdout);
    putchar (progress_tab[block_index++ % 4]);
    fflush (stdout);
#endif

    if (*LB_X == 0)
    {
      *LB_Y = (*LB_Y + YV) % NB_MCU_Y;

      if (*LB_Y == 0) 
      {
        /*
         * TODO: Send the picture to someone !
         */

#ifdef PROGRESS
        puts ("\033[1Ddone");
#endif

#ifdef PROGRESS
        printf ("Image %lu : |", imageCount++);
        fflush (stdout);
#endif
      }
    }
  }
}
