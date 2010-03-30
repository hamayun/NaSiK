/*************************************************************************************
 * File   : soclib_timer.cpp,     
 *
 * Copyright (C) 2009 TIMA Laboratory
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

#define DEBUG_OPTION "soclib_timer"

#include "soclib_timer.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "symbol.h"
#include "errno.h"

using namespace libta;
using namespace mapping;

soclib_timer::soclib_timer(sc_module_name name)
  : device_slave(name),
    _nb_timers(NULL),
    _irq_config(NULL)
{
  _generic_name = "TIMER";
	DOUT_CTOR << this->name() << std::endl;
}

soclib_timer::~soclib_timer()
{
	DOUT_DTOR << this->name() << std::endl;
}

void soclib_timer::end_of_elaboration()
{
  uint32_t    index;
  char        *token;

	device_slave::end_of_elaboration();

	GET_ATTRIBUTE("NB_TIMERS",_nb_timers,uint32_t,false);
	DOUT << name() << ": NB_TIMERS = " << _nb_timers->value << std::endl;

	GET_ATTRIBUTE("IRQ_CONFIG",_irq_config,char*,false);
	DOUT << name() << ": IRQ_CONFIG = " << _irq_config->value << std::endl;

	_segments.push_back(mapping::init(name(), TIMER_SPAN * sizeof(uint32_t)));
	_registers.ptr = _segments[0]->base_addr;

  // Number of timer
	symbol<uint32_t> * symbol_value;
	symbol_value = new symbol<uint32_t>("SOCLIB_TIMER_NDEV");
	symbol_value->push_back(_nb_timers->value);
	_symbols.push_back(symbol_value);

  // Timers configuration 
  // SOCLIB_TIMER_DEVICES = { timer_0.irq, timer_0.addr, timer_1.irq,  timer_1.addr  ... }
	symbol_value = new symbol<uint32_t>("SOCLIB_TIMER_DEVICES");
  token = strtok(_irq_config->value, " ");
  index = 0;
  while(token != NULL)
  {
    symbol_value->push_back(strtol(token,NULL,10));
    symbol_value->push_back((uint32_t)&(_registers.reg32[index*TIMER_SPAN]));
    index++;
    token = strtok(NULL, " ");
  }
	_symbols.push_back(symbol_value);
  ASSERT_MSG( index == _nb_timers->value, "IRQ_CONFIG doesn't match timer count!!!");

}

void soclib_timer::slv_write (uint32_t *addr, uint32_t  data)
{
  DOUT << name() << " data = 0x" << std::hex << (uint32_t)data << " @0x" << std::hex << (uintptr_t)addr << std::endl;
  switch(REGISTER_INDEX(UINT32,addr) & 0x03)
  {
  }
}

void soclib_timer::slv_read (uint32_t  *addr, uint32_t  *data)
{
  switch(REGISTER_INDEX(UINT32,addr) & 0x03)
  {
  }
}


