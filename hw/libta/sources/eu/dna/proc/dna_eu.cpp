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

#define DEBUG_OPTION "eu"

#include "dna/proc/dna_eu.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "errno.h"

namespace libta
{
  dna_eu::dna_eu(sc_module_name name) :
    eu_base(name)
  {
    DOUT_CTOR << this->name() << std::endl;
    _it_status = 0;
  }

  dna_eu::~dna_eu()
  {
    DOUT_DTOR << this->name() << std::endl;
    delete [] _it_vector;
  }

  void dna_eu::end_of_elaboration()
  {
    eu_base::end_of_elaboration();

    GET_ATTRIBUTE("CPU_N_IT",_cpu_n_it,uint32_t,false);
    DOUT << name() << ": CPU_N_IT = " << _cpu_n_it->value << std::endl;
    _it_vector = new uintptr_t[_cpu_n_it->value];
    for(uint32_t i = 0; i < _cpu_n_it->value ; i ++) _it_vector[i] = (uintptr_t)NULL;

  }

  void dna_eu::interrupt()
  {
  }

}


