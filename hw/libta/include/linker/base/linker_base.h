/*************************************************************************************
 * File   : linker_base.h,     
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

#ifndef __LINKER_BASE_H__
#define __LINKER_BASE_H__

#include "systemc.h"
#include "interfaces/link.h"
#include "mapping.h"
#include "symbol.h"
#include <vector>
#include <string>

namespace libta {

#define NB_MAX_LINKER_PORT 256

	SC_MODULE(linker_base),
		public LINKER::LOADER
		{
			public:
				sc_export< LINKER::LOADER >  			    exp_linker_loader;
				sc_port< LINKER, NB_MAX_LINKER_PORT >    p_linker;

				void * load(const char * filename);
				mapping::segment_t * get_section(const char * section_name);
				virtual uintptr_t get_start_addr() = 0;
                            link_map* get_link_map();

				linker_base(sc_module_name name);
				~linker_base();
				void start_of_simulation();
				void end_of_elaboration();

			private:
				void load_elf_sections(const char * filename);

			protected:
				sc_attribute < char * > *_application;
				bool _end_of_elaboration_flag;

				void * _sw_image;
				std::vector< mapping::segment_t * > _sections_vector;
				std::vector< symbol_base * > _symbols_vector;
		};

} // end namespace libta

#endif				// __SW_LOADER_H__
