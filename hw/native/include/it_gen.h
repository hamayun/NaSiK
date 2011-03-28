/*************************************************************************************
 * File   : it_gen.h,     
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

#ifndef __IT_GEN_H__
#define __IT_GEN_H__

#include "systemc.h"
#include "devices.h"
#include "interfaces/interrupt.h"

namespace native {

	class it_gen: 
		public device_slave
	{
		public:
      sc_port< INTERRUPT > p_it;

		public:
		// IO interfaces methods
		void slv_read(uint32_t  *addr, uint32_t  *data);
		void slv_write(uint32_t  *addr, uint32_t  data);

    SC_HAS_PROCESS(it_gen);

		it_gen(sc_module_name name);
		~it_gen();

    void thread();

		void end_of_elaboration();

		typedef enum {
			IT_START = 0,
			/* ... */
			IT_SPAN = 1,
		} REGISTERS;

		private:
		sc_attribute < uint32_t >    *_nb_cycles;

    sc_event                      _wake_up;

	};

}

#endif				// __IT_GEN_H__
