/*************************************************************************************
 * File   : execution_spy.cpp,
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

#include "execution_spy.h"
#include "analyzer.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <link.h>
#include "errno.h"
#include "pthread.h"
#include "semaphore.h"
#include <vector>
#include <iostream>
#include <string.h>
#include <iostream>

using namespace std;

#define DEBUG_EXECUTION_SPY false
#define DOUT_FCT  if(DEBUG_EXECUTION_SPY) std::cout << __func__ << ": "
#define DOUT      if(DEBUG_EXECUTION_SPY) std::cout

ExecutionSpy::ExecutionSpy(bool analyze, bool online_analyze, bool no_thread,
                           char *elf_file, uintptr_t app_base_addr)
   :m_no_thread(no_thread),
    m_analyze(analyze),
    m_online_analyze(online_analyze)
{
    //std::cout << "Registering to the execution_spy" << std::endl;
    ASSERT(elf_file != NULL);
    ASSERT(app_base_addr != 0);

    // Create Annotation Map with Buffers and Analyzer Object
    register_self(elf_file, app_base_addr);
}

ExecutionSpy::~ExecutionSpy()
{}

void ExecutionSpy::synchronize_no_thread()
{
  annotation_buffer_t  *buffer_ptr;
  annotation_t         *annotation_ptr;

  buffer_ptr = &(m_annotation_buffer_set->buffers[0]);
  annotation_ptr = &(buffer_ptr->buffer[buffer_ptr->count]);
  buffer_ptr->count = ((buffer_ptr->count) + 1) % BUFFER_SIZE;
  // Compute One annotation information
  compute(annotation_ptr, 1);

  // In case of no_thread option, we don't care about the online analyze option.
  // And always push annotations to anaylzer in case analyze option is set.
  if(m_analyze == true)
  {
    DOUT_FCT << "push annotation" << std::endl;
    m_annotation_buffer_set->analyzer->push(annotation_ptr);
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
  if(m_no_thread)
  {
    return;
  }

  buffer_ptr =  &(m_annotation_buffer_set->buffers[m_annotation_buffer_set->current]);
  annotation_ptr =  &(buffer_ptr->buffer[buffer_ptr->count - m_annotation_buffer_set->db_count]);
  saved_count = m_annotation_buffer_set->db_count;

  // First, just switch to the next buffer but do not start the dump
  // of the current buffer to the file because the eu will work on
  // the trace data's.
  if( buffer_ptr->count >= BUFFER_THRESHOLD)
  {
    next_buffer = (m_annotation_buffer_set->current + 1) % NB_BUFFER;
    DOUT_FCT << this << " wait ack " << next_buffer << std::endl;
    sem_wait(&(m_annotation_buffer_set->buffers[next_buffer].ack));
    previous_buffer = m_annotation_buffer_set->current;
    DOUT_FCT << this << " switch buffer " << m_annotation_buffer_set->current << " -> " << next_buffer << std::endl;
    m_annotation_buffer_set->current = next_buffer;
    buffer_switched = 1;
  }
  m_annotation_buffer_set->db_count = 0;

  // Compute annotation information
  compute(annotation_ptr, saved_count);

  // Now we can unlock the thread to dump the trace.
  if(buffer_switched)
  {
    DOUT_FCT << this << " unlock buffer " << previous_buffer << " (" << &(m_annotation_buffer_set->buffers[previous_buffer].sem) << ")" << std::endl;
    sem_post(&(m_annotation_buffer_set->buffers[previous_buffer].sem));
  }
}

void ExecutionSpy::register_self(char * elf_file, uintptr_t app_base_addr)
{
    DOUT_FCT << ": application " << elf_file << std::endl;
    m_annotation_buffer_set = new annotation_buffer_set_t;
    memset(m_annotation_buffer_set, 0x0, sizeof(annotation_buffer_set_t));

    m_annotation_buffer_set->current = 0;
    m_annotation_buffer_set->db_count = 0;

    for( int i = 0 ; i < NB_BUFFER ; i ++)
    {
      sem_init(&(m_annotation_buffer_set->buffers[i].sem), 0, 0);
      sem_init(&(m_annotation_buffer_set->buffers[i].ack), 0, 1);
      m_annotation_buffer_set->buffers[i].count = 0;
    }
    sem_wait(&(m_annotation_buffer_set->buffers[m_annotation_buffer_set->current].ack));
    m_annotation_buffer_set->analyzer = new Analyzer(elf_file, app_base_addr, m_annotation_buffer_set->buffers,
                                                 m_no_thread, m_analyze, m_online_analyze);
}

void ExecutionSpy::close()
{
    m_annotation_buffer_set->analyzer->ending();
}

void ExecutionSpy::annotate(void *vm_addr, db_buffer_desc_t *pbuff_desc)
{
  annotation_buffer_t  *buffer_ptr;
  annotation_t  *annotation_ptr;
  annotation_db_t *db;

  while(pbuff_desc->StartIndex != pbuff_desc->EndIndex)
  {
    buffer_ptr =  &(m_annotation_buffer_set->buffers[m_annotation_buffer_set->current]);
    annotation_ptr =  &(buffer_ptr->buffer[buffer_ptr->count]);

    // Get pointer to the annotation db;
    db = (annotation_db_t *)((uint32_t)vm_addr + (uint32_t)pbuff_desc->Buffer[pbuff_desc->StartIndex].pdb);

    // TODO: Unify the MBB_DEFAULT & BB_DEFAULT types together.
    annotation_ptr->type = db->Type;
    annotation_ptr->db = db;
    // Get the Return Address of the Caller of the mbb_annotation Function.
    annotation_ptr->bb_addr = (uint32_t) db->FuncAddr;
    annotation_ptr->thread = (uint32_t) pbuff_desc->Buffer[pbuff_desc->StartIndex].thread_context;

    pbuff_desc->StartIndex = (pbuff_desc->StartIndex + 1) % pbuff_desc->Capacity;

    /* We can use a Static Counter Here that will correspond with the Annotation Push
     * count inside the Analyzer Thread for debugging the Thread Contexts Switches.
     * An Examples is given below.
    {
        static int        annotate_count = 0;
        annotate_count++;
        //if(annotate_count >= 42200 && annotate_count <= 42500)
        {
            std::cout << "annotate_count: " << std::dec << annotate_count << std::endl;
            if (m_annotation_buffer_set->analyzer)
                m_annotation_buffer_set->analyzer->print_annotation(annotation_ptr);
            else
                std::cout << "No Analyzer Assigned !!!" << std::endl;
        }
    }
    */

    if(m_no_thread)
    {
        // The buffer should contain one db entry only for this option.
        ASSERT(pbuff_desc->StartIndex == pbuff_desc->EndIndex);

        synchronize_no_thread();
        return;
    }

    m_annotation_buffer_set->db_count++;
    buffer_ptr->count++;
    ASSERT(buffer_ptr->count <= BUFFER_SIZE);

    // The computation treshold is used to break dead lock in the annotated software.
    if( (m_annotation_buffer_set->db_count >= COMPUTATION_THRESHOLD) || (buffer_ptr->count >= BUFFER_SIZE))
    {
        synchronize();
    }
  }
}

