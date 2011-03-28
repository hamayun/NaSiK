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

device::device(sc_module_name name)
  : sc_module(name), 
  _clock_period(NULL),
  _clock_unit(NULL),
  _generic_name("none")
{
  DOUT_CTOR << this->name() << std::endl;
}

device::~device()
{
  DOUT_DTOR << this->name() << std::endl;
}

void device::end_of_elaboration()
{
  GET_ATTRIBUTE("CLOCK_PERIOD",_clock_period,uint32_t,REQUIRED_ATTRIBUTE);
  DOUT << name() << ": CLOCK_PERIOD = " << _clock_period->value << std::endl;
  GET_ATTRIBUTE("CLOCK_UNIT",_clock_unit,uint32_t,REQUIRED_ATTRIBUTE);
  DOUT << name() << ": CLOCK_UNIT = " << _clock_unit->value << std::endl;
}

