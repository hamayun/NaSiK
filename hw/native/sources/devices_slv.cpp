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

void device_slave::end_of_elaboration()
{
  device::end_of_elaboration();
  GET_ATTRIBUTE("REQ_LATENCY",_req_latency,uint32_t,OPTIONAL_ATTRIBUTE);
  DOUT << name() << ": REQ_LATENCY = " << (_req_latency == NULL ? 0 : _req_latency->value) << std::endl;
  GET_ATTRIBUTE("RSP_LATENCY",_rsp_latency,uint32_t,OPTIONAL_ATTRIBUTE);
  DOUT << name() << ": RSP_LATENCY = " << (_rsp_latency == NULL ? 0 : _rsp_latency->value) << std::endl;
}

device_slave::device_slave(sc_module_name name)
: device(name),
  _req_latency(NULL),
  _rsp_latency(NULL)
{
  exp_io(*this);
  exp_linker(*this);
}

device_slave::~device_slave()
{
}

std::vector< symbol_base* > * device_slave::get_symbols()
{
  return(&_symbols);
}

std::vector< mapping::segment_t * > * device_slave::get_mapping()
{
  return(&_segments);
}

void device_slave::read (uint8_t  *addr, uint8_t  *data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_read(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

void device_slave::read (uint16_t  *addr, uint16_t  *data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_read(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

void device_slave::read (uint32_t  *addr, uint32_t  *data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_read(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

void device_slave::read (uint64_t  *addr, uint64_t  *data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_read(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

void device_slave::write (uint8_t  *addr, uint8_t  data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_write(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

void device_slave::write (uint16_t  *addr, uint16_t  data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_write(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

void device_slave::write (uint32_t  *addr, uint32_t  data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_write(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

void device_slave::write (uint64_t  *addr, uint64_t  data)
{
  WAIT_LATENCY(_req_latency->value);
  slv_write(addr,data);
  WAIT_LATENCY(_rsp_latency->value);
}

