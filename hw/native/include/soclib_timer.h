/*************************************************************************************
 * File   : soclib_timer.h,     
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

#ifndef __SOCLIB_TIMER_H__
#define __SOCLIB_TIMER_H__

#include "systemc.h"
#include "devices.h"

namespace native {

	class soclib_timer: 
		public device_slave
	{

		public:
		// IO interfaces methods
		void slv_read(uint32_t  *addr, uint32_t  *data);
		void slv_write(uint32_t  *addr, uint32_t  data);

		soclib_timer(sc_module_name name);
		~soclib_timer();

		void end_of_elaboration();

    enum SoclibTimerRegisters {
      TIMER_VALUE    = 0,
      TIMER_MODE     = 1,
      TIMER_PERIOD   = 2,
      TIMER_RESETIRQ = 3,
      TIMER_SPAN     = 4,
    };

    enum SoclibTimerMode {
      TIMER_RUNNING = 1,
      TIMER_IRQ_ENABLED = 2,
    };


    private:
    sc_attribute < uint32_t >    *_nb_timers;
    sc_attribute < char * >      *_irq_config;
  };

}

#endif				// __SOCLIB_TIMER_H__
