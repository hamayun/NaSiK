/*************************************************************************************
 * File   : symbol.cpp,     
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

#define DEBUG_OPTION "symbol_link"

#include "symbol.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "dlfcn.h"

namespace libta {

template<>
bool symbol<uint32_t>::link(void * sw_image)
{
	uint32_t* symbol_ptr;
	unsigned int index;

	DOUT_FCT << _name << std::endl;

        symbol_ptr = (uint32_t*)dlsym(sw_image,_name.c_str());
	ASSERT_MSG(symbol_ptr != NULL, _name);

	for(index = 0 ; index < _values.size() ; index++)
	{
                symbol_ptr[index] = _values[index];
		DOUT << std::hex << &(symbol_ptr[index]) << " = 0x" << std::hex << _values[index] << std::endl;
        }

	return(false);
}

template<>
bool symbol<uint64_t>::link(void * sw_image)
{
	uint64_t* symbol_ptr;
	unsigned int index;

	DOUT_FCT << _name << std::endl;

	symbol_ptr = (uint64_t*)dlsym(sw_image,_name.c_str());
	ASSERT_MSG(symbol_ptr != NULL, _name);

	for(index = 0 ; index < _values.size() ; index++)
	{
		symbol_ptr[index] = _values[index];
		DOUT << std::hex << &(symbol_ptr[index]) << " = 0x" << std::hex << _values[index] << std::endl;
	}

	return(false);
}

}

