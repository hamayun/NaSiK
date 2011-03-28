/*************************************************************************************
 * File   : bridge.h,     
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

#ifndef __BRIDGE_H__
#define __BRIDGE_H__

#include "systemc.h"
#include "mapping.h"
#include "interfaces/io.h"
#include "devices.h"
#include <vector>
#include <list>
#include <map>

#define NB_MAX_PORTS     256

namespace native {

class bridge:
		public device_master_slave
{
	public:
	bridge(sc_module_name name);
	~bridge();

	void end_of_elaboration();

	void slv_read (uint8_t  *addr, uint8_t  *data);
	void slv_read (uint16_t *addr, uint16_t *data);
	void slv_read (uint32_t *addr, uint32_t *data);
             
	void slv_write (uint8_t  *addr, uint8_t  data);
	void slv_write (uint16_t *addr, uint16_t data);
	void slv_write (uint32_t *addr, uint32_t data);

  std::vector< mapping::segment_t * > * get_mapping();

};

}

#endif				// __BRIDGE_H__
