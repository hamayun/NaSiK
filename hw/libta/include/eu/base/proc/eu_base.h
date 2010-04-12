/*************************************************************************************
 * File   : eu_base.h,     
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

#ifndef __EU_BASE_H__
#define __EU_BASE_H__

#include "systemc.h"
#include "assertion.h"
#include <map>
#include "interfaces/io.h"
#include "interfaces/link.h"
#include "execution_spy.h"
#include "annotation.h"
#include "devices.h"

#define MAX_EU 1024

namespace libta
{
    class eu_base:
    public device_master,
    public annotation::ExecutionSpy
    {
        public:
            sc_port<LINKER::LOADER> p_linker_loader;

            SC_HAS_PROCESS(eu_base);

            eu_base(sc_module_name name);
            ~eu_base();

            virtual void end_of_elaboration();
            void thread();

            // annotation support
            void compute(annotation::annotation_t *trace, uint32_t count);
            static inline eu_base * get_current_eu()
            {
                uint32_t i;
                void * current_process = (void*)::sc_get_current_process_b();
                for(i = 0; i < _nb_eu; i++)
                {
                    if(_proccess_handle[i] == current_process) break;
                }
                return(_eu[i]);
            };

            static std::vector< const eu_base* > * get_all_eu();

        protected:
            uintptr_t _current_thread_id;
            sc_attribute < uint32_t > *_id;                 // sc_attribute is a 'name' and 'value' pair

            typedef void (*entry_fct_t) ();
            entry_fct_t _sw_entry;                          // The Entry Point Function; Where the EU Starts its Execution. 

            /* eu management */
            static std::map< void * , eu_base * > _eu_map;

            /* mp support */
            static std::map< unsigned int , sc_event* > _mp_synchro_map;
            static void *     _proccess_handle[MAX_EU];
            static eu_base *  _eu[MAX_EU];
            static uint32_t   _nb_eu;

            /* interrupt support */
            sc_event  _it_event;
            virtual void interrupt() = 0;
    };
} // end namespace libta

#endif				// __EU_H__
