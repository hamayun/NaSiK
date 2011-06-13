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

#ifndef DISPATCH_H
#define DISPATCH_H

#include <stdint.h>
#include <Private/Mjpeg.h>
#include <KahnProcessNetwork/KahnProcessNetwork.h>

/*
 * Define the thread signature
 */

extern void builder ( SOF_section_t SOF_section, uint32_t YV,
    uint32_t YH, uint32_t flit_size, uint8_t * MCU_YCbCr,
    uint8_t * picture, uint32_t * LB_X, uint32_t * LB_Y);

/*
 * Define useful macro to deal with the picture format
 */

#define MCU_INDEX(ptr, index) (ptr + ((index) * MCU_sx * MCU_sy))
#define MCU_LINE(ptr,n) (ptr + ((n) * MCU_sx))

#define FB_Y_LINE(ptr,n) (ptr + ((n) * MCU_sx * NB_MCU_X))
#define FB_Y_INDEX(ptr,x,y) \
  (ptr + ((y) * MCU_sy * MCU_sx * NB_MCU_X) + ((x) * MCU_sx))

#define FB_UV_LINE(ptr,n) (ptr + (((n) * MCU_sx * NB_MCU_X) >> 1))
#define FB_U_INDEX(ptr,x,y) \
  (ptr + (((y) * MCU_sy * SOF_section . width) >> 1) +  \
   (((x) * MCU_sx >> 1)) + (SOF_section . width * SOF_section . height))

#define FB_V_INDEX(ptr,x,y) \
  (ptr + (((y) * MCU_sy * SOF_section . width >> 1)) + (((x) * MCU_sx >> 1))  \
   + ((SOF_section . width * SOF_section . height * 3) >> 1))

#endif
