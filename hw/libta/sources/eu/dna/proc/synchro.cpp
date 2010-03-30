/*************************************************************************************
 * File   : synchro.cpp,     
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

namespace libta
{
  long int dna_eu::test_and_set(volatile long int * spinlock)
  {
    long int ret;
    while(
        ((ret = p_io->load_linked((uint32_t*)spinlock,_id->value)) == 0) &&
        (p_io->store_cond((uint32_t*)spinlock,1,_id->value) == false)
        );
    return(ret);
  }

  long int dna_eu::compare_and_swap(volatile long int * p_val, long int oldval, long int newval)
  {
    long int ret;
    while(
        ((ret = p_io->load_linked((uint32_t*)p_val,_id->value)) == oldval) &&
        ( (ret = (p_io->store_cond((uint32_t*)p_val,newval,_id->value))) == false)
        );
    return(ret);
  }
}
