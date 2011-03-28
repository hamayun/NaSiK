/*************************************************************************************
 * File   : dna_linker.h,     
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

#ifndef __DNA_LINKER_H__
#define __DNA_LINKER_H__

#include "systemc.h"
#include "base/linker_base.h"

namespace native {

	typedef struct _extfifo_config {
		uint32_t depth;
		uint32_t cell_size;

		void * header_address;
	
		uint32_t dtype;
		void * lock;
	} extfifo_config_t;

    class dna_linker : 
	public linker_base
    {
        public:
            dna_linker(sc_module_name name);
            ~dna_linker();
            uintptr_t get_start_addr();
            void end_of_elaboration();
            void start_of_simulation();

        private:
            sc_attribute< char * > *_cpu_os_entry_point;
            sc_attribute< char * > *_app_entry_point;

            sc_attribute< char * > *_sections_decl;
            sc_attribute< char * > *_sections_size;

            sc_attribute< char * > *_os_drivers_list;
            sc_attribute< char * > *_os_filesystems_list;

            sc_attribute< uint32_t > *_os_thread_stack_size;
            sc_attribute< uint32_t > *_platform_n_native;
            sc_attribute< uint32_t > *_channel_rdv_ndev;
			sc_attribute< uint32_t > *_extfifo_channel_ndev;
			sc_attribute< char * > *_extfifo_channels_list;
			sc_attribute< char * > *_extfifo_mem_section;
			//sc_attribute< char * > *_aicu_platform_base;

            sc_attribute< char * > *_os_kernel_heap_section;
            sc_attribute< char * > *_os_user_heap_section;
            sc_attribute< char * > *_os_kernel_bss_section;
    };
} // end namespace native

#endif				// __SW_LOADER_H__
