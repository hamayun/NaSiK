/*************************************************************************************
 * File   : framebuffer.h,     
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

#ifndef __FRAMEBUFFER_H__
#define __FRAMEBUFFER_H__

#include "systemc.h"
#include "devices.h"
#include "SDL/SDL.h"

namespace native {

	class framebuffer: 
		public device_slave
	{

		public:
		// IO interfaces methods
		void slv_read  (uint32_t  *addr, uint32_t  *data);
		void slv_read  (uint16_t  *addr, uint16_t  *data);
		void slv_read  (uint8_t  *addr, uint8_t  *data);
		void slv_write (uint32_t  *addr, uint32_t  data);
		void slv_write (uint16_t  *addr, uint16_t  data);
		void slv_write (uint8_t   *addr, uint8_t  data);

		framebuffer(sc_module_name name);
		~framebuffer();

		void end_of_elaboration();
    void * _screen_thread();

		private:
		void init(void);
    static void * screen_thread(void *args);

		private:

		sc_attribute< uint32_t >        *_width;
		sc_attribute< uint32_t >        *_height;

    SDL_Surface                     *_screen;
    pthread_t                       _thread;

	};

}

#endif				// __SOCLIB_FB_H__
