/*************************************************************************************
 * File   : soclib_tg.cpp,     
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

#define DEBUG_OPTION "soclib_tg"

#include "soclib_tg.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "symbol.h"
#include "errno.h"

using namespace native;
using namespace mapping;

soclib_tg::soclib_tg(sc_module_name name)
  : device_slave(name)
{
  _generic_name = "Traffic Generator";
	DOUT_CTOR << this->name() << std::endl;
}

soclib_tg::~soclib_tg()
{
	DOUT_DTOR << this->name() << std::endl;
}

void soclib_tg::end_of_elaboration()
{
	device_slave::end_of_elaboration();

	GET_ATTRIBUTE("FILE",_file,char*,false);
	DOUT << name() << ": FILE = " << _file->value << std::endl;

  _file_id = fopen(_file->value,"r");

	_segments.push_back(mapping::init(name(), sizeof(uint32_t)));
	_registers.reg32 = (uint32_t*)_segments[0]->base_addr;

	symbol<uintptr_t> * symbol_ptr;
	symbol_ptr = new symbol<uintptr_t>("SOCLIB_TG_DEVICE");
  symbol_ptr->push_back(_registers.ptr);
	_symbols.push_back(symbol_ptr);

}

void soclib_tg::slv_read (uint32_t  *addr, uint32_t  *data)
{
	switch(REL_ADDR(addr) & 0x3)
	{
		case 0x00:
			{
        fread((void*)data, 1, sizeof(uint32_t), _file_id);
				break;
			}
		default :
			ASSERT_MSG( false, "Invalide address");
	}
}


