/*************************************************************************************
 * File   : soclib_tg.h,     
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

#ifndef __SOCLIB_TG_H__
#define __SOCLIB_TG_H__

#include "systemc.h"
#include "devices.h"

namespace libta {

	class soclib_tg: 
		public device_slave
	{

		public:
		// IO interfaces methods
		void slv_read (uint32_t   *addr, uint32_t *data);

		soclib_tg(sc_module_name name);
		~soclib_tg();

		void end_of_elaboration();

		private:
		sc_attribute < char * >     *_file;

    FILE                        *_file_id;

	};

}

#endif				// __SOCLIB_TG_H__
