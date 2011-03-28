/*************************************************************************************
 * File   : devices.cpp,     
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

#define DEBUG_OPTION "device"

#include "devices.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "symbol.h"

using namespace native;
using namespace mapping;

  dev_channel::dev_channel(sc_module_name name)
: sc_module(name),
  _address(0),
  _data(0),
  _width(32),
  _op(ACCESS_NONE)
{
  exp_io(*this);
}

dev_channel::~dev_channel()
{
}

std::vector< mapping::segment_t * > * dev_channel::get_mapping()
{
  return(p_io->get_mapping());
}


void dev_channel::read (uint8_t  *addr, uint8_t  *data)
{
  SET_BUS_ACCESS(addr,0,8,ACCESS_READ);
  p_io->read(addr,data);
  SET_BUS_ACCESS(addr,*data,8,ACCESS_NONE);
}

void dev_channel::read (uint16_t  *addr, uint16_t  *data)
{
  SET_BUS_ACCESS(addr,0,16,ACCESS_READ);
  p_io->read(addr,data);
  SET_BUS_ACCESS(addr,*data,16,ACCESS_NONE);
}

void dev_channel::read (uint32_t  *addr, uint32_t  *data)
{
  SET_BUS_ACCESS(addr,0,32,ACCESS_READ);
  p_io->read(addr,data);
  SET_BUS_ACCESS(addr,*data,32,ACCESS_NONE);
}

void dev_channel::read (uint64_t  *addr, uint64_t  *data)
{
  SET_BUS_ACCESS(addr,0,64,ACCESS_READ);
  p_io->read(addr,data);
  SET_BUS_ACCESS(addr,*data,64,ACCESS_NONE);
}

void dev_channel::write (uint8_t  *addr, uint8_t  data)
{
  SET_BUS_ACCESS(addr,data,8,ACCESS_WRITE);
  p_io->write(addr,data);
  SET_BUS_ACCESS(addr,data,32,ACCESS_NONE);
}

void dev_channel::write (uint16_t  *addr, uint16_t  data)
{
  SET_BUS_ACCESS(addr,data,16,ACCESS_WRITE);
  p_io->write(addr,data);
  SET_BUS_ACCESS(addr,data,16,ACCESS_NONE);
}

void dev_channel::write (uint32_t  *addr, uint32_t  data)
{
  SET_BUS_ACCESS(addr,data,32,ACCESS_WRITE);
  p_io->write(addr,data);
  SET_BUS_ACCESS(addr,data,32,ACCESS_NONE);
}

void dev_channel::write (uint64_t  *addr, uint64_t  data)
{
  SET_BUS_ACCESS(addr,data,64,ACCESS_WRITE);
  p_io->write(addr,data);
  SET_BUS_ACCESS(addr,data,64,ACCESS_NONE);
}

uint32_t dev_channel::load_linked (uint32_t *addr, uint32_t id)
{
  uint32_t ret;
  SET_BUS_ACCESS(addr,0,64,ACCESS_READ);
  ret = p_io->load_linked(addr,id);
  SET_BUS_ACCESS(addr,ret,64,ACCESS_NONE);
  return(ret);
}


bool dev_channel::store_cond (uint32_t *addr, uint32_t data,uint32_t id)
{
  bool ret;
  SET_BUS_ACCESS(addr,data,32,ACCESS_WRITE);
  ret = p_io->store_cond(addr,data,id);
  SET_BUS_ACCESS(addr,data,32,ACCESS_NONE);
  return(ret);
}

