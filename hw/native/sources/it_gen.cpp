/*************************************************************************************
 * File   : it_gen.cpp,     
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

#define DEBUG_OPTION "it_gen"

#include "it_gen.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "symbol.h"

using namespace native;
using namespace mapping;

it_gen::it_gen(sc_module_name name)
  : device_slave(name),
    _nb_cycles(NULL)
{
  _generic_name = "Interrupt Generator";
	DOUT_CTOR << this->name() << std::endl;

  SC_THREAD(thread);
}

it_gen::~it_gen()
{
	DOUT_DTOR << this->name() << std::endl;
}

void it_gen::end_of_elaboration()
{
	device_slave::end_of_elaboration();

	GET_ATTRIBUTE("NB_CYCLES",_nb_cycles,uint32_t,false);
	DOUT << name() << " NB_CYCLES: = " << _nb_cycles->value << std::endl;

	_segments.push_back(mapping::init(name(), IT_SPAN * sizeof(uint32_t)));
	_registers.ptr = _segments[0]->base_addr;

  // TTY configuration 
  // SOCLIB_TTY_DEVICES = { tty_0.irq, tty_0.addr, tty_1.irq,  tty_1.addr  ... }
  symbol<uint32_t> * symbol_value;
	symbol_value = new symbol<uint32_t>("IT_GEN_DEVICE");
  symbol_value->push_back((uint32_t)_registers.ptr);
	_symbols.push_back(symbol_value);

}

void it_gen::thread()
{
  sc_time   tick = sc_time((double)(_clock_period->value * _nb_cycles->value), _clock_unit->value);
  bool stop_tick = false;

  while(1)
  {
    wait(_wake_up);
    DOUT << name() << " WAKE UP !!! " << std::endl;
    stop_tick = false;
    while(!stop_tick)
    {
      if(_registers.reg32[IT_START] == 0)
      {
        stop_tick = true;
      }
      else
      {
        DOUT << name() << " INTERRUPT !!! " << std::endl;
        p_it->it_set(0);
      }
      wait(tick,_wake_up);
    }
  }
}

void it_gen::slv_write (uint32_t *addr, uint32_t  data)
{
	DOUT << name() << " data = 0x" << std::hex << (uint32_t)data << " @0x" << std::hex << (uintptr_t)addr << std::endl;
	switch(REGISTER_INDEX(UINT32,addr) & 0x03)
	{
		case IT_START:
      {	
        *addr = data;
        _wake_up.notify();
				break;
			}
		default :
			ASSERT_MSG( false, "Invalide address");
	}
}

void it_gen::slv_read (uint32_t  *addr, uint32_t  *data)
{
  *data = *addr;
}


