/*************************************************************************************
 * File   : analyzer.h,     
 *
 * Copyright (C) 2008 TIMA Laboratory
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

#ifndef __ANALYZER_H__
#define __ANALYZER_H__

#include "annotation.h"
#include <map>
#include <vector>
#include <list>
#include <sstream>

#define INIT_COST (cost_t){ 0, 0 }
#define INIT_CALL (call_t){ { 0, NULL} , NULL , 0, INIT_COST }
#define INIT_ANNOTATION (annotation_t){ 0, 0, 0, 0, NULL, 0 }
#define INIT_BB   (basicblock_t){ INIT_ANNOTATION, 0, NULL, INIT_COST, NULL }
#define INIT_THREAD_SLOT  (thread_slot_t){ 0, INIT_BB, NULL, NULL}
#define INIT_CALL_CONTEXT (call_context_t){ NULL, NULL, INIT_COST }

typedef struct
{
  uint32_t    instructions;
  uint32_t    cycles;
} cost_t;

typedef struct
{
  uintptr_t     base_addr;
  char          *name;
} sym_desc_t;

typedef struct
{
  sym_desc_t                symbol;
  struct bb_t               *entry;
  uint32_t                  counter;
  cost_t                    cost;
} call_t;

typedef struct
{
  call_t          *call;
  bb_t            *prev_bb;
  cost_t          cost;
} call_context_t;

typedef struct bb_t
{
  annotation_t          annotation;
  uint32_t              counter;
  std::list< call_t* >  *calls;
  cost_t                cost;
  bb_t                  *next;
} basicblock_t;

typedef struct
{
  uintptr_t                     id;
  basicblock_t                  bb_list; // The list head is a dummy bb
  basicblock_t                  *prev_bb;
  std::list< call_context_t* >  *call_stack;
} thread_slot_t;

class Analyzer
{
  public:
      Analyzer(char * elf_file, uintptr_t app_base_addr, annotation_buffer_t * buffers,
                   bool no_thread, bool analyze, bool online_analyze);
      ~Analyzer(void);

      static void*  thread_fct(void *args);
      void          print_annotation(annotation_t *bb);
      void*         _thread_fct();
      void          ending();
      void          push(annotation_t * bb);

  private:
      void build_callgrind();
      void load_symtab(const char * filename);
      void print_stack();

      inline void push_context(basicblock_t *bb);
      inline void pop_context(void);
      inline void add_cost(cost_t * c1, cost_t *c2);
      inline void compute_cost(basicblock_t *bb);
      static bool symbol_compare(sym_desc_t s1, sym_desc_t s2);
      sym_desc_t * find_symbol(uintptr_t offset);

  private:
      pthread_t                               _thread;
      annotation_buffer_t                     *_buffers;
      std::map< uintptr_t, thread_slot_t * >  _thread_slots;
      uintptr_t                               _previous_thread_id;
      thread_slot_t                           *_current_thread_slot;
      uintptr_t                               _appli_base_addr;
      bool                                    _ending;
      sem_t                                   _sem;

      std::map< uintptr_t, sym_desc_t * >     _sym_cash;
      std::vector< sym_desc_t >               _sym_vector;

      int                                     _file_id;
      std::stringstream                       _str_tag;
      bool                                    _no_thread; 
      bool                                    _online_analyze;
      bool                                    _analyze;
};

#define ASSERT(expression) \
if(!(expression)) \
{ \
	std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : assertion failed in " << __func__ << ", " << #expression << std::endl; \
	*(int *)NULL = 0; \
}

#define ASSERT_MSG(expression, msg) \
if(!(expression)) \
{ \
	std::cerr << __FILE__ << ":" << std::dec << __LINE__ << " : assertion failed in " << __func__ << ", " << #expression << std::endl; \
	std::cerr << "ASSERT MESSAGE: " << msg << std::endl; \
	*(int *)NULL = 0; \
}

#endif				// __ANALYZER_H__
