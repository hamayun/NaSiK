/*************************************************************************************
 * File   : dna_eu.h,     
 *
 * Copyright (C) 2009 TIMA Laboratory
 * Author(s) :      Patrice, GERIN patrice.gerin@imag.fr
 * Bug Fixer(s) :   Mian-Muhammad, HAMAYUN mian-muhammad.hamayun@imag.fr
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

#ifndef __DNA_EU_H__
#define __DNA_EU_H__

#include "systemc.h"
#include "base/proc/eu_base.h"
#include "interfaces/interrupt.h"
#include "stdint.h"

extern "C" {
  #include "dna/hal/cal/headers/cpu_context.h"
  #include "dna/hal/cal/headers/cpu_trap.h"
}

namespace native
{
    class dna_eu : 
		public eu_base, 
		public INTERRUPT
    {
        public:
			sc_export< INTERRUPT > exp_it;
			void it_set(uint32_t id);
			void it_unset(uint32_t id);

            dna_eu(sc_module_name name);
            ~dna_eu();

            void end_of_elaboration();

            /* eu context */
            void context_init(CPU_CONTEXT_T *ctx, void *sp, int32_t ssize, void *entry, void *arg);
            void context_load(CPU_CONTEXT_T *to);
            void context_save(CPU_CONTEXT_T *from);
            void context_commute(CPU_CONTEXT_T *from, CPU_CONTEXT_T *to);

            /* eu synchro */
            long int test_and_set(volatile long int * spinlock);
            long int compare_and_swap(volatile long int * p_val, long int oldval, long int newval);

            /* eu io */
      void write(uint8_t *addr, uint8_t value);
      void write(uint16_t *addr, uint16_t value);
      void write(uint32_t *addr, uint32_t value);
            void write(uint64_t *addr, uint64_t value);
            void write(double *addr, double value);
            void write(float *addr, float value);

            uint8_t  read(uint8_t *addr);
            uint16_t read(uint16_t *addr);
            uint32_t read(uint32_t *addr);
            uint64_t read(uint64_t *addr);
            double read(double *addr);
            float read(float *addr);

            /* eu trap */
            void trap_attach_esr (exception_id_t id, exception_handler_t isr);
            void trap_attach_isr (interrupt_id_t id, uint32_t mode, interrupt_handler_t isr);
            interrupt_status_t trap_mask_and_backup (void);
            void trap_restore (interrupt_status_t backup);
            void trap_enable (interrupt_id_t id);
            void trap_disable (interrupt_id_t id);

            /* eu mp */
            unsigned int get_proc_id(void);

			/* interrupt support */
			public:
			interrupt_status_t          _it_status;
			bool                        _it_enable;
			bool                        _it_flag;
			interrupt_handler_t         handler_table[CPU_N_IT];

			void it_handler();

    };

	inline void sc_trace(sc_trace_file *tf, const dna_eu &eu, std::ofstream *vcd_conf)
	{
	  char _buf[32];
	  ::sc_trace(tf, eu._current_thread_id, (std::string)(eu.name()) + ".id");
	  for(unsigned int i = 0; i < 32 ; i++)
	  {
		std::sprintf(_buf,"[%d]",i);
		::sc_trace(tf, &eu._current_thread_name[i], (std::string)(eu.name()) + ".name" + _buf);
	  }
	  ::sc_trace(tf, eu._it_status, (std::string)(eu.name()) + ".it_status");
	  ::sc_trace(tf, eu._it_enable, (std::string)(eu.name()) + ".it_enable");
	  ::sc_trace(tf, eu._it_flag, (std::string)(eu.name()) + ".it_flag");
	  //::sc_trace(tf, eu.hal_call_profile, (std::string)(eu.name()) + ".hal_call");

	  if(vcd_conf != NULL)
		*vcd_conf << "@200\n-" << eu.name() << ":\n";
	  if(vcd_conf != NULL)
	  {
		*vcd_conf << "@20\n+id SystemC.\\" << eu.name() << ".id" << "[" << sizeof(uintptr_t) * 8 - 1 << ":0]\n";

		*vcd_conf << "@820\n#+thread ";
		for(unsigned int i = 0; i < 32 ; i++)
		{
		  for(unsigned int j = 0; j < 8 ; j++)
		  {
		    *vcd_conf << "(" << j << ")SystemC.\\" << eu.name() << ".name(" << i << ")" << "[7:0] ";
		  }
		}
		*vcd_conf << "\n";
		*vcd_conf << "@20\n+it_status SystemC.\\" << eu.name() << ".it_status" << "\n";
		*vcd_conf << "@20\n+it_enable SystemC.\\" << eu.name() << ".it_enable" << "\n";
		*vcd_conf << "@20\n+it_flag SystemC.\\" << eu.name() << ".it_flag" << "\n";
		*vcd_conf << "@20\n+hal_call SystemC.\\" << eu.name() << ".hal_call" << "[" << sizeof(uintptr_t) * 8 - 1 << ":0]\n";
	  }
	}
} // end namespace native
#endif				// __EU_H__

