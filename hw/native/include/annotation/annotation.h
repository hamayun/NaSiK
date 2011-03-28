/*************************************************************************************
 * File   : annotation.h,     
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

#ifndef __ANNOTATION_H__
#define __ANNOTATION_H__

#include "stdint.h"
#include "semaphore.h"

#define BUFFER_SIZE             (1024 * 128)
#define NB_BUFFER               3

namespace native
{
  namespace annotation
  {

    typedef enum {
      BB_DEFAULT = 0,
      BB_ENTRY = 1,
      BB_RETURN = 2
    } BB_TYPE;

    typedef struct {
      uint32_t          Type;
      uint32_t          InstructionCount;
      uint32_t          CycleCount;
      uint32_t          LoadCount;
      uint32_t          StoreCount;
    } annotation_db_t;

    typedef struct {
      uintptr_t         eu;        // EU identifier (address)
      uintptr_t         thread;    // Thread identifier (address); infact its the corresponding eu_base object (Needs Verification !!!)
      uintptr_t         bb_addr;   // Native basic bloc address
      uintptr_t         caller_addr;   // Address of the caller
      annotation_db_t   *db;       // Pointer to the annotation data 
      uint8_t           type;       // This is the type of Annotation; Copied from Type found in AnnotationDB
    } annotation_t;

    typedef struct {
      sem_t                  sem;
      sem_t                  ack;
      annotation_t           buffer[BUFFER_SIZE];
      volatile uint32_t      count;
    } annotation_buffer_t;

  } // end namespace annotation
} // end namespace native

#endif				// __ANNOTATION_H__
