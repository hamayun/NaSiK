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
#include <time.h>

#define HOSTTIME_BASEPORT 0x5000

// This value is used to calculate the profiling overhead.
// This particular value is for KVM Platform on my Desktop Machine.
// Use selfProf Application to get this Value.
#define ONE_PROFILE_CALL_COST 0.000001614

typedef enum hosttime_port_value
{
    HOSTTIME_CURRENT_TIME = 0,
    HOSTTIME_COMP_START = 1,
    HOSTTIME_COMP_END = 2,
    HOSTTIME_IO_START = 3,
    HOSTTIME_IO_END = 4,
    HOSTTIME_FLUSH_DATA = 5,
    HOSTTIME_PROFILE_COST_FACTOR = 6,
    HOSTTIME_PROFILE_TIME_DELTA = 7,
    HOSTTIME_ERASE_MEMORY = 8,
    HOSTTIME_VERIFY_MEMORY = 9
} hosttime_port_value_t;

typedef struct hosttime {
    FILE                *m_host_file;
    clockid_t            m_clk_id;
    struct timespec      m_clk_res;                    // clock resolution

    struct timespec      m_comp_start_ts;              // computation start timestamp
    struct timespec      m_comp_end_ts;                // computation end timestamp
    struct timespec      m_comp_total_ts;              // computation total timestamp

    struct timespec      m_io_start_ts;                // i/o start timestamp
    struct timespec      m_io_end_ts;                  // i/o end timestamp
    struct timespec      m_io_total_ts;                // i/o total timestamp

    // Counters for Sanity Check of Profile Info
    uint32_t             m_comp_start_count;
    uint32_t             m_comp_end_count;
    uint32_t             m_io_start_count;
    uint32_t             m_io_end_count;

    uint8_t              m_estimate_cost_factor;
    struct timespec      m_first_ts;
    struct timespec      m_last_ts;

    struct timespec      m_prev_ts;                   // For calculating delta time
    int32_t              m_delta_count;
} hosttime_t;

int32_t init_hosttime(hosttime_t* pht, const char *filename);
int32_t hosttime_handler(void *opaque, int32_t size, int32_t is_write, uint64_t addr, uint64_t *value);
void close_hosttime(hosttime_t* pht);

#endif

