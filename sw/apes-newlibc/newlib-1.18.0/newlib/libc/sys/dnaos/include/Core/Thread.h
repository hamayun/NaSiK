/****h* core/thread
 * SUMMARY
 * Thread management.
 ****
 * Copyright (C) 2007 TIMA Laboratory
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DNA_CORE_THREAD_PUBLIC_H
#define DNA_CORE_THREAD_PUBLIC_H

#include <stdint.h>

#include <Core/Time.h>
#include <DnaTools/DnaTools.h>

/****v* thread/thread_status
 * SUMMARY
 * Available thread status.
 *
 * SOURCE
 */

typedef enum _thread_status
{
  DNA_THREAD_SLEEP      = 0xBAFF,
  DNA_THREAD_READY      = 0xFACE,
  DNA_THREAD_RUNNING    = 0xBEEF,
  DNA_THREAD_WAIT       = 0xBADD,
  DNA_THREAD_ENDED      = 0xDEAD
}
thread_status_t;

/*
 ****/

/****t* types/thread_resource_t
 * SUMMARY
 * Type of the resource for which a thread can wait.
 *
 * SOURCE
 */

typedef enum
{
  DNA_NO_RESOURCE,
  DNA_RESOURCE_SEMAPHORE,
  DNA_RESOURCE_THREAD
}
thread_resource_t;

/*
 ****/

/****t* types/thread_handler_t
 * SUMMARY
 * Thread handler type.
 *
 * SOURCE
 */

typedef int32_t (* thread_handler_t) (void * args);

/*
 ****/

/****t* types/thread_info_t
 * SUMMARY
 * Thread information type.
 *
 * SOURCE
 */

typedef struct _thread_info
{
  char name[DNA_NAME_LENGTH];

  int32_t cpu_id;
  int32_t group;
  int32_t cpu_affinity;
  int32_t sem_tokens;

  thread_status_t status;
  thread_resource_t resource;
  int32_t resource_id;

  bigtime_t kernel_time;
  bigtime_t user_time;

  void * stack_base;
  uint32_t stack_size;
}
thread_info_t;

/*
 ****/

extern status_t thread_find (char *  name, int32_t * tid);
extern status_t thread_get_info (int32_t id, thread_info_t * info);

#endif

