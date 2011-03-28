/*************************************************************************************
 * File   : analyzer.cpp,     
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

#define DEBUG_OPTION "analyzer"

#include "analyzer.h"
#include "assertion.h"
#include "debug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "errno.h"
#include <libelf.h>
#include "pthread.h"
#include "semaphore.h"
#include "dlfcn.h"
#include <vector>
#include <string>
#include <string.h>
#include <fstream>
#include "stdio.h"
#include "stdlib.h"
#include <time.h>
#include <iostream>

namespace native
{
  namespace annotation
  {

    Analyzer::Analyzer(link_map *linkmap, annotation_buffer_t * buffers)
      : _buffers(buffers),
        _previous_thread_id(0),
        _current_thread_slot(NULL),
        _link_map(linkmap),
        _appli_base_addr(0),
        _ending(false),
        _online_analyze(false)
    {
      std::string filename;
      time_t rawtime;
      struct tm * timeinfo;


      DOUT_FCT << " Application " << linkmap->l_name << " at " << std::hex << linkmap->l_addr << std::endl;
      load_symtab(linkmap->l_name);
      sem_init(&_sem, 0, 1); 

      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      printf ( "Current local time and date: %s", asctime (timeinfo) );
      _str_tag << timeinfo->tm_year + 1900 << "-" << timeinfo->tm_mon + 1 << "-" << timeinfo->tm_mday;

      _analyze = isOptionSet(ANALYZE_OPTION);
      _online_analyze = isOptionSet(ONLINE_OPTION);
      DOUT_FCT << "ANALYZE_OPTION = " << _analyze << std::endl;
      DOUT_FCT << "ONLINE_OPTION = " << _online_analyze << std::endl;
      if((_online_analyze == false) && (_analyze == true))
      {
        filename = (std::string)"analyze." + _str_tag.str() + (std::string)".dump";
        _file_id = ::open(filename.c_str(), O_CREAT | O_TRUNC | O_WRONLY | O_LARGEFILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH );
        ASSERT_MSG( (_file_id > 0) , strerror(errno) );

        _appli_base_addr = linkmap->l_addr;
      }

      if(isDebugOptionSet(NO_THREAD_OPTION) == false)
        pthread_create(&_thread, NULL, Analyzer::thread_fct, (void*)this);
    }

    Analyzer::~Analyzer()
    {
    }

    void * Analyzer::thread_fct(void *args)
    {
      Analyzer *analyser;
      analyser = (Analyzer*)(args);

      analyser->_thread_fct();

      pthread_exit(NULL);
      return(NULL);
    }

    void Analyzer::print_annotation(annotation_t *bb)
    {
        std::cout << "\teu=0x" << std::hex << bb->eu
                  << "\tthread=0x" << std::hex << bb->thread
                  << "\tbb_addr=0x" << std::hex << bb->bb_addr
                  << "\tcaller_addr=0x" << std::hex << bb->caller_addr
                  << "\tdb->Type=0x" << std::hex << bb->db->Type
                  << "\tdb->InstructionCount=" << std::dec << bb->db->InstructionCount
                  << "\tdb->CycleCount=" << std::dec << bb->db->CycleCount
                  << "\ttype=" << std::dec << bb->type << std::endl;
    }

    void * Analyzer::_thread_fct()
    {
      uint32_t              buffer_index;
      annotation_t          *buffer;

      buffer_index = 0;

      while(!_ending)
      {
        DOUT_FCT << "wait on buffer " << buffer_index << " (" << &(_buffers[buffer_index].sem) << ")" << std::endl;
        sem_wait(&(_buffers[buffer_index].sem));
        DOUT_FCT << "buffer " << buffer_index << " ready"<< std::endl;

        sem_wait(&_sem);
        // compute buffer
        DOUT_FCT << _buffers[buffer_index].count << " annotation to push " << std::endl;
        buffer = _buffers[buffer_index].buffer;
        for(uint32_t i = 0; i < _buffers[buffer_index].count; i++)
        {
          // All addresses must be relative to the base address of the
          // loaded application in memory.
          buffer[i].bb_addr = buffer[i].bb_addr - _appli_base_addr;
          buffer[i].db = (annotation_db_t*)((uintptr_t)(buffer[i].db) - _appli_base_addr);
        }

        if(_analyze == true)
        {
          if(_online_analyze == true)
          {
            // online analysis
            buffer = _buffers[buffer_index].buffer;
            for(uint32_t i = 0; i < _buffers[buffer_index].count; i++)
            {
              //print_annotation(&(buffer[i]));
              push(&(buffer[i])); 
            }
          }
          else
          {
            // For offline anaylsis; write annotations in the buffer to the dump file.
            write(_file_id, (const void *)buffer, sizeof(annotation_t) * _buffers[buffer_index].count);
          }
        }
        _buffers[buffer_index].count = 0;

        sem_post(&_sem);

        sem_post(&(_buffers[buffer_index].ack));
        DOUT_FCT << "unlock buffer " << buffer_index << " (" << &(_buffers[buffer_index].sem) << ")" << std::endl;
        buffer_index = (buffer_index + 1) % NB_BUFFER;
      }
      DOUT_FCT << "exit ..." << std::endl;

      ::close(_file_id);

      return(NULL);
    }

    void Analyzer::ending()
    {
      _ending = true;
      sem_wait(&_sem);

      if(isDebugOptionSet(NO_THREAD_OPTION))
      {
        for(uint32_t i = 0; i < NB_BUFFER ; i++)
        {
          sem_post(&(_buffers[i].sem));
          sem_post(&(_buffers[i].ack));
        }

        sem_post(&_sem);
        DOUT_FCT << "join thread" << std::endl;
        pthread_join(_thread, NULL);
        DOUT_FCT << "thread joined" << std::endl;
      }

      build_callgrind();

    }

    void Analyzer::push(annotation_t *bb)
    {
      call_t           *call_ptr;
      call_context_t   *context_ptr;

      // Avoid search in the map each time a new bb is pushed.
      // Most of pushes belong to the previous thread slot.
      if(bb->thread != _previous_thread_id)
      {
        //std::cout << "Thread Switch, Current Thread=0x" << std::hex << bb->thread
        //          << " Previous Thread=0x" << std::hex << _previous_thread_id << std::endl;

        // if its not the previous thread, then search in the thread slots map.
        std::map< uintptr_t, thread_slot_t* >::iterator  it = _thread_slots.find(bb->thread);

        // if couldn't find the thread in slots, create a new thread and add it to slots
        if(it == _thread_slots.end())
        {
          // Create New thread slot ... and Initialize it. 
          _current_thread_slot = new thread_slot_t;
          *_current_thread_slot = INIT_THREAD_SLOT;
          _current_thread_slot->id = bb->thread;            
          _current_thread_slot->call_stack = new std::list< call_context_t* >();
          // Insert into the thread slot map
          _thread_slots[bb->thread] = _current_thread_slot;

          // Initial function context
          call_ptr = new call_t;
          *call_ptr = INIT_CALL;
          call_ptr->entry = NULL;
          call_ptr->symbol.name = (char*)"out_of_context";
          context_ptr = new call_context_t;
          *context_ptr = INIT_CALL_CONTEXT;
          context_ptr->call = call_ptr;
          _current_thread_slot->call_stack->push_back(context_ptr);

          DOUT_FCT << "New thread ID=0x" << std::hex << bb->thread
                   << " at 0x" << std::hex << _current_thread_slot
                   << " bb->db->InstructionCount = " << std::dec <<  bb->db->InstructionCount
                   << std::endl;
        }
        else
        {
          _current_thread_slot = it->second;
        }

        // Save the previous thread id for the next comparison
        _previous_thread_id = bb->thread;
        DOUT_FCT << "Current thread ID=0x" << std::hex << bb->thread << std::endl;
      }

      // Insert the bb ...
      basicblock_t  * cur_bb;
      basicblock_t  * bb_ptr;

      // Start from the first bb in the current thread slot
      cur_bb = &(_current_thread_slot->bb_list);

      // And loop until the bb_addr of the next bb is less than from the input bb address.
      while( (cur_bb->next != NULL) && (cur_bb->next->annotation.bb_addr < bb->bb_addr) )
        cur_bb = cur_bb->next;

      // Are we at the end of bb list? or next bb's address is really greater than from the input bb. 
      if( (cur_bb->next == NULL) || (cur_bb->next->annotation.bb_addr > bb->bb_addr) )
      {
        bb_ptr =  new basicblock_t;
        *bb_ptr = INIT_BB;
        // Insert into the bb list
        bb_ptr->next = cur_bb->next;
        cur_bb->next = bb_ptr;
        // Link Annotation to new list entry
        bb_ptr->annotation = *bb;
        bb_ptr->calls = new std::list< call_t* >();
        // Copy Annotation info from annotation db to cost object ... both within the "basicblock_t" object.
        compute_cost(bb_ptr);
      }
      // The bb to compute is the next one
      bb_ptr = cur_bb->next;

      sym_desc_t  *sym;
      // Find symbol for the current bb_addr in symbol vector ... constructed earlier
      sym = find_symbol(bb_ptr->annotation.bb_addr); 

      DOUT_FCT << "New BB belong to " << sym->name << "(0x" << bb_ptr->annotation.bb_addr << ")";
      DOUT << ((bb_ptr->annotation.type & BB_ENTRY) ? " ENTRY" : "")
           << ((bb_ptr->annotation.type & BB_RETURN) ? " RETURN" : "")  << std::endl;
      
      // If the bb is an entry basic block ... push context (It marks the start of a new function)
      if(bb_ptr->annotation.type & BB_ENTRY)
      {
        push_context(bb_ptr);
      }

      // Compute How many times the current BB has been executed.
      bb_ptr->counter++;

      // Cumulative cost for the Current Thread Slot. 
      add_cost(&(_current_thread_slot->call_stack->back()->cost), &(bb_ptr->cost)); 

      if(bb_ptr->annotation.type & BB_RETURN)
      {
        pop_context();
      }
      else
      {
        // perhaps this keeps track of the previous basic block of the same function
        // until we pop the context of this function. 
        _current_thread_slot->prev_bb = bb_ptr;
      }
    }

    inline void Analyzer::push_context(basicblock_t *bb)
    {
      std::list< call_t* >::iterator   calls_it;
      call_t           *call_ptr;
      call_context_t   *context_ptr;
      sym_desc_t  *sym;

      DOUT_FCT << ">>>>>>>>>>>>>>>>>>>>" << std::endl;
      print_stack();

      if(_current_thread_slot->prev_bb != NULL)
      {
        // A basic block may be the caller of multiple functions ... in LLVM
        for( calls_it = _current_thread_slot->prev_bb->calls->begin() ;
            calls_it != _current_thread_slot->prev_bb->calls->end() ;
            calls_it++)
        {
            if( (*calls_it)->entry == bb) break;
        }

        if( calls_it == _current_thread_slot->prev_bb->calls->end() )
        {
          call_ptr = new call_t;
          *call_ptr = INIT_CALL;
          sym = find_symbol(bb->annotation.bb_addr);
          call_ptr->entry = bb;
          call_ptr->symbol = *sym;
          _current_thread_slot->prev_bb->calls->push_back(call_ptr);

          DOUT_FCT << "Create call to " << call_ptr->symbol.name << std::endl;
        }
        else
        {
          call_ptr = *calls_it;
          DOUT_FCT << "call to " << call_ptr->symbol.name << " already exist" << std::endl;
        }

        DOUT_FCT << "Call from 0x" << std::hex << _current_thread_slot->prev_bb->annotation.bb_addr
                 << " to 0x" << std::hex << bb->annotation.bb_addr << std::endl;

        call_ptr->counter++;
        context_ptr = new call_context_t;
        *context_ptr = INIT_CALL_CONTEXT;
        context_ptr->call = call_ptr;
        context_ptr->prev_bb = _current_thread_slot->prev_bb;
      }
      else
      {
        // Initial function
        // This means that we are at the start of a New Function Context and this is the first BB
        // thats why we can search for its address and expect to find a corresponding symbol
        DOUT_FCT << " Entering a new context ";
        call_ptr = new call_t;
        *call_ptr = INIT_CALL;
        sym = find_symbol(bb->annotation.bb_addr);
        call_ptr->entry = bb;
        call_ptr->symbol = *sym;

        DOUT << "entry: 0x" << std::hex << call_ptr->entry << " symbol: " << call_ptr->symbol.name << std::endl;
        context_ptr = new call_context_t;
        *context_ptr = INIT_CALL_CONTEXT;
        context_ptr->call = call_ptr;
      }

      _current_thread_slot->call_stack->push_back(context_ptr);

      DOUT_FCT << std::hex << _current_thread_slot << " pushed " << context_ptr->call->symbol.name << std::endl;
    }

    inline void Analyzer::pop_context(void)
    {
      call_context_t   *context_ptr;

      DOUT_FCT << "<<<<<<<<<<<<<<<<<<<<" << std::endl;
      print_stack();

      context_ptr = _current_thread_slot->call_stack->back();
      _current_thread_slot->call_stack->pop_back();
      ASSERT(_current_thread_slot->call_stack->empty() == false);

      DOUT_FCT << std::hex << _current_thread_slot << " popped " << context_ptr->call->symbol.name << std::endl;
      std::cout << context_ptr->call->symbol.name << ":cost:" << std::dec << context_ptr->cost.instructions
                << ":" << context_ptr->cost.cycles << std::endl;

      add_cost(&(context_ptr->call->cost), &(context_ptr->cost));
      add_cost(&(_current_thread_slot->call_stack->back()->cost), &(context_ptr->cost));

      if(_current_thread_slot->call_stack->size() == 1)
      {
        DOUT_FCT << " Not in a context now"  << std::endl;
        _current_thread_slot->prev_bb = NULL;
      }
      else
      {
        _current_thread_slot->prev_bb = context_ptr->prev_bb;
      }

      delete context_ptr;
    }

    void Analyzer::print_stack()
    {
      std::list< call_context_t * >::iterator  list_it;
      uint32_t level;

      list_it = _current_thread_slot->call_stack->begin();
      level = 0;
      for( ; list_it != _current_thread_slot->call_stack->end(); list_it ++ , level ++)
      {
        DOUT_FCT << std::hex << _current_thread_slot << ":" << std::dec << level << " "
                 << (*list_it)->call->symbol.name << std::endl;
      }
    }

    inline void Analyzer::add_cost(cost_t * c1, cost_t *c2)
    {
      c1->instructions += c2->instructions;
      c1->cycles += c2->cycles;
    }

    inline void Analyzer::compute_cost(basicblock_t *bb)
    {
      // This copies the annotation info ... doesn't compute anything
      bb->cost.instructions = bb->annotation.db->InstructionCount;
      bb->cost.cycles = bb->annotation.db->CycleCount;
    }

    void Analyzer::load_symtab(const char * filename)
    {
      int                      fd;
      Elf                      *elf;
      Elf_Scn                  *scn;
      Elf32_Ehdr               *ehdr32;
      Elf32_Shdr               *shdr;
      Elf_Data                 *data;
      Elf32_Sym                *esym;
      Elf32_Sym                *lastsym;
      const char               *section_name;
      const char               *sym_name;
      std::list< sym_desc_t >  sym_list;
      sym_desc_t               symbol;

      DOUT_FCT << filename << std::endl;
      ASSERT_MSG ( (elf_version(EV_CURRENT) != EV_NONE), elf_errmsg(-1));
      ASSERT_MSG ( ((fd = open(filename, O_RDONLY, 0)) >= 0 ), strerror(errno));
      ASSERT_MSG ( ((elf = elf_begin(fd, ELF_C_READ, NULL)) != NULL), elf_errmsg(-1));
      ASSERT_MSG ( (elf_kind(elf) == ELF_K_ELF), "Not an ELF object.");

      ehdr32 = elf32_getehdr(elf);

      scn = NULL;
      while( (scn = elf_nextscn(elf,scn)) != NULL)
      {
        shdr = elf32_getshdr(scn);
        section_name = elf_strptr(elf, ehdr32->e_shstrndx, (size_t)shdr->sh_name);
        if( shdr->sh_type == SHT_SYMTAB )
        {
          DOUT_FCT << " SHT_SYMTAB found in " << section_name << std::endl;

          data = NULL;
          ASSERT_MSG( ( ((data = elf_getdata(scn, data)) != NULL) && (data->d_size != 0) ), "Section had no data !!!");

          esym = (Elf32_Sym*) data->d_buf;
          lastsym = (Elf32_Sym*) ((char*)data->d_buf + data->d_size);

          for( ; esym < lastsym ; esym++)
          {
            if( (esym->st_value == 0) || (ELF32_ST_BIND(esym->st_info)== STB_WEAK) ||
                (ELF32_ST_BIND(esym->st_info)== STB_NUM) || (ELF32_ST_TYPE(esym->st_info)!= STT_FUNC)) 
              continue;

            sym_name = elf_strptr(elf,shdr->sh_link , (size_t)esym->st_name);
            ASSERT_MSG( (sym_name != NULL), elf_errmsg(elf_errno()));
            symbol.name = strdup(sym_name);
            symbol.base_addr = (uintptr_t)esym->st_value;
            sym_list.push_back(symbol);
          }
        }
      }
      sym_list.sort(symbol_compare);
      /* Convert list to vector */
      while(sym_list.empty() == false)
      {
        _sym_vector.push_back(sym_list.front());
        sym_list.pop_front();
      }

      //std::cout << "Symbol Vector Table" << std::endl;
      for( uint32_t i = 0 ; i < _sym_vector.size() ; i++)
      {
        //DOUT_FCT << (void*)_sym_vector[i].base_addr << " " << _sym_vector[i].name << std::endl;
      }

      (void) elf_end(elf);
      (void) ::close(fd);
    }

    bool Analyzer::symbol_compare(sym_desc_t s1, sym_desc_t s2)
    {
      if(s1.base_addr < s2.base_addr) return(true);
      return(false);
    }

    sym_desc_t * Analyzer::find_symbol(uintptr_t offset)
    {
      uintptr_t   address = offset - _link_map->l_addr;

      // Check if symbol is in cache
      std::map< uintptr_t, sym_desc_t *>::iterator    it = _sym_cash.find(address);
      if( it != _sym_cash.end())
      {
        return(it->second);
      }

      for(uint32_t i = 0; i < _sym_vector.size(); i++)
      {
        if( (_sym_vector[i+1].base_addr >= address) && (_sym_vector[i].base_addr <= address) )
        {
          _sym_cash[address] = &(_sym_vector[i]);
          return(&_sym_vector[i]);
        }
      }
      ASSERT_MSG( false, "Should not be here ... don't the limit of the last function !!!");
      return((sym_desc_t*)NULL);
    }

    void Analyzer::build_callgrind()
    {
      std::map< uintptr_t, thread_slot_t * >::iterator  threads_it;
      basicblock_t      *cur_bb;
      sym_desc_t        *sym;
      uint32_t          line_index;
      std::fstream      callgrind_file;
      std::string       filename;
      uint32_t          id;
      std::stringstream str_id; 
      std::list< call_t* >::iterator   calls_it;

      for( threads_it = _thread_slots.begin(), id = 0; threads_it != _thread_slots.end(); threads_it++, id++)
      {
        // Seek to the begining of str_id stringstream.
        str_id.seekp(std::ios_base::beg);
        str_id << id;

        filename = (std::string)"cachegrind.out." + _str_tag.str();
        if(id > 0)
          filename += (std::string)"." + str_id.str();

        // Create a New file for each thread. 
        DOUT_FCT << "Create " << filename << std::endl;
        callgrind_file.open(filename.data(), std::fstream::out);

        // Empty the context stack ...
        _current_thread_slot = threads_it->second;
        DOUT_FCT << " Empty context stack" << std::endl;
        while(_current_thread_slot->call_stack->size() > 1) 
        {
          DOUT_FCT << " Context stack size is " << _current_thread_slot->call_stack->size() << std::endl;
          pop_context();
        }

        cur_bb = _current_thread_slot->bb_list.next;

        callgrind_file << "events: Instructions Cycles\n\n";

        while( cur_bb != NULL )
        {

          line_index = 0;
          //
          // The current basic block should be an entry here ...
          // ASSERT_MSG(cur_bb->annotation.db->Type & BB_ENTRY, "expected an ENTRY basicblock");
          ASSERT_MSG(cur_bb->annotation.type & BB_ENTRY, "expected an ENTRY basicblock");
          sym = find_symbol(cur_bb->annotation.bb_addr);
          DOUT_FCT << "Begin function " << sym->name << std::endl;
          DOUT_FCT << "   BB @0x" << cur_bb->annotation.bb_addr << " -> " << sym->name;
//          DOUT << ((cur_bb->annotation.db->Type & BB_ENTRY) ? " ENTRY" : "")
//               << ((cur_bb->annotation.db->Type & BB_RETURN) ? " RETURN" : "")  << std::endl;
          DOUT << ((cur_bb->annotation.type & BB_ENTRY) ? " ENTRY" : "")
               << ((cur_bb->annotation.type & BB_RETURN) ? " RETURN" : "")  << std::endl;
          callgrind_file << "fn=" << sym->name << std::endl;
          callgrind_file << "0 " << cur_bb->cost.instructions * cur_bb->counter 
                         << " "  << cur_bb->cost.cycles * cur_bb->counter << std::endl;

          // All the calls that originate in this basic block
          for( calls_it = cur_bb->calls->begin() ; calls_it != cur_bb->calls->end() ; calls_it++)
          {
            DOUT_FCT << "   + call to " << (*calls_it)->symbol.name << std::endl;
            callgrind_file << "cfn=" << (*calls_it)->symbol.name << std::endl;
            callgrind_file << "calls=" << (*calls_it)->counter << " 0" << std::endl;
            callgrind_file << "+1 " << (*calls_it)->cost.instructions 
                           << " "   << (*calls_it)->cost.cycles << std::endl;
          }

          cur_bb = cur_bb->next;
          //while( (cur_bb != NULL) && !(cur_bb->annotation.db->Type & BB_ENTRY))
          while( (cur_bb != NULL) && !(cur_bb->annotation.type & BB_ENTRY))
          {
            sym = find_symbol(cur_bb->annotation.bb_addr);
            DOUT_FCT << "   BB @0x" << cur_bb->annotation.bb_addr << " -> " << sym->name;
            //DOUT << ((cur_bb->annotation.db->Type & BB_ENTRY) ? " ENTRY" : "")
            //     << ((cur_bb->annotation.db->Type & BB_RETURN) ? " RETURN" : "")  << std::endl;
            DOUT << ((cur_bb->annotation.type & BB_ENTRY) ? " ENTRY" : "")
                 << ((cur_bb->annotation.type & BB_RETURN) ? " RETURN" : "")  << std::endl;
            callgrind_file << "+1 " << cur_bb->cost.instructions * cur_bb->counter 
                           << " "   << cur_bb->cost.cycles * cur_bb->counter << std::endl;

            for( calls_it = cur_bb->calls->begin() ; calls_it != cur_bb->calls->end() ; calls_it++)
            {
              DOUT_FCT << "   + call to " << (*calls_it)->symbol.name << std::endl;
              callgrind_file << "cfn=" << (*calls_it)->symbol.name << std::endl;
              callgrind_file << "calls=" << (*calls_it)->counter << " 0" << std::endl;
              if( (*calls_it)->cost.instructions > 0 )
                callgrind_file << "+1 " << (*calls_it)->cost.instructions 
                               << " "   << (*calls_it)->cost.cycles << std::endl;
            }

            line_index++;
            cur_bb = cur_bb->next;
          } 

          callgrind_file << "\n";

        }

        callgrind_file.close();
      }
    }
  }
}
