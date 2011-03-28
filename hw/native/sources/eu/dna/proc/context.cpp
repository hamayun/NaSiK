/*************************************************************************************
 * File   : context.cpp,     
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

#define DEBUG_OPTION "dna_eu_context"

#include "dna/proc/dna_eu.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include <ucontext.h>

namespace native
{

  void dna_eu::context_init(CPU_CONTEXT_T *ctx, void *stack, int32_t ssize, void *entry, void *arg)
  {
    ucontext_t * new_ctx;
    new_ctx = new ucontext_t;
    getcontext(new_ctx);
    new_ctx->uc_stack.ss_sp = (void *)stack;
    new_ctx->uc_stack.ss_size = ssize;
    makecontext(new_ctx, (void (*)()) entry, 1, arg);
    *((ucontext_t**)ctx) = new_ctx;
    DOUT_NAME << " context_init: " << std::hex << ((ucontext_t**)ctx) << std::endl;
  }

  void dna_eu::context_load(CPU_CONTEXT_T *to)
  {
    ucontext_t tmpContext;
    _current_thread_id = (uintptr_t)to;
    DOUT_NAME << " context_load: " << std::hex << to << std::endl;
    memset(_current_thread_name,0x00,32);
    std::sprintf(_current_thread_name,"%s",(char*)((uint32_t)to - 0xA0 + 0x04));
    swapcontext(&tmpContext, *((ucontext_t **)to));
  }

  void dna_eu::context_save(CPU_CONTEXT_T *from)
  {
    DOUT_NAME << " save context 0x" << std::hex << from << std::endl;
    getcontext(*((ucontext_t**)from));
  }

  void dna_eu::context_commute(CPU_CONTEXT_T *from, CPU_CONTEXT_T *to)
  {
    _current_thread_id = (uintptr_t)to;
    DOUT_NAME << " context_commute: " << std::hex << from << " to 0x" << std::hex << to << std::endl;
	memset(_current_thread_name,0x00,32);
    std::sprintf(_current_thread_name,"%s",(char*)((uint32_t)to - 0xA0 + 0x04));
    swapcontext(*((ucontext_t**)from), *((ucontext_t**)to));
  }

}
