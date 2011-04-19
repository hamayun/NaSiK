/****h* core/time
 * SUMMARY
 * Time management.
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

#ifndef DNA_CORE_TIME_PUBLIC_H
#define DNA_CORE_TIME_PUBLIC_H

#include <stdint.h>
#include <DnaTools/DnaTools.h>

#define DNA_PERIODIC_ALARM		0x1
#define DNA_ONE_SHOT_ALARM		0x2
#define DNA_RELATIVE_ALARM		0x4
#define DNA_ABSOLUTE_ALARM		0x8

/****t* time/bigtime_t
 * SUMMARY
 * 64-bit precision time type.
 *
 * SOURCE
 */

typedef int64_t bigtime_t;

/*
 ****/

/****t* time/alarm_callback_t
 * SUMMARY
 *
 * Alarm callback type.
 *
 * SOURCE
 */

typedef status_t (* alarm_callback_t) (void * data);

/*
 ****/

extern int32_t time_set_alarm (bigtime_t deadline, int32_t mode,
    alarm_callback_t callback, void * data);
extern status_t time_cancel_alarm (int32_t alarm);
extern status_t time_reset_alarm (int32_t alarm);

#endif

