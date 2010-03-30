/*************************************************************************************
 * File   : generic_noc.h,     
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

#ifndef __GENERIC_NOC_H__
#define __GENERIC_NOC_H__

#include "systemc.h"
#include "mapping.h"
#include "interfaces/io.h"
#include "devices.h"
#include <vector>
#include <list>

#define NB_MAX_PORTS     256

namespace libta {

class generic_noc:
		public device_master_slave
{
	public:
	generic_noc(sc_module_name name);
	~generic_noc();

	void end_of_elaboration();
	void start_of_simulation();

	void slv_read (uint8_t  *addr, uint8_t  *data);
	void slv_read (uint16_t *addr, uint16_t *data);
	void slv_read (uint32_t *addr, uint32_t *data);
             
	void slv_write (uint8_t  *addr, uint8_t  data);
	void slv_write (uint16_t *addr, uint16_t data);
	void slv_write (uint32_t *addr, uint32_t data);

       uint32_t load_linked (uint32_t *addr, uint32_t id);
       bool store_cond (uint32_t *addr, uint32_t data, uint32_t id);

	private:

	// Parameters
	sc_attribute < uint32_t >*_depth;

	typedef struct {
		mapping::segment_t   *map;
		unsigned int                port;
		bool                           ready;
		sc_event                    event;
              std::list< uint32_t >    linked_id;
	} portmap_desc_t;
	static bool compare(portmap_desc_t *pm1, portmap_desc_t *pm2);

	std::vector< mapping::segment_t * > _memory_map;
	std::vector< portmap_desc_t * > _portmap;

	uint32_t _concurrent_com;
	sc_event _depth_event;

	private:
	bool   compare_mapping(generic_noc::portmap_desc_t *pm1, generic_noc::portmap_desc_t *pm2);
	inline portmap_desc_t * decode(uintptr_t addr);
	inline void lock(portmap_desc_t * pm);
	inline void unlock(portmap_desc_t * pm);

};

}

#endif				// __GENERIC_NOC_H__
