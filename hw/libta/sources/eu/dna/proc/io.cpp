/*************************************************************************************
 * File   : io.cpp,     
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

  void dna_eu::write(uint8_t *addr, uint8_t value)
  {
    p_io->write(addr,value);
  }

  void dna_eu::write(uint16_t *addr, uint16_t value)
  {
    p_io->write(addr,value);
  }

  void dna_eu::write(uint32_t *addr, uint32_t value)
  {
    p_io->write(addr,value);
  }

  uint8_t  dna_eu::read(uint8_t *addr)
  {
    uint8_t result;
    p_io->read(addr,&result);
    return(result);
  }

  uint16_t dna_eu::read(uint16_t *addr)
  {
    uint16_t result;
    p_io->read(addr,&result);
    return(result);
  }

  uint32_t dna_eu::read(uint32_t *addr)
  {
    uint32_t result;
    p_io->read(addr,&result);
    return(result);
  }

}
