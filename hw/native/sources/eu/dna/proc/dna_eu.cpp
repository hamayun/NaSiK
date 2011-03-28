/*************************************************************************************
 * File   : dna_eu.cpp,     
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

#define DEBUG_OPTION "dna_eu"

#include "dna/proc/dna_eu.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "errno.h"

namespace native
{
  dna_eu::dna_eu(sc_module_name name) :
    eu_base(name), 
    _it_status(1),
    _it_enable(0),
    _it_flag(false)
  {
    DOUT_CTOR << this->name() << std::endl;
    _current_thread_id = 0xFFFFFFFF;
    std::sprintf(_current_thread_name,"noname");

    exp_it(*this);
  }

  dna_eu::~dna_eu()
  {
    DOUT_DTOR << this->name() << std::endl;
  }

  void dna_eu::end_of_elaboration()
  {
    eu_base::end_of_elaboration();
  }

  // Check whether this functions is Really Executed OR the one found in Execution Spy is Executed
  void dna_eu::synchronize()
  {
    dna_eu *ptr = this;
//    hal_call_count[_id->value]++;
//    _hal_call_profile++;
    wait(_sc_tps_synchro);
    if(ptr != this) cerr << "----\n";

    if(_it_status && _it_enable && _it_flag)
    {
      _it_enable = 0;
      it_handler();
      _it_flag = false;
    }
  }

  void dna_eu::it_set(uint32_t id)
  {
      if(_it_enable == 0) 
      {
        DOUT_NAME << " Interrupt missed" << std::endl;
      }
      else
      {
        _it_flag = true;
      }
  }

  void dna_eu::it_unset(uint32_t id)
  {
      _it_flag = false;
  }
}


