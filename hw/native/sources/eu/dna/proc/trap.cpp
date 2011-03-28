/*************************************************************************************
 * File   : trap.cpp,     
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

namespace native
{
    void dna_eu::trap_attach_esr (exception_id_t id, exception_handler_t isr)
    {
    }

    void dna_eu::trap_attach_isr (interrupt_id_t id, uint32_t mode, interrupt_handler_t isr)
    {
      handler_table[id] = isr;
    }

    interrupt_status_t dna_eu::trap_mask_and_backup (void)
    {
      interrupt_status_t old;
      old = _it_status;
      _it_status = 0;
      return(old);
    }

    void dna_eu::trap_restore (interrupt_status_t backup)
    {
      _it_status = backup;
    }

    void dna_eu::trap_enable (interrupt_id_t id)
    {
      _it_enable = 1;
    }

    void dna_eu::trap_disable (interrupt_id_t id)
    {
      _it_enable = 0;
    }



}
