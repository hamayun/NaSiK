/*************************************************************************************
 * File   : spinlock.h,     
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

#ifndef __SPINLOCK_H__
#define __SPINLOCK_H__

#include "systemc.h"
#include "devices.h"

namespace native {

  class spinlock:
    public device_slave
  {
    public:
      sc_port < LINKER::LOADER > p_linker_loader;

      void slv_read(uint32_t  *addr, uint32_t *data);
      void slv_write(uint32_t  *addr, uint32_t data);

      spinlock(sc_module_name name);
      ~spinlock();

      void end_of_elaboration();

    private:

      sc_attribute < char* >    *_section;

  };

}

#endif				// __SPINLOCK_H__
