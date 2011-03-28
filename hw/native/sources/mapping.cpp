/*************************************************************************************
 * File   : mapping.cpp,     
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

#define DEBUG_OPTION "mapping"

#include "mapping.h"
#include "assertion.h"
#include "debug.h"

namespace native
{
namespace mapping
{

segment_t * init(const char * str, uint32_t sz)
{
	segment_t * segment_ptr;
	unsigned char * base_ptr;

	base_ptr = new unsigned char[sz];
  segment_ptr = init(str, (uintptr_t)base_ptr, sz);

	return(segment_ptr);
}

segment_t * init(const char * str, uintptr_t base, uint32_t sz)
{
	segment_t * segment_ptr;

	segment_ptr = new segment_t;

	ASSERT(segment_ptr != NULL);
	segment_ptr -> name = strdup(str);
	segment_ptr -> base_addr = base;
	segment_ptr -> size = sz;
	segment_ptr -> end_addr = (uintptr_t)base + (uintptr_t)sz - (uintptr_t)1;
	segment_ptr -> flags = FLAG_WRITE | FLAG_ALLOC | FLAG_EXECINSTR;

	DOUT_FCT << " new segment " << segment_ptr -> name
		        << " from " << std::hex<< segment_ptr -> base_addr
		        << " to " << std::hex<< segment_ptr -> end_addr
						<< " (" << ((sz >> 10) ? ((double)sz / 1024.0) : sz) << ((sz >> 10) ? "Kb" : "b") << ")" << std::endl;

	return(segment_ptr);
}
bool compare(segment_t *s1, segment_t *s2)
{
	if(s1->base_addr < s2->base_addr) return(true);
	return(false);
}

}
}
