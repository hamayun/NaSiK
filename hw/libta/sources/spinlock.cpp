/*************************************************************************************
 * File   : spinlock.cpp,     
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

#define DEBUG_OPTION "spinlock"

#include "spinlock.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"

using namespace libta;

spinlock::spinlock(sc_module_name name)
  : device_slave(name),
		p_linker_loader("p_linker_loader"),
    _section(NULL)
{
  _generic_name = "Spinlock";
	DOUT_CTOR << this->name() << std::endl;
}

spinlock::~spinlock()
{
	DOUT_DTOR << this->name() << std::endl;
}

void spinlock::end_of_elaboration()
{
	mapping::segment_t * segment_ptr;
	symbol< uintptr_t > * symbol_ptr;

	device_slave::end_of_elaboration();

	GET_ATTRIBUTE("SECTION",_section,char*,false);
	DOUT << name() << ": SECTION = " << _section->value << std::endl;

	DOUT << name() << " map section " << _section->value << std::endl;

	segment_ptr = p_linker_loader->get_section(_section->value); 
	_segments.push_back(segment_ptr);
	symbol_ptr = new symbol< uintptr_t >("PLATFORM_SPIN_ARRAY");
	symbol_ptr->push_back(segment_ptr->base_addr);
	_symbols.push_back(symbol_ptr);

  memset((void*)segment_ptr->base_addr, 0x00, segment_ptr->size);
}

void spinlock::slv_read (uint32_t  *addr, uint32_t *data)
{
	DOUT_FCT << name() << std::endl;
	*data = *addr;
	*addr = 1;
}

void spinlock::slv_write (uint32_t  *addr, uint32_t data)
{
	DOUT_FCT << name() << std::endl;
	if(data == 0)
		*addr = data;
}

