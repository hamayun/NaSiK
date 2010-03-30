/*************************************************************************************
 * File   : eu_base.cpp,     
 *
 * Copyright (C) 2009 TIMA Laboratory
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

#define DEBUG_OPTION "eu_base"

#include "base/proc/eu_base.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "errno.h"

namespace libta
{
  void *     eu_base::_proccess_handle[MAX_EU];
  eu_base *       eu_base::_eu[MAX_EU];
  uint32_t   eu_base::_nb_eu = 0;

  std::map< void * , eu_base * > eu_base::_eu_map;

  eu_base::eu_base(sc_module_name name) :
    device_master(name),
    ExecutionSpy(),
    p_linker_loader("p_linker_loader"),
    _id(NULL)
  {
    DOUT_CTOR << this->name() << std::endl;

    SC_THREAD(thread);
    // thread id == eu id means not a thread context.
    _current_thread_id = (uintptr_t)this;
    DOUT_NAME << ": Initialize Thread ID = 0x" << std::hex << _current_thread_id << std::endl;
  }

  eu_base::~eu_base()
  {
    DOUT_DTOR << this->name() << std::endl;
  }

  void eu_base::end_of_elaboration()
  {
    device_master::end_of_elaboration();

    GET_ATTRIBUTE("ID",_id,uint32_t,false);
    DOUT << name() << ": ID = " << _id->value << std::endl;
  }

  void eu_base::thread()
  {
    void *  key;
    std::map< void * , eu_base * >::iterator eu_map_it;

    key = (void*)sc_get_current_process_b();
    eu_map_it = _eu_map.find(key);
    DOUT_NAME << " map[" << key << "] = " << std::hex << this << std::endl;
    ASSERT_MSG(eu_map_it == _eu_map.end(),"CPU already in the map");
    _eu_map[(void*)key] = this;

    _proccess_handle[_nb_eu] = key;
    _eu[_nb_eu] = this;
    _nb_eu++;

    _sw_entry = (entry_fct_t) p_linker_loader->get_start_addr();

    DOUT_NAME << " registering to the execution_spy" << std::endl;
    register_self(p_linker_loader->get_link_map());

    DOUT_NAME << " BOOTING ... " << std::endl;

    while(1)
    {
      _sw_entry();
      ASSERT_MSG( false, "Unexpected end of execution...");
    }
  }

  void eu_base::compute(annotation::annotation_t *trace, uint32_t count)
  {
#if 0
    uint32_t  index;
    DOUT_NAME << __func__ << " count " << count << std::endl;
    for( index = 0 ; index < count; index++ )
    {
      ASSERT_MSG(trace[index].db->InstructionCount, "Instruction count should not be 0");
      DOUT_NAME << __func__ << " wait " << trace[index].db->InstructionCount << std::endl;
      trace[index].eu = (uintptr_t)this;
      trace[index].thread = _current_thread_id;
      wait(trace[index].db->InstructionCount,SC_NS);
    }
#else
    uint32_t  index;
    uint32_t  instructions = 0;
    uint32_t  cycles = 0;
    DOUT_NAME << __func__ << " count " << count << std::endl;
    for( index = 0 ; index < count; index++ )
    {
      trace[index].eu = (uintptr_t)this;
      trace[index].thread = _current_thread_id;
      if(trace[index].type == annotation::BB_DEFAULT)
      {
        cycles += trace[index].db->CycleCount;
        instructions += trace[index].db->InstructionCount;
      }
    }
    DOUT_NAME << __func__ << " wait " << cycles << std::endl;
    wait(cycles,SC_NS,_it_event);

#endif

  }

  std::vector< const eu_base* > * eu_base::get_all_eu()
  {
    std::map< void * , eu_base * >::const_iterator mapit;
    std::vector< const eu_base* > * eu_vector;

    eu_vector = new std::vector< const eu_base* >();

    for(mapit = _eu_map.begin(); mapit != _eu_map.end() ; ++mapit)
    {
      eu_vector->push_back((*mapit).second);
    }
    return(eu_vector);
  }

  extern "C"
  {
    void mbb_annotation(annotation::annotation_db_t *db)
    {
      eu_base::get_current_eu()->annotate(db);
    }

    void annotation_entry()
    {
      eu_base::get_current_eu()->annotate_entry();
    }

    void annotation_return()
    {
      eu_base::get_current_eu()->annotate_return();
    }
  }
}


