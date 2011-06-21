/*************************************************************************************
 * File   : execution_spy.h,     
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

#ifndef __EXECUTION_SPY_H__
#define __EXECUTION_SPY_H__

#include "annotation.h"
#include "analyzer.h"
#include "semaphore.h"
#include <link.h>
#include <string>

#define ANALYZE_OPTION true
#define ONLINE_OPTION true
#define NO_THREAD_OPTION false        // if true analyzer thread will *NOT* be created.

#define COMPUTATION_THRESHOLD   128
#define BUFFER_THRESHOLD        ((BUFFER_SIZE) - (COMPUTATION_THRESHOLD) - 1)

typedef struct {
  annotation_buffer_t   buffers[NB_BUFFER];
  volatile uint32_t     current;            // Current used buffer
  volatile uint32_t     db_count;           // Amount of uncomputed db in the current buffer
  Analyzer              *analyzer;          // Analyzer in charge of these buffers
} annotation_buffer_set_t;

extern "C" void mbb_annotation(annotation_db_t *db);

class ExecutionSpy
{
  friend void mbb_annotation(annotation_db_t *db);

  public:
      ExecutionSpy(bool analyze, bool online_analyze, bool no_thread,
                   char *elf_file, uintptr_t app_base_addr);
      ~ExecutionSpy(void);

      void synchronize(void);
      void annotate(void *vm_addr, db_buffer_desc_t *pbuff_desc);
      void close(void);

  protected:
      void register_self(char *elf_file, uintptr_t app_base_addr);
      virtual void compute(annotation_t *trace, uint32_t count) = 0;

  // Annotation buffer management
  private:
      annotation_buffer_set_t     *m_annotation_buffer_set;
      bool                         m_no_thread;
      bool                         m_analyze;
      bool                         m_online_analyze;
      void synchronize_no_thread(void);
};

#endif				// __EXECUTION_SPY_H__
