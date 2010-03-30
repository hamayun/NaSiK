/*************************************************************************************
 * File   : generic_noc.cpp,     
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

#define DEBUG_OPTION "generic_noc"

#include "generic_noc.h"
#include "utils.h"
#include "assertion.h"
#include "debug.h"
#include <list>
#include <vector>

using namespace libta;

generic_noc::generic_noc(sc_module_name name)
	: device_master_slave(name),
	_depth(NULL),
	_concurrent_com(0)
{
	DOUT_CTOR << this->name() << std::endl;
}

generic_noc::~generic_noc()
{
	DOUT_CTOR << this->name() << std::endl;
}

void generic_noc::end_of_elaboration()
{
	device_master_slave::end_of_elaboration();

	GET_ATTRIBUTE("DEPTH",_depth,uint32_t,false);
	DOUT << name() << ": DEPTH = " << _depth->value << std::endl;
}

bool generic_noc::compare(portmap_desc_t *pm1, portmap_desc_t *pm2)
{
	return(mapping::compare(pm1->map,pm2->map));
}

void generic_noc::start_of_simulation()
{
	unsigned int port_index;
	unsigned int map_index;
	std::vector< mapping::segment_t * > *slave_mapping;
	std::list< portmap_desc_t * > portmap_list;
	std::list< portmap_desc_t * >::iterator portmap_list_it;
	portmap_desc_t * portmap_ptr;
       //setDebugOption("generic_noc");

	/* Build and sort the list of portmap */
	DOUT << name() << ": Build memory mapping" << std::endl;
	for( port_index = 0 ; port_index < (unsigned int)p_io.size(); port_index++)             // Iterate through all of the Ports 
	{
		slave_mapping = p_io[port_index]->get_mapping();

		DOUT << "\tSlave port " << port_index << std::endl;
		for( map_index = 0 ; map_index < slave_mapping->size(); map_index++)        // Iterate through all of the Segments 
		{
			DOUT << "\t\tsegment " << (*slave_mapping)[map_index]->name << std::endl;
			portmap_ptr = new portmap_desc_t;
			portmap_ptr->map = (*slave_mapping)[map_index];
			portmap_ptr->port = port_index;
			portmap_ptr->ready = true;
			portmap_list.push_back(portmap_ptr);
		}
	}
	portmap_list.sort(compare);

	/* Convert list to vector */
	while(portmap_list.empty() == false)
	{
		portmap_ptr = portmap_list.front();
		_portmap.push_back(portmap_ptr);
		portmap_list.pop_front();
		DOUT << portmap_ptr->map->name << ": "
		     << std::hex << portmap_ptr->map->base_addr << " - "
		     << std::hex << portmap_ptr->map->end_addr 
				 << std::hex << " mapped to port " << (portmap_ptr)->port << std::endl;
	}

	/* Check the mapping */
	for( port_index = 0 ; port_index < (_portmap.size()-1); port_index++)
	{
		DOUT << _portmap[port_index]->map->name << ": "
		     << std::hex << _portmap[port_index]->map->base_addr << " - "
		     << std::hex << _portmap[port_index]->map->end_addr 
				 << std::hex << " mapped to port " << _portmap[port_index]->port << std::endl;

		ASSERT_MSG ( ( _portmap[port_index]->map->end_addr < _portmap[port_index+1]->map->base_addr ), "Inconsistent mapping");
	}

}

inline generic_noc::portmap_desc_t * generic_noc::decode(uintptr_t addr)
{
	int midle;
	int low;
	int high;

	low = 0;
	high = _portmap.size();

	while( low <= high )
	{
		midle = (low + high) / 2;
		if(addr < _portmap[midle]->map->base_addr)
		{
			high = midle - 1;
		}
		else
		{
			if(addr > _portmap[midle]->map->end_addr)
			{
				low = midle + 1;
			}
			else
			{
                            DOUT_NAME << " decode = " << _portmap[midle] -> port << std::endl;
				return(_portmap[midle]);
			}
		}
	}
	ASSERT_MSG( false , "Invalide address");
	return(NULL);
}

void generic_noc::lock(portmap_desc_t * pm)
{
	while(_concurrent_com >= _depth->value)
      {
            DOUT_NAME << " wait on _depth_event" << std::endl;
    	     wait(_depth_event);
      }
	_concurrent_com++;

	while(pm->ready == false)
      {
            DOUT_NAME << " wait on port " << pm->port << std::endl;
	     wait(pm->event);
      }

       DOUT_NAME << " lock port " << pm->port << std::endl;
	pm->ready = false;

}

inline void generic_noc::unlock(portmap_desc_t * pm)
{
	pm->ready = true;
	_concurrent_com--;

       DOUT_NAME << " unlock port " << pm->port << std::endl;
	_depth_event.notify();
	pm->event.notify();
}


void generic_noc::slv_read (uint8_t  *addr, uint8_t  *data)
{
	portmap_desc_t * portmap_ptr;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);
	mst_read(addr,data,portmap_ptr->port);
	unlock(portmap_ptr);
}

void generic_noc::slv_read (uint16_t  *addr, uint16_t  *data)
{
	portmap_desc_t * portmap_ptr;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);
	mst_read(addr,data,portmap_ptr->port);
	unlock(portmap_ptr);
}

void generic_noc::slv_read (uint32_t  *addr, uint32_t  *data)
{
	portmap_desc_t * portmap_ptr;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);
	mst_read(addr,data,portmap_ptr->port);
	unlock(portmap_ptr);
}

void generic_noc::slv_write (uint8_t  *addr, uint8_t  data)
{
	portmap_desc_t * portmap_ptr;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);
	mst_write(addr,data,portmap_ptr->port);
	unlock(portmap_ptr);
}

void generic_noc::slv_write (uint16_t *addr, uint16_t data)
{
	portmap_desc_t * portmap_ptr;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);
	mst_write(addr,data,portmap_ptr->port);
	unlock(portmap_ptr);
}

void generic_noc::slv_write (uint32_t *addr, uint32_t data)
{
	portmap_desc_t * portmap_ptr;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);
	mst_write(addr,data,portmap_ptr->port);
	unlock(portmap_ptr);
}

uint32_t generic_noc::load_linked (uint32_t *addr, uint32_t id)
{
	portmap_desc_t * portmap_ptr;
       uint32_t  data;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);
       portmap_ptr->linked_id.push_front(id);
	mst_read(addr,&data,portmap_ptr->port);
	unlock(portmap_ptr);
       return(data);
}


bool generic_noc::store_cond (uint32_t *addr, uint32_t data, uint32_t id)
{
       bool ret = false;
	portmap_desc_t * portmap_ptr;

	portmap_ptr = decode((uintptr_t)addr);

	lock(portmap_ptr);

      while(!portmap_ptr->linked_id.empty())
      {
        if( id == portmap_ptr->linked_id.front()) ret = true;
        portmap_ptr->linked_id.pop_front();
      }

      if(ret)
	  mst_write(addr,data,portmap_ptr->port);

	unlock(portmap_ptr);

      return(ret);
}


