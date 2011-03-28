/*************************************************************************************
 * File   : execution_spy.cpp,     
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

#define DEBUG_OPTION "execution_spy"

#include "execution_spy.h"
#include "analyzer.h"
#include "assertion.h"
#include "debug.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <link.h>
#include "errno.h"
#include "pthread.h"
#include "semaphore.h"
#include "dlfcn.h"
#include <vector>
#include <iostream>
#include <string.h>
#include <iostream>

using namespace std; 

namespace native
{
  namespace annotation
  {
    std::map< std::string , annotation_shared_t* >     ExecutionSpy::_annotation_shared_map;
    bool                                               ExecutionSpy::_no_thread = false;
    bool                                               ExecutionSpy::_analyze = false;

    ExecutionSpy::ExecutionSpy()
      : _annotation_shared(NULL),
        _link_map(NULL)
    {
      _analyze = isOptionSet(ANALYZE_OPTION);
      _no_thread = isDebugOptionSet(NO_THREAD_OPTION);
    }

    ExecutionSpy::~ExecutionSpy()
    {}

    void ExecutionSpy::synchronize_no_thread()
    {
      annotation_buffer_t  *buffer_ptr;
      annotation_t         *annotation_ptr;

      buffer_ptr = &(_annotation_shared->buffers[0]);
      annotation_ptr = &(buffer_ptr->buffer[buffer_ptr->count]);
      buffer_ptr->count = ((buffer_ptr->count) + 1) % BUFFER_SIZE;
      // Compute ONE annotation information
      compute(annotation_ptr, 1);

      // In case of no_thread option, we don't care about the online analyze option.
      // And always push annotations to anaylzer in case analyze option is set.
      if(_analyze == true)
      {
        DOUT_FCT << "push annotation" << std::endl;
        _annotation_shared->analyzer->push(annotation_ptr);
      }
    }

    void ExecutionSpy::synchronize()
    {
      annotation_buffer_t  *buffer_ptr;
      annotation_t         *annotation_ptr;
      uint32_t             saved_count;
      uint32_t             next_buffer;
      uint32_t             previous_buffer = 0;
      uint32_t             buffer_switched = 0;

      // if no thread option is set, the annotation db are analyzed one by one
      // at each annotation call. This mode is for debug purpose.
      if(_no_thread)
      {
        return;
      }

      buffer_ptr =  &(_annotation_shared->buffers[_annotation_shared->current]);
      annotation_ptr =  &(buffer_ptr->buffer[buffer_ptr->count - _annotation_shared->db_count]);
      saved_count = _annotation_shared->db_count;

      // First, just switch to the next buffer but do not start the dump
      // of the current buffer to the file because the eu will work on
      // the trace data's.
      if( buffer_ptr->count >= BUFFER_THRESHOLD)
      {
        next_buffer = (_annotation_shared->current + 1) % NB_BUFFER;
        DOUT_FCT << this << " wait ack " << next_buffer << std::endl;
        sem_wait(&(_annotation_shared->buffers[next_buffer].ack));
        previous_buffer = _annotation_shared->current;
        DOUT_FCT << this << " switch buffer " << _annotation_shared->current << " -> " << next_buffer << std::endl;
        _annotation_shared->current = next_buffer;
        buffer_switched = 1;
      }
      _annotation_shared->db_count = 0;

      // Compute annotation information
      compute(annotation_ptr, saved_count);

      // Now we can unlock the thread to dump the trace.
      if(buffer_switched)
      {
        DOUT_FCT << this << " unlock buffer " << previous_buffer << " (" << &(_annotation_shared->buffers[previous_buffer].sem) << ")" << std::endl;
        sem_post(&(_annotation_shared->buffers[previous_buffer].sem));
      }
    }

    void ExecutionSpy::register_self(link_map *linkmap)
    {
      std::map< std::string , annotation_shared_t* >::iterator  mapit;

      DOUT_FCT << ": application " << linkmap->l_name << std::endl;

      mapit = _annotation_shared_map.find(linkmap->l_name);
      if(mapit == _annotation_shared_map.end())
      {
        DOUT_FCT << ": new application ... " << std::endl;
        _annotation_shared = new annotation_shared_t;
        memset(_annotation_shared, 0x0, sizeof(annotation_shared_t));

        _annotation_shared->current = 0;
        _annotation_shared->db_count = 0;

        for( int i = 0 ; i < NB_BUFFER ; i ++) 
        {
          sem_init(&(_annotation_shared->buffers[i].sem), 0, 0);
          sem_init(&(_annotation_shared->buffers[i].ack), 0, 1);
          _annotation_shared->buffers[i].count = 0;
        }
        sem_wait(&(_annotation_shared->buffers[_annotation_shared->current].ack));
        _annotation_shared->analyzer = new Analyzer(linkmap, _annotation_shared->buffers);
        _annotation_shared_map[linkmap->l_name] = _annotation_shared;
      }
      else
      {
        DOUT_FCT << ": application already exist ... " << std::endl;
        _annotation_shared = _annotation_shared_map[linkmap->l_name];
      }
      _link_map = linkmap;
    }

    void ExecutionSpy::close()
    {
//      if(_no_thread == true) return;
      std::map< std::string , annotation_shared_t* >::iterator  mapit;

      for( mapit = _annotation_shared_map.begin() ; mapit != _annotation_shared_map.end() ; mapit++)
      {
        mapit->second->analyzer->ending();
      }

    }

    void ExecutionSpy::annotate_entry()
    {
      annotation_buffer_t  *buffer_ptr;

      buffer_ptr =  &(_annotation_shared->buffers[_annotation_shared->current]);
      buffer_ptr->buffer[buffer_ptr->count].type = BB_ENTRY;

      if(_no_thread)
      {
        synchronize_no_thread();
        return;
      }

      _annotation_shared->db_count++;
      buffer_ptr->count++;
      ASSERT(buffer_ptr->count <= BUFFER_SIZE);
      // The computation trshold is used to break
      // dead lock in the annotated software.
      if( (_annotation_shared->db_count >= COMPUTATION_THRESHOLD) || (buffer_ptr->count >= BUFFER_SIZE))
      {
        synchronize();
      }
    }

    void ExecutionSpy::annotate(annotation_db_t *db)
    {
      annotation_buffer_t  *buffer_ptr;
      annotation_t  *annotation_ptr;

      buffer_ptr =  &(_annotation_shared->buffers[_annotation_shared->current]);
      annotation_ptr =  &(buffer_ptr->buffer[buffer_ptr->count]);

      // TODO: Unify the MBB_DEFAULT & BB_DEFAULT types together. 
      annotation_ptr->type = db->Type;
      annotation_ptr->db = db;
      // Get the Return Address of the Caller of the Current Function.
      annotation_ptr->bb_addr = (uintptr_t)__builtin_return_address (1);

      if(_no_thread)
      {
        synchronize_no_thread();
        return;
      }

      _annotation_shared->db_count++;
      buffer_ptr->count++;
      ASSERT(buffer_ptr->count <= BUFFER_SIZE);
      // The computation treshold is used to break
      // dead lock in the annotated software.
      if( (_annotation_shared->db_count >= COMPUTATION_THRESHOLD) || (buffer_ptr->count >= BUFFER_SIZE))
      {
        synchronize();
      }
    }

    void ExecutionSpy::annotate_return()
    {
      annotation_buffer_t  *buffer_ptr;

      buffer_ptr =  &(_annotation_shared->buffers[_annotation_shared->current]);
      buffer_ptr->buffer[buffer_ptr->count].type = BB_RETURN;

      if(_no_thread)
      {
        synchronize_no_thread();
        return;
      }

      _annotation_shared->db_count++;
      buffer_ptr->count++;
      ASSERT(buffer_ptr->count <= BUFFER_SIZE);
      // The computation trshold is used to break
      // dead lock in the annotated software.
      if( (_annotation_shared->db_count >= COMPUTATION_THRESHOLD) || (buffer_ptr->count >= BUFFER_SIZE))
      {
        synchronize();
      }
    }
  }
}
