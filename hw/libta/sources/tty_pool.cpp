/*************************************************************************************
 * File   : tty_pool.cpp,     
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

#define DEBUG_OPTION "tty_pool"

#include "tty_pool.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include <unistd.h>
#include "symbol.h"
#include "errno.h"

#define READ_PIPE  0
#define WRITE_PIPE 1

using namespace libta;
using namespace mapping;

tty_pool::tty_pool(sc_module_name name)
  : device_slave(name),
    _nb_tty(NULL)
{
  _generic_name = "TTY pool";
	DOUT_CTOR << this->name() << std::endl;
}

tty_pool::~tty_pool()
{
	DOUT_DTOR << this->name() << std::endl;
}

void tty_pool::end_of_elaboration()
{
	char buffer[16];
	int pipes[2];
	unsigned int pid;
  uint32_t index;
  char *token;

	device_slave::end_of_elaboration();

	GET_ATTRIBUTE("NB_TTY",_nb_tty,uint32_t,false);
	DOUT << name() << ": NB_TTY = " << _nb_tty->value << std::endl;

	GET_ATTRIBUTE("IRQ_CONFIG",_irq_config,char*,false);
	DOUT << name() << ": IRQ_CONFIG = " << _irq_config->value << std::endl;

	_segments.push_back(mapping::init(name(), _nb_tty->value * TTY_SPAN * sizeof(uint32_t)));
	_registers.ptr = _segments[0]->base_addr;

	symbol<uint32_t> * symbol_value;
	symbol_value = new symbol<uint32_t>("SOCLIB_TTY_NDEV");
	symbol_value->push_back(_nb_tty->value);
	_symbols.push_back(symbol_value);

  // TTY configuration 
  // SOCLIB_TTY_DEVICES = { tty_0.irq, tty_0.addr, tty_1.irq,  tty_1.addr  ... }
	symbol_value = new symbol<uint32_t>("SOCLIB_TTY_DEVICES");
  token = strtok(_irq_config->value, " ");
  index = 0;
  while(token != NULL)
  {
    symbol_value->push_back(strtol(token,NULL,10));
    symbol_value->push_back((uint32_t)&(_registers.reg32[index*TTY_SPAN]));
    index++;
    token = strtok(NULL, " ");
  }
	_symbols.push_back(symbol_value);
  ASSERT_MSG( index == _nb_tty->value, "IRQ_CONFIG doesn't match tty count!!!");

	for(uint32_t i = 0 ; i < _nb_tty->value ; i++)
	{
		pipe(pipes); // pipes[0] = read ; pipes[1] = write
		_registers.reg32[i*TTY_SPAN + TTY_WRITE] = (uint32_t)pipes[1];
		_registers.reg32[i*TTY_SPAN + TTY_READ] = (uint32_t)pipes[0];
		sprintf(buffer,"%d",pipes[0]);
		if (!(pid = fork())) {
			if (execlp("xterm","xterm","-sb","-sl","1000","-e","tty_term",
						buffer,
						NULL) == -1) {
				ASSERT_MSG( false , strerror(errno));
				exit(1);
			}
		}
	}
}

void tty_pool::slv_write (uint8_t *addr, uint8_t  data)
{
	DOUT << name() << " data = 0x" << std::hex << (uint32_t)data << " @0x" << std::hex << (uintptr_t)addr << std::endl;
	switch(REGISTER_INDEX(UINT32,addr) & 0x03)
	{
		case TTY_WRITE:
      {	
				::write(*(addr),&data,1);
				break;
			}
		case TTY_STATUS:
			{
				ASSERT_MSG( false, "TTY_STATUS readonly register");
				break;
			}
		case TTY_READ:
			{
				ASSERT_MSG( false, "TTY_READ readonly register");
				break;
			}
		default :
			ASSERT_MSG( false, "Invalide address");
	}
}

void tty_pool::slv_read (uint8_t  *addr, uint8_t  *data)
{
	switch(REGISTER_INDEX(UINT32,addr) & 0x03)
	{
		case TTY_WRITE:
			{
				::write(*(addr),&data,1);
				break;
			}
		case TTY_STATUS:
			{
				ASSERT_MSG( false, "TTY_STATUS access not implemented");
				break;
			}
		case TTY_READ:
			{
				ASSERT_MSG( false, "TTY_READ access not implemented");
				break;
			}
		default :
			ASSERT_MSG( false, "Invalide address");
	}
}


