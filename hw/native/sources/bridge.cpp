/*************************************************************************************
 * File   : bridge.cpp,     
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

#define DEBUG_OPTION "bridge"

#include "bridge.h"
#include "utils.h"
#include "assertion.h"
#include "debug.h"

using namespace native;

bridge::bridge(sc_module_name name)
	: device_master_slave(name)
{
	DOUT_CTOR << this->name() << std::endl;
}

bridge::~bridge()
{
	DOUT_CTOR << this->name() << std::endl;
}

void bridge::end_of_elaboration()
{
	device_master_slave::end_of_elaboration();
}

std::vector< mapping::segment_t * > * bridge::get_mapping()
{
  return(p_io[0]->get_mapping());
}

void bridge::slv_read (uint8_t  *addr, uint8_t  *data)
{
	mst_read(addr,data,0);
}

void bridge::slv_read (uint16_t  *addr, uint16_t  *data)
{
	mst_read(addr,data,0);
}

void bridge::slv_read (uint32_t  *addr, uint32_t  *data)
{
	mst_read(addr,data,0);
}

void bridge::slv_write (uint8_t  *addr, uint8_t  data)
{
	mst_write(addr,data,0);
}

void bridge::slv_write (uint16_t *addr, uint16_t data)
{
	mst_write(addr,data,0);
}

void bridge::slv_write (uint32_t *addr, uint32_t data)
{
	mst_write(addr,data,0);
}
