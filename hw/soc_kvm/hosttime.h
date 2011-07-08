/*
 *  Copyright (c) 2010 TIMA Laboratory
 *
 *  This file is part of Rabbits.
 *
 *  Rabbits is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Rabbits is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Rabbits.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _HOSTTIME_H_
#define _HOSTTIME_H_

#include <stdint.h>

#define HOSTTIME_BASEPORT 0x5000

// This value is used to calculate the profiling overhead.
// This particular value is for KVM Platform on my Desktop Machine.
//#define ONE_PROFILE_CALL_COST 0.000010975
//#define ONE_PROFILE_CALL_COST   0.00000309
//#define ONE_PROFILE_CALL_COST   0.000003085
//#define ONE_PROFILE_CALL_COST 0.000003075
#define ONE_PROFILE_CALL_COST 0.00000309

typedef enum hosttime_port_offset
{
    HOSTTIME_CURRENT_TIME = 0,
    HOSTTIME_COMP_START = 1,
    HOSTTIME_COMP_END = 2,
    HOSTTIME_IO_START = 3,
    HOSTTIME_IO_END = 4,
    HOSTTIME_FLUSH_DATA = 5
} hosttime_port_t;

typedef struct hosttime {
    FILE                *host_file;

    double               m_comp_start;
    double               m_comp_end;
    double               m_comp_total;
    double               m_io_start;
    double               m_io_end;
    double               m_io_total;

    // Counters for Sanity Check of Profile Info
    uint32_t             m_comp_start_count;
    uint32_t             m_comp_end_count;
    uint32_t             m_io_start_count;
    uint32_t             m_io_end_count;
} hosttime_t;

int init_hosttime(hosttime_t* phosttime, const char *filename);
int hosttime_handler(void *opaque, int size, int is_write, uint64_t addr, uint64_t *value);
void close_hosttime(hosttime_t* phosttime);

#endif

