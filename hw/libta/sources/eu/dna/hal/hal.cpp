/*************************************************************************************
 * File   : hal.cpp,     
 *
 * Copyright (C) 2008 TIMA Laboratory
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

#define DEBUG_OPTION "dna_hal"

#include "dna/proc/dna_eu.h"
#include "assertion.h"
#include "debug.h"

extern "C" {
  #include "dna/hal/cal/headers/cpu_context.h"
}

#define SYNCHRONIZE()                               \
  DOUT_FCT << ": entry" << std::endl;               \
  dna_eu *current_eu = (dna_eu*)eu_base::get_current_eu();              \
  current_eu->synchronize()

using namespace libta;
using namespace annotation;

extern "C" {

  // Context switching
  // -------------------------------------------------------------------------------------------------------
  void __dnaos_hal_context_init(CPU_CONTEXT_T *ctx, void *sp, int32_t ssize, void *entry, void *arg)
  {
    SYNCHRONIZE();
    current_eu->context_init(ctx,sp,ssize,entry,arg);
  }
  void __dnaos_hal_context_load(context_t *to)
  {
    SYNCHRONIZE();
    current_eu->context_load(to);
  }
  void __dnaos_hal_context_save(context_t *from, uintptr_t *label)
  {
    SYNCHRONIZE();
    current_eu->context_save(from);
  }
  void __dnaos_hal_context_commute(context_t *from, context_t *to)
  {
    SYNCHRONIZE();
    current_eu->context_commute(from,to);
  }

  // IO accesses
  // -----------------------------------------------------------------------------------------------------
  void __dnaos_hal_write_uint8(uint8_t *addr, uint8_t value)
  {
    SYNCHRONIZE();
    current_eu->write(addr,value);
  }

  void __dnaos_hal_write_uint16(uint16_t *addr, uint16_t value)
  {
    SYNCHRONIZE();
    current_eu->write(addr,value);
  }

  void __dnaos_hal_write_uint32(uint32_t *addr, uint32_t value)
  {
    SYNCHRONIZE();
    current_eu->write(addr,value);
  }

  uint8_t  __dnaos_hal_read_uint8(uint8_t *addr)
  {
    SYNCHRONIZE();
    return(current_eu->read(addr));
  }

  uint16_t __dnaos_hal_read_uint16(uint16_t *addr)
  {
    SYNCHRONIZE();
    return(current_eu->read(addr));
  }

  uint32_t __dnaos_hal_read_uint32(uint32_t *addr)
  {
    SYNCHRONIZE();
    return(current_eu->read(addr));
  }

  // trap
  // ---------------------------------------------------------------------------------------------------
  void __dnaos_hal_trap_attach_esr (exception_id_t id, exception_handler_t isr)
  {
    SYNCHRONIZE();
    current_eu->trap_attach_esr(id,isr);
  }

  void __dnaos_hal_trap_attach_isr (interrupt_id_t id, uint32_t mode, interrupt_handler_t isr)
  {
    SYNCHRONIZE();
    current_eu->trap_attach_isr(id,mode,isr);
  }

  interrupt_status_t __dnaos_hal_trap_mask_and_backup (void)
  {
    SYNCHRONIZE();
    return(current_eu->trap_mask_and_backup());
  }

  void __dnaos_hal_trap_restore (interrupt_status_t backup)
  {
    SYNCHRONIZE();
    current_eu->trap_restore(backup);
  }

  void __dnaos_hal_trap_enable (interrupt_id_t id)
  {
    SYNCHRONIZE();
    current_eu->trap_enable(id);
  }

  void __dnaos_hal_trap_disable (interrupt_id_t id)
  {
    SYNCHRONIZE();
    current_eu->trap_disable(id);
  }

  // Multi-processor support
  // ---------------------------------------------------------------------------------------------------
  unsigned int __dnaos_hal_get_proc_id(void)
  {
    SYNCHRONIZE();
    return(current_eu->get_proc_id());
  }

  // Processor synchro support
  // ---------------------------------------------------------------------------------------------------
  long int __dnaos_hal_test_and_set(volatile long int * spinlock)
  {
    SYNCHRONIZE();
    return(current_eu->test_and_set(spinlock));
  }

  long int __dnaos_hal_compare_and_swap(volatile long int * p_val, long int oldval, long int newval)
  {
    SYNCHRONIZE();
    return(current_eu->compare_and_swap(p_val,oldval,newval));
  }
}
