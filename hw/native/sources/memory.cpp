/*************************************************************************************
 * File   : memory.cpp,     
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

#define DEBUG_OPTION "memory"

#include "memory.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"

using namespace native;

memory::memory(sc_module_name name)
  : device_slave(name),
		p_linker_loader("p_linker_loader"),
    _sections(NULL),
    _base_addr(NULL)
{
  _generic_name = "Memory";
	DOUT_CTOR << this->name() << std::endl;
}

memory::~memory()
{
	DOUT_DTOR << this->name() << std::endl;
}

void memory::end_of_elaboration()
{
	char * token;
	symbol< uintptr_t > * symbol_ptr;

	device_slave::end_of_elaboration();

	GET_ATTRIBUTE("SECTIONS",_sections,char*,false);
	GET_ATTRIBUTE("BASE_ADDR",_base_addr,char*,OPTIONAL_ATTRIBUTE);

	token = strtok(_sections->value, " ");
	while(token != NULL)
	{
		DOUT << name() << " map section " << token << std::endl;
		_segments.push_back(p_linker_loader->get_section(token));
		token = strtok(NULL, " ");
	}

  if(_base_addr != NULL)
  {
    uint32_t index = 0;

    token = strtok(_base_addr->value, " ");
    while(token != NULL)
    {
      DOUT << name() << " Symbol " << token << std::endl;
      symbol_ptr = new symbol< uintptr_t >(token);
      symbol_ptr->push_back(_segments[index++]->base_addr);
      _symbols.push_back(symbol_ptr);
      token = strtok(NULL, " ");
    }
  }
}

void memory::slv_read (uint8_t  *addr, uint8_t  *data)
{
	*data = *addr;
}

void memory::slv_read (uint16_t  *addr, uint16_t *data)
{
	*data = *addr;
}

void memory::slv_read (uint32_t  *addr, uint32_t *data)
{
	*data = *addr;
}

void memory::slv_write (uint8_t  *addr, uint8_t  data)
{
	*addr = data;
}

void memory::slv_write (uint16_t  *addr, uint16_t data)
{
	*addr = data;
}

void memory::slv_write (uint32_t  *addr, uint32_t data)
{
	*addr = data;
}

