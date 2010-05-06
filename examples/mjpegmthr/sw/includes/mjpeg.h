/**************************************************************************************
 * File   : mjpeg.h, file for JPEG-JFIF Multi-thread decoder    
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************************/

#ifndef __MJPEG_H__
#define __MJPEG_H__

#include "jpeg.h"

#define NB_IDCT			3

/*****************************
 * Differents levels of print
 ****************************/

#ifdef VERBOSE
#define VPRINTF(format, ...) printf ("[%s] " format, __FUNCTION__, ## __VA_ARGS__)
#else
#define VPRINTF(format, ...)
#endif

#ifdef INFO
#define IPRINTF(format, ...) printf ("[%s] " format, __FUNCTION__, ## __VA_ARGS__)
#else
#define IPRINTF(format, ...)
#endif

#endif
