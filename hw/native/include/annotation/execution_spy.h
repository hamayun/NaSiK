/*************************************************************************************
 * File   : execution_spy.h,     
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

#ifndef __EXECUTION_SPY_H__
#define __EXECUTION_SPY_H__

#include "annotation.h"
#include "analyzer.h"
#include "semaphore.h"
#include <link.h>
#include <string>

#define COMPUTATION_THRESHOLD   128
#define BUFFER_THRESHOLD        ((BUFFER_SIZE) - (COMPUTATION_THRESHOLD) - 1)

namespace native
{
  namespace annotation
  {

    typedef void (* simu_compute_fct_t) (annotation_t * buffer, uint32_t count);

    typedef struct {
      annotation_buffer_t   buffers[NB_BUFFER];
      volatile uint32_t     current;            // Current used buffer
      volatile uint32_t     db_count;           // Amount of uncomputed db in the current buffer
      Analyzer              *analyzer;          // Analyzer in charge of these buffers
    } annotation_shared_t;

    extern "C" void mbb_annotation(annotation_db_t *db);

    class ExecutionSpy
    {
      friend void mbb_annotation(annotation_db_t *db);

      public:
      ExecutionSpy(void);
      ~ExecutionSpy(void);

      void synchronize(void);
      void annotate(annotation_db_t *db);
      void annotate_entry();
      void annotate_return();
      static void close(void);

      protected:
      void register_self(link_map *linkmap);

      virtual void compute(annotation_t *trace, uint32_t count) = 0;

      // Application registration
      private:
      static std::map< std::string , annotation_shared_t* >    _annotation_shared_map;

      // Annotation buffer management
      private:
      annotation_shared_t         *_annotation_shared;
      link_map                    *_link_map;

      private:
      static bool                 _no_thread;
      static bool                 _analyze;
      void synchronize_no_thread(void);

    };

  } // end namespace annotation
} // end namespace native

#endif				// __EXECUTION_SPY_H__
