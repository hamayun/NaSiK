/*************************************************************************************
 * File   : bridgefs.cpp,     
 *
 * Copyright (C) 2008 TIMA Laboratory
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

#define DEBUG_OPTION "bridgefs"

#include "bridgefs.h"
#include "fd_access.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace native
{

using namespace mapping;

bridgefs::bridgefs(sc_module_name name)
  : 
    device_master_slave(name),
    _nb_fdaccess(NULL),
    _irq_config(NULL)
  {
    _generic_name = "BridgeFS";
    SC_THREAD(thread);
    DOUT_CTOR << this->name() << std::endl;
  }

bridgefs::~bridgefs()
{
  DOUT_DTOR << this->name() << std::endl;
}

void bridgefs::end_of_elaboration()
{
  uint32_t    index;
  char        *token;

  device_master_slave::end_of_elaboration();

	GET_ATTRIBUTE("NB_FDACCESS",_nb_fdaccess,uint32_t,false);
	DOUT << name() << ": NB_FDACCESS = " << _nb_fdaccess->value << std::endl;

	GET_ATTRIBUTE("IRQ_CONFIG",_irq_config,char*,false);
	DOUT << name() << ": IRQ_CONFIG = " << _irq_config->value << std::endl;

  _segments.push_back(mapping::init(name(), sizeof(uint32_t) * BRIDGEFS_NB_REG));
  _registers.ptr = _segments[0]->base_addr;

  symbol<uint32_t> * symbol_value;
  symbol_value = new symbol<uint32_t>("SOCLIB_FDACCESS_NDEV");
  symbol_value->push_back(_nb_fdaccess->value);
  _symbols.push_back(symbol_value);

  // fdaccess configuration 
  // SOCLIB_FDACCESS_DEVICES = { fdaccess_0.irq, fdaccess_0.addr, fdaccess_1.irq,  fdaccess_1.addr  ... }
	symbol_value = new symbol<uint32_t>("SOCLIB_FDACCESS_DEVICES");
  token = strtok(_irq_config->value, " ");
  index = 0;
  while(token != NULL)
  {
    symbol_value->push_back((uint32_t)1);
    symbol_value->push_back(strtol(token,NULL,10));
    symbol_value->push_back((uint32_t)&(_registers.reg32[index*BRIDGEFS_NB_REG]));
    index++;
    token = strtok(NULL, " ");
  }
	_symbols.push_back(symbol_value);
  ASSERT_MSG( index == _nb_fdaccess->value, "IRQ_CONFIG doesn't match fdaccess count!!!");

}

void bridgefs::thread()
{
  while(1)
  {
    wait(_op_event);
    switch(_registers.reg32[REG_OP])
    {
      case FD_ACCESS_OPEN : 
        { 
          DOUT_NAME << " FD_ACCESS_OPEN" << std::endl;
          open_file(); 
          break; 
        }
      case FD_ACCESS_READ : 
        { 
          DOUT_NAME << " FD_ACCESS_READ" << std::endl;
          read_file(); 
          break; 
        }
      case FD_ACCESS_LSEEK :
        {
          DOUT_NAME << " FD_ACCESS_LSEEK" << std::endl;
          lseek_file();
          break;
        }
      default:
        ASSERT_MSG(false, "Unsupported FD operation");
    }
    _registers.reg32[REG_OP] = FD_ACCESS_NOOP;
  }
}

void bridgefs::slv_write (uint32_t  *addr, uint32_t  data)
{
  *addr = data;
  switch(REGISTER_INDEX(UINT32,addr))
  {
    case REG_OP:
      {
        _op_event.notify();
        break;
      }
    default:
      break;
  }
  switch(REGISTER_INDEX(UINT32,addr))
  {
    case REG_FD: { DOUT_NAME << "REG_FD" << std::endl; break;}
    case REG_BUFFER: { DOUT_NAME << "REG_BUFFER" << std::endl; break;}
    case REG_SIZE: { DOUT_NAME << "REG_SIZE" << std::endl; break;}
    case REG_HOW: { DOUT_NAME << "REG_HOW" << std::endl; break;}
    case REG_MODE: { DOUT_NAME << "REG_MODE" << std::endl; break;}
    case REG_OP: { DOUT_NAME << "REG_OP" << std::endl; break;}
    case REG_RETVAL: { DOUT_NAME << "REG_RETVAL" << std::endl; break;}
    case REG_ERRNO: { DOUT_NAME << "REG_ERRNO" << std::endl; break;}
    case REG_IRQ_ENABLE: { DOUT_NAME << "REG_IRQ_ENABLE" << std::endl; break;}
    case BRIDGEFS_NB_REG : { DOUT_NAME << "BRIDGEFS_NB_REG " << std::endl; break;}
  }
}

void bridgefs::slv_read (uint32_t  *addr, uint32_t  *data)
{
  *data = *addr;
  switch(REGISTER_INDEX(UINT32,addr))
  {
    case REG_FD: { DOUT_NAME << "REG_FD" << std::endl; break;}
    case REG_BUFFER: { DOUT_NAME << "REG_BUFFER" << std::endl; break;}
    case REG_SIZE: { DOUT_NAME << "REG_SIZE" << std::endl; break;}
    case REG_HOW: { DOUT_NAME << "REG_HOW" << std::endl; break;}
    case REG_MODE: { DOUT_NAME << "REG_MODE" << std::endl; break;}
    case REG_OP: { DOUT_NAME << "REG_OP" << std::endl; break;}
    case REG_RETVAL: { DOUT_NAME << "REG_RETVAL" << std::endl; break;}
    case REG_ERRNO: { DOUT_NAME << "REG_ERRNO" << std::endl; break;}
    case REG_IRQ_ENABLE: { DOUT_NAME << "REG_IRQ_ENABLE" << std::endl; break;}
    case BRIDGEFS_NB_REG : { DOUT_NAME << "BRIDGEFS_NB_REG " << std::endl; break;}
  }
}

void bridgefs::open_file()
{
  char * buffer;
  uint32_t size;
  uint32_t index;
  uint8_t data;
  uint8_t * addr;

  size = _registers.reg32[REG_SIZE];
  addr = (uint8_t*)_registers.reg32[REG_BUFFER];

  buffer = new char[size + 1];

  for(index = 0 ; index < size ; index++, addr++)
  {
    DOUT_NAME << " buffer =  " << buffer << std::endl;
    mst_read(addr,&data);
    buffer[index] = data;
  }
  buffer[index] = 0;

  DOUT_NAME << " open " << buffer << std::endl;
  //_registers.RETVAL = (uint32_t) ::open(buffer,_registers.HOW,_registers.MODE);
  _registers.reg32[REG_RETVAL] = (uint32_t) ::open(buffer,O_RDONLY);
  _registers.reg32[REG_ERRNO] = errno;
  _registers.reg32[REG_OP] = FD_ACCESS_NOOP;
  DOUT_NAME << " open " << buffer << " done ... " << std::endl;

  delete [] buffer;
}

void bridgefs::read_file()
{
  char * buffer;
  uint32_t size;
  uint32_t index;
  uint8_t * addr;

  size = _registers.reg32[REG_SIZE];

  buffer = new char[size + 1];

  addr = (uint8_t*)_registers.reg32[REG_BUFFER];
  _registers.reg32[REG_RETVAL] = (uint32_t) ::read(_registers.reg32[REG_FD],buffer,_registers.reg32[REG_SIZE]);
  _registers.reg32[REG_ERRNO] = errno;
  ASSERT_MSG( (int)_registers.reg32[REG_RETVAL] != -1 , strerror(errno));

  for(index = 0 ; index < size ; index++, addr++)
  {
    mst_write(addr,(uint8_t)buffer[index]);
  }
  delete [] buffer;
}

void bridgefs::lseek_file()
{
  _registers.reg32[REG_RETVAL] = (uint32_t) ::lseek(_registers.reg32[REG_FD], _registers.reg32[REG_SIZE], _registers.reg32[REG_MODE]);
  _registers.reg32[REG_ERRNO] = errno;
  _registers.reg32[REG_OP] = FD_ACCESS_NOOP;
  DOUT_NAME << " lseek " << _registers.reg32[REG_SIZE] << " done ... " << std::endl;

}

}
