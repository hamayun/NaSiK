/*************************************************************************************
 * File   : utils.h,     
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Patrice, GERIN patrice.gerin@imag.fr
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

#ifndef __UTILS_H__
#define __UTILS_H__

#include "stdio.h"
#include "assertion.h"

#define OPTIONAL_ATTRIBUTE true
#define REQUIRED_ATTRIBUTE false

#define GET_ATTRIBUTE(attr_name, attr_ptr,type,optional) \
{ \
	sc_attribute< type >* tmp_attr = (sc_attribute< type >*)get_attribute(attr_name); \
	if(!optional) \
	{ \
		ASSERT_MSG(tmp_attr != NULL, "missing attribute \"" attr_name "\""); \
	} \
	if(tmp_attr != NULL) \
	{ \
		attr_ptr = tmp_attr; \
	} \
} 

#endif				// __UTILS_H__
