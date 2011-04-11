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
#include <iomanip>

using namespace native;
using namespace std; 

generic_noc::generic_noc(sc_module_name name)
	: device_master_slave(name),
	_depth(NULL),
  _start_of_simulation(false),
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

std::vector< mapping::segment_t * > * generic_noc::get_mapping()
{
  start_of_simulation();
  return(&_segments);
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

  if(_start_of_simulation == true) {return;}
  _start_of_simulation = true;
	/* Build and sort the list of portmap */
	cerr << name() << ": Build memory mapping" << std::endl;
	for( port_index = 0 ; port_index < (unsigned int)p_io.size(); port_index++)
	{
		slave_mapping = p_io[port_index]->get_mapping();

		cerr << "\tSlave port " << port_index << std::endl;
		for( map_index = 0 ; map_index < slave_mapping->size(); map_index++)
		{
			cerr << "\t\tsegment " << (*slave_mapping)[map_index]->name << std::endl;
      _segments.push_back((*slave_mapping)[map_index]);
			portmap_ptr = new portmap_desc_t;
			portmap_ptr->map = (*slave_mapping)[map_index];
			portmap_ptr->port = port_index;
			portmap_ptr->ready = true;
			portmap_ptr->current_event = 0;
			portmap_ptr->events = new sc_event[p_io.size()];
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
//		DOUT << portmap_ptr->map->name << ": "
//		     << std::hex << portmap_ptr->map->base_addr << " - "
//		     << std::hex << portmap_ptr->map->end_addr 
//				 << std::hex << " mapped to port " << (portmap_ptr)->port << std::endl;
	}

	/* Check the mapping */
	for( port_index = 0 ; port_index < (_portmap.size()-1); port_index++)
	{
		cerr << setfill(' ') << setw(15) << _portmap[port_index]->map->name << ": "
		     << "0x" << setfill('0') << setw(8) << std::hex << _portmap[port_index]->map->base_addr << " - "
		     << "0x" << setfill('0') << setw(8) << std::hex << _portmap[port_index]->map->end_addr 
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
                            DOUT_NAME << " decode port = " << _portmap[midle] -> port << std::endl;
                            return(_portmap[midle]);
			}
		}
	}

        std::cerr << " Decoding Failed addr = 0x" << std::hex << addr << std::endl;
	ASSERT_MSG( false , "Invalide address");
	return(NULL);
}

void generic_noc::lock(portmap_desc_t * pm)
{
  sc_event *ev;
	while(_concurrent_com >= _depth->value)
  {
    DOUT_NAME << " wait on _depth_event" << std::endl;
		wait(_depth_event);
  }
	_concurrent_com++;

	while(pm->ready == false)
  {
    DOUT_NAME << " wait on port " << pm->port << std::endl;
    ev = &pm->events[pm->current_event];
    pm->events_list.push_back(ev);
    pm->current_event = (pm->current_event + 1) % p_io.size();
		wait(*ev);
  }

  DOUT_NAME << " lock port " << pm->port << std::endl;
	pm->ready = false;

}

inline void generic_noc::unlock(portmap_desc_t * pm)
{
  sc_event *ev;
	pm->ready = true;
	_concurrent_com--;

  DOUT_NAME << " unlock port " << pm->port << std::endl;
	_depth_event.notify();
  if(!(pm->events_list.empty()))
  {
  ev = pm->events_list.front();
  pm->events_list.pop_front();
	ev->notify();
  }
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

    DOUT_NAME << " load_linked A @" << addr << " id = " << id << std::endl;

    portmap_ptr = decode((uintptr_t)addr);

    lock(portmap_ptr);
    portmap_ptr->linked_map[(uint32_t)addr] = id;
    mst_read(addr,&data,portmap_ptr->port);
    unlock(portmap_ptr);

    DOUT_NAME << " load_linked B @" << addr << " loaded data = " << data << std::endl;
    return(data);
}


bool generic_noc::store_cond (uint32_t *addr, uint32_t data, uint32_t id)
{
    bool ret = true;
    portmap_desc_t * portmap_ptr;

    portmap_ptr = decode((uintptr_t)addr);

    DOUT_NAME << " store_cond A @" << addr << " id = " << id
              << " (linked = " << (*(portmap_ptr->linked_map.find((uint32_t)addr))).second << ")" << std::endl;

    lock(portmap_ptr);

    if( (*(portmap_ptr->linked_map.find((uint32_t)addr))).second == id)
    {
        portmap_ptr->linked_map.erase((uint32_t)addr);
        DOUT_NAME << " store_cond B @" << addr << " data stored = " << data << " OK " << std::endl;
        mst_write(addr,data,portmap_ptr->port);
        ret = false;
    }
    unlock(portmap_ptr);

    DOUT_NAME << " store_cond C @" << addr << " ret = " << ret << std::endl;
    return(ret);
}


