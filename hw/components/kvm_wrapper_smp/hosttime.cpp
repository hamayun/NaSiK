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

#include <stdio.h>
#include <stdlib.h>
#include "hosttime.h"

#if 0
double get_clock(void) {
  /* Unix or Linux: use resource usage */
  struct rusage t;
  double procTime;
  /* (1) Get the rusage data structure at this moment (man getrusage) */
  getrusage(0,&t);
  /* (2) What is the elapsed time ? - CPU time = User time + System time */
  /* (2a) Get the seconds */
  procTime = t.ru_utime.tv_sec + t.ru_stime.tv_sec;
  /* (2b) More precisely! Get the microseconds part ! */
  return ( procTime + (t.ru_utime.tv_usec + t.ru_stime.tv_usec) * 1e-6 ) ;
}
#endif

hosttime::hosttime(const char *filename)
{
    int32_t status;

    m_host_file = fopen (filename, "wb");
    if(m_host_file == NULL)
    {
        fprintf(stderr, "Error in %s: file %s can not be opened!\n", __func__, filename);
    }

    m_clk_id = CLOCK_PROCESS_CPUTIME_ID;  // CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID; CLOCK_THREAD_CPUTIME_ID;
    status = clock_getres(m_clk_id, & m_clk_res);
    if(status){
        fprintf(stderr, "%s: Error Getting Clock Resolution!\n", __func__);
    }

    m_comp_total_ts.tv_sec = 0;
    m_comp_total_ts.tv_nsec = 0;
    m_io_total_ts.tv_sec = 0;
    m_io_total_ts.tv_nsec = 0;

    m_comp_start_count = 0;
    m_comp_end_count = 0;
    m_io_start_count = 0;
    m_io_end_count = 0;

    m_estimate_cost_factor = 0;
    m_first_ts.tv_sec = 0;
    m_first_ts.tv_nsec = 0;
    m_last_ts.tv_sec = 0;
    m_last_ts.tv_nsec = 0;

    m_prev_ts.tv_sec = 0;
    m_prev_ts.tv_nsec = 0;
    m_delta_count = -1;
}

hosttime::~hosttime()
{
    if (m_host_file)
    {
        fclose (m_host_file);
        m_host_file = NULL;
    }

    // Make the Simulation Stop
    //simulation_stop(SIGINT);
    //exit(0);
}

