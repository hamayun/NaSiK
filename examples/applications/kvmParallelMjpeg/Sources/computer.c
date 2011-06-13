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

#include <Private/Computer.h>

void computer (uint32_t flit_size, int32_t * block_YCbCr, uint8_t * Idct_YCbCr)
{
  for (uint32_t i = 0; i < flit_size; i++)
  {
    IDCT(& block_YCbCr[i * 64], & Idct_YCbCr[i * 64]);
  }
}

