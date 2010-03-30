/*************************************************************************************
 * File   : dna_eu.h,     
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

#ifndef __DNA_EU_H__
#define __DNA_EU_H__

#include "systemc.h"
#include "base/proc/eu_base.h"
#include "stdint.h"

extern "C" {
  #include "dna/hal/cal/headers/cpu_context.h"
  #include "dna/hal/cal/headers/cpu_trap.h"
}

namespace libta
{

class dna_eu:
  public eu_base
  {
    public:
      dna_eu(sc_module_name name);
      ~dna_eu();

      void end_of_elaboration();

      /* interrupt support */
    protected:
      void interrupt();
      interrupt_status_t        _it_status;
      sc_attribute < uint32_t > *_cpu_n_it;
      uint32_t                  *_it_lines;
      uintptr_t                 *_it_vector;

    public:

      /* eu context */
      void context_init(CPU_CONTEXT_T *ctx, void *sp, int32_t ssize, void *entry, void *arg);
      void context_load(CPU_CONTEXT_T *to);
      void context_save(CPU_CONTEXT_T *from);
      void context_commute(CPU_CONTEXT_T *from, CPU_CONTEXT_T *to);

      /* eu synchro */
      long int test_and_set(volatile long int * spinlock);
      long int compare_and_swap(volatile long int * p_val, long int oldval, long int newval);

      /* eu io */
      void write(uint8_t *addr, unsigned char value);
      void write(uint16_t *addr, unsigned short value);
      void write(uint32_t *addr, unsigned int value);
      uint8_t  read(uint8_t *addr);
      uint16_t read(uint16_t *addr);
      uint32_t read(uint32_t *addr);

      /* eu trap */
      void trap_attach_esr (exception_id_t id, exception_handler_t isr);
      void trap_attach_isr (interrupt_id_t id, uint32_t mode, interrupt_handler_t isr);
      interrupt_status_t trap_mask_and_backup (void);
      void trap_restore (interrupt_status_t backup);
      void trap_enable (interrupt_id_t id);
      void trap_disable (interrupt_id_t id);

      /* eu mp */
      unsigned int get_proc_id(void);

  };

} // end namespace libta

#endif				// __EU_H__