void hosttime::hosttime_handler(uint32_t value)
{
    hosttime_request_t request = (hosttime_request_t) value;

    if(m_host_file == NULL)
    {
        fprintf(stderr, "Invalide Hosttime File Handle\n");
        return;
    }

    switch (request)
    {
        case HOSTTIME_CURRENT_TIME:
            {
                struct timespec        curr_time;
                clock_gettime(m_clk_id, & curr_time);
                fprintf(m_host_file, "Current Time is : %ld.%09ld\n", curr_time.tv_sec, curr_time.tv_nsec);
                fprintf(stderr,           "Current Time is : %ld.%09ld\n", curr_time.tv_sec, curr_time.tv_nsec);
                fflush(m_host_file);
            }
            break;

        case HOSTTIME_COMP_START:
            m_comp_start_count++;
            clock_gettime(m_clk_id, & m_comp_start_ts);
            break;

        case HOSTTIME_COMP_END:
            if(m_comp_start_count <= m_comp_end_count)
            {
                fprintf(stderr, "Computation Profile Wrong Usage Order: m_comp_start_count = %d, m_comp_end_count = %d\n",
                                m_comp_start_count, m_comp_end_count);
                return;
            }

            m_comp_end_count++;
            clock_gettime(m_clk_id, & m_comp_end_ts);
            if(m_comp_end_ts.tv_nsec < m_comp_start_ts.tv_nsec)
            {
                m_comp_end_ts.tv_nsec += 1e9 / m_clk_res.tv_nsec;
                m_comp_end_ts.tv_sec--;
            }

            m_comp_total_ts.tv_sec  += (m_comp_end_ts.tv_sec  - m_comp_start_ts.tv_sec);
            m_comp_total_ts.tv_nsec += (m_comp_end_ts.tv_nsec - m_comp_start_ts.tv_nsec);
            // Handle the Addition Overflow in Nanosecond part.
            while(m_comp_total_ts.tv_nsec - 1e9 > 0)
            {
                m_comp_total_ts.tv_nsec -= 1e9;
                m_comp_total_ts.tv_sec++;
                /*
                fprintf(stderr, "Overflow: ETS (%ld:%09ld), STS (%ld:%09ld) => TTS (%ld:%09ld)\n",
                        m_comp_end_ts.tv_sec, m_comp_end_ts.tv_nsec,
                        m_comp_start_ts.tv_sec, m_comp_start_ts.tv_nsec,
                        m_comp_total_ts.tv_sec, m_comp_total_ts.tv_nsec);
                */
            }
            break;

        case HOSTTIME_IO_START:
            m_io_start_count++;
            clock_gettime(m_clk_id, & m_io_start_ts);
            break;

        case HOSTTIME_IO_END:
            if(m_io_start_count <= m_io_end_count)
            {
                fprintf(stderr, "I/O Profile Wrong Usage Order: m_io_start_count = %d, m_io_end_count = %d\n",
                                m_io_start_count, m_io_end_count);
                return;
            }

            m_io_end_count++;
            clock_gettime(m_clk_id, & m_io_end_ts);
            if(m_io_end_ts.tv_nsec < m_io_start_ts.tv_nsec)
            {
                m_io_end_ts.tv_nsec += 1e9 / m_clk_res.tv_nsec;
                m_io_end_ts.tv_sec--;
            }

            m_io_total_ts.tv_sec  += (m_io_end_ts.tv_sec  - m_io_start_ts.tv_sec);
            m_io_total_ts.tv_nsec += (m_io_end_ts.tv_nsec - m_io_start_ts.tv_nsec);
            // Handle the Addition Overflow in Nanosecond part.
            while(m_io_total_ts.tv_nsec - 1e9 > 0)
            {
                m_io_total_ts.tv_nsec -= 1e9;
                m_io_total_ts.tv_sec++;
            }
            break;

        case HOSTTIME_FLUSH_DATA:
            {
                double io_profile_overhead, comp_profile_overhead;
                double net_io_cost_ts, net_comp_cost_ts;
#ifndef VERBOSE_FLUSH
                double net_cost;
#endif
                struct timespec        total_cost;

                if(m_estimate_cost_factor)
                {
                    struct timespec        curr_time;
                    clock_gettime(m_clk_id, & curr_time);

                    m_last_ts.tv_sec = curr_time.tv_sec;
                    m_last_ts.tv_nsec = curr_time.tv_nsec;
                }

                if((m_comp_start_count != m_comp_end_count) ||
                   (m_io_start_count != m_io_end_count))
                {
                    fprintf(m_host_file, "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
                    fprintf(stderr,           "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
                }

                io_profile_overhead = ONE_PROFILE_CALL_COST * (m_io_start_count + m_io_end_count);
                net_io_cost_ts = (m_io_total_ts.tv_sec + m_io_total_ts.tv_nsec * 1e-9) - io_profile_overhead;

                comp_profile_overhead = ONE_PROFILE_CALL_COST * (m_comp_start_count + m_comp_end_count);
                net_comp_cost_ts = (m_comp_total_ts.tv_sec + m_comp_total_ts.tv_nsec * 1e-9) - comp_profile_overhead;

                total_cost.tv_sec  = m_io_total_ts.tv_sec + m_comp_total_ts.tv_sec;
                total_cost.tv_nsec = m_io_total_ts.tv_nsec + m_comp_total_ts.tv_nsec;
                while(total_cost.tv_nsec - 1e9 > 0)
                {
                    total_cost.tv_nsec -= 1e9;
                    total_cost.tv_sec++;
                }

#ifdef VERBOSE_FLUSH
                fprintf(stderr,
                        "Total IO Time                : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        m_io_total_ts.tv_sec, m_io_total_ts.tv_nsec,
                        m_io_start_count, m_io_end_count,
                        io_profile_overhead, net_io_cost_ts);

                fprintf(stderr,
                        "Total Computation Time       : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        m_comp_total_ts.tv_sec, m_comp_total_ts.tv_nsec,
                        m_comp_start_count, m_comp_end_count,
                        comp_profile_overhead, net_comp_cost_ts);

                fprintf(stderr,
                        "Total I/O + Computation Time : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        total_cost.tv_sec, total_cost.tv_nsec,
                        m_io_start_count + m_comp_start_count,
                        m_io_end_count + m_comp_end_count,
                        io_profile_overhead + comp_profile_overhead,
                        net_io_cost_ts + net_comp_cost_ts);

                fprintf(m_host_file,
                        "Total IO Time                : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        m_io_total_ts.tv_sec, m_io_total_ts.tv_nsec,
                        m_io_start_count, m_io_end_count,
                        io_profile_overhead, net_io_cost_ts);

                fprintf(m_host_file,
                        "Total Computation Time       : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        m_comp_total_ts.tv_sec, m_comp_total_ts.tv_nsec,
                        m_comp_start_count, m_comp_end_count,
                        comp_profile_overhead, net_comp_cost_ts);

                fprintf(m_host_file,
                        "Total I/O + Computation Time : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        total_cost.tv_sec, total_cost.tv_nsec,
                        m_io_start_count + m_comp_start_count,
                        m_io_end_count + m_comp_end_count,
                        io_profile_overhead + comp_profile_overhead,
                        net_io_cost_ts + net_comp_cost_ts);

#else
                net_cost = net_io_cost_ts + net_comp_cost_ts;
                fprintf(stderr, "NetCost: %4.6f\n", net_cost);
                fprintf(m_host_file, "NetCost: %4.6f\n", net_cost);
#endif

                if(m_estimate_cost_factor)
                {
                    uint32_t cost_nsec, cost_sec;
                    double prof_call_factor = 0.0, total_cost = 0.0;
                    if((m_last_ts.tv_nsec - m_first_ts.tv_nsec) < 0)
                    {
                        m_last_ts.tv_nsec += 1e9;
                        m_last_ts.tv_sec--;
                    }

                    cost_sec  = m_last_ts.tv_sec - m_first_ts.tv_sec;
                    cost_nsec = m_last_ts.tv_nsec - m_first_ts.tv_nsec;

                    total_cost = cost_sec + (cost_nsec * 1e-9);
                    prof_call_factor = total_cost / (m_io_start_count + m_io_end_count +
                                                     m_comp_start_count + m_comp_end_count);

                    prof_call_factor /= 2.0;        // We do this because half of the Guest2Host mode switch time can't be measured.
                    fprintf(stderr, "Profile Call Factor Estimate : %0.9f\n", prof_call_factor);
                }

                fflush(m_host_file);
                this->~hosttime();
            }
            break;

        case HOSTTIME_PROFILE_COST:
            {
                struct timespec        curr_time;

                m_estimate_cost_factor = 1;
                clock_gettime(m_clk_id, & curr_time);
                m_first_ts.tv_sec = curr_time.tv_sec;
                m_first_ts.tv_nsec = curr_time.tv_nsec;
            }
            break;

        case HOSTTIME_PROFILE_TIME_DELTA:
            {
                struct timespec        curr_time;
                struct timespec        curr_time_pristine;
                struct timespec        delta_time;

                clock_gettime(m_clk_id, & curr_time);

                // Save current time for later copy to previous time stamp.
                curr_time_pristine.tv_sec = curr_time.tv_sec;
                curr_time_pristine.tv_nsec = curr_time.tv_nsec;

                if((curr_time.tv_nsec - m_prev_ts.tv_nsec) < 0)
                {
                    curr_time.tv_nsec += 1e9;
                    curr_time.tv_sec--;
                }

                delta_time.tv_sec  = curr_time.tv_sec - m_prev_ts.tv_sec;
                delta_time.tv_nsec = curr_time.tv_nsec - m_prev_ts.tv_nsec;

                fprintf(stderr,
                        "Current Time                 : %ld.%09ld    Delta[%2d] : %ld.%09ld\n",
                        curr_time_pristine.tv_sec,  curr_time_pristine.tv_nsec,
                        m_delta_count, delta_time.tv_sec, delta_time.tv_nsec);
                fprintf(m_host_file,
                        "Current Time                 : %ld.%09ld    Delta[%2d] : %ld.%09ld\n",
                        curr_time_pristine.tv_sec,  curr_time_pristine.tv_nsec,
                        m_delta_count, delta_time.tv_sec, delta_time.tv_nsec);

                // Save for Next Delta Time
                m_prev_ts.tv_sec = curr_time_pristine.tv_sec;
                m_prev_ts.tv_nsec = curr_time_pristine.tv_nsec;
                m_delta_count++;
            }
            break;

        default:
            printf ("%s: Bad Port Value = %d\n", __func__, request);
            exit (1);
            break;
    }

    return;
}
