/*************************************************************************************
 * File   : tty_pool.h,     
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

#ifndef __TTY_POOL_H__
#define __TTY_POOL_H__

#include "systemc.h"
#include "devices.h"

namespace libta {

	class tty_pool: 
		public device_slave
	{

		public:
		// IO interfaces methods
		void slv_read(uint8_t  *addr, uint8_t  *data);
		void slv_write(uint8_t  *addr, uint8_t  data);

		tty_pool(sc_module_name name);
		~tty_pool();

		void end_of_elaboration();

		typedef enum {
			TTY_WRITE = 0,
			TTY_STATUS = 1,
			TTY_READ = 2,
			/* ... */
			TTY_SPAN = 4,
		} REGISTERS;

		private:
		sc_attribute < uint32_t >    *_nb_tty;
    sc_attribute < char * >      *_irq_config;
	};

}

#endif				// __TTY_POOL_H__
