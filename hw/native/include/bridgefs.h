/*************************************************************************************
 * File   : bridgefs.h,     
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

#ifndef __BRIDGEFS_H__
#define __BRIDGEFS_H__

#include "systemc.h"
#include "devices.h"

namespace native {

  class bridgefs:
    public device_master_slave
  {
    public:

      void slv_read  (uint32_t  *addr, uint32_t  *data);
      void slv_write (uint32_t  *addr, uint32_t  data);

      SC_HAS_PROCESS(bridgefs);

      bridgefs(sc_module_name name);
      ~bridgefs();

      void thread();
      void end_of_elaboration();

      enum {
        REG_FD = 0,
        REG_BUFFER = 1,
        REG_SIZE = 2,
        REG_HOW = 3,
        REG_MODE = 4,
        REG_OP = 5,
        REG_RETVAL = 6,
        REG_ERRNO = 7,
        REG_IRQ_ENABLE = 8,
        BRIDGEFS_NB_REG = 9,
      };

    private:
      void open_file();
      void read_file();
      void lseek_file();

    private:
      sc_event	_op_event;

    private:
      sc_attribute < uint32_t >    *_nb_fdaccess;
      sc_attribute < char * >      *_irq_config;

  };

}

#endif				// __TTY_POOL_H__
