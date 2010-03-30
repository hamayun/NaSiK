/*************************************************************************************
 * File   : assert.h,     
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

#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <iostream>

#ifndef LIBTA_NO_ASSERTION

//      printf("%s:%d : assertion failed in %s - %s\n",__FILE__,__LINE__,__func__, #expression); 
//      printf("ASSERT MESSAGE: %s\n",msg); 

#define ASSERT(expression) \
if(!(expression)) \
{ \
	std::cerr << __FILE__ << ":" << __LINE__ << " : assertion failed in " << __func__ << ", " << #expression << std::endl; \
	*(int *)NULL = 0; \
}

#define ASSERT_MSG(expression, msg) \
if(!(expression)) \
{ \
	std::cerr << __FILE__ << ":" << __LINE__ << " : assertion failed in " << __func__ << ", " << #expression << std::endl; \
	std::cerr << "ASSERT MESSAGE: " << msg << std::endl; \
	*(int *)NULL = 0; \
}

#else

#define ASSERT(expression)

#define ASSERT_MSG(expression, msg) \

#endif				// LIBTA_NO_ASSERTION
#endif				// __ASSERT_H__
