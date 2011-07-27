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

int32_t init_hosttime(hosttime_t* pht, const char *filename)
{
    int32_t status;

    pht->m_host_file = fopen (filename, "wb");
    if(pht->m_host_file == NULL)
    {
        fprintf(stderr, "Error in %s: file %s can not be opened!\n", __func__, filename);
        return (1);
    }

    pht->m_clk_id = CLOCK_PROCESS_CPUTIME_ID;  // CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_PROCESS_CPUTIME_ID; CLOCK_THREAD_CPUTIME_ID;
    status = clock_getres(pht->m_clk_id, & pht->m_clk_res);
    if(status){
        fprintf(stderr, "%s: Error Getting Clock Resolution!\n", __func__);
        return (1);
    }

    pht->m_comp_total_ts.tv_sec = 0;
    pht->m_comp_total_ts.tv_nsec = 0;
    pht->m_io_total_ts.tv_sec = 0;
    pht->m_io_total_ts.tv_nsec = 0;

    pht->m_comp_start_count = 0;
    pht->m_comp_end_count = 0;
    pht->m_io_start_count = 0;
    pht->m_io_end_count = 0;

    pht->m_estimate_cost_factor = 0;
    pht->m_first_ts.tv_sec = 0;
    pht->m_first_ts.tv_nsec = 0;
    pht->m_last_ts.tv_sec = 0;
    pht->m_last_ts.tv_nsec = 0;

    pht->m_prev_ts.tv_sec = 0;
    pht->m_prev_ts.tv_nsec = 0;
    pht->m_delta_count = -1;
    return 0;
}

void close_hosttime(hosttime_t* pht)
{
    if (pht->m_host_file)
    {
        fclose (pht->m_host_file);
        pht->m_host_file = NULL;
    }

    // Make the Simulation Stop
    //simulation_stop(SIGINT);
    exit(0);
}

int32_t hosttime_handler(void *opaque, int32_t size, int32_t is_write, uint64_t addr, uint64_t *value)
{
    hosttime_t* pht = (hosttime_t *) opaque;
    hosttime_port_value_t port_value = (hosttime_port_value_t) *value;

    if((!pht) || pht->m_host_file == NULL)
    {
        fprintf(stderr, "Invalide Hosttime File Handle\n");
        return (1);
    }

    switch (port_value)
    {
        case HOSTTIME_CURRENT_TIME:
            {
                struct timespec        curr_time;
                clock_gettime(pht->m_clk_id, & curr_time);
                fprintf(pht->m_host_file, "Current Time is : %ld.%09ld\n", curr_time.tv_sec, curr_time.tv_nsec);
                fprintf(stderr,           "Current Time is : %ld.%09ld\n", curr_time.tv_sec, curr_time.tv_nsec);
                fflush(pht->m_host_file);
            }
            break;

        case HOSTTIME_COMP_START:
            pht->m_comp_start_count++;
            clock_gettime(pht->m_clk_id, & pht->m_comp_start_ts);
            break;

        case HOSTTIME_COMP_END:
            if(pht->m_comp_start_count <= pht->m_comp_end_count)
            {
                fprintf(stderr, "Computation Profile Wrong Usage Order: m_comp_start_count = %d, m_comp_end_count = %d\n",
                                pht->m_comp_start_count, pht->m_comp_end_count);
                return (2);
            }

            pht->m_comp_end_count++;
            clock_gettime(pht->m_clk_id, & pht->m_comp_end_ts);
            if(pht->m_comp_end_ts.tv_nsec < pht->m_comp_start_ts.tv_nsec)
            {
                pht->m_comp_end_ts.tv_nsec += 1e9 / pht->m_clk_res.tv_nsec;
                pht->m_comp_end_ts.tv_sec--;
            }

            pht->m_comp_total_ts.tv_sec  += (pht->m_comp_end_ts.tv_sec  - pht->m_comp_start_ts.tv_sec);
            pht->m_comp_total_ts.tv_nsec += (pht->m_comp_end_ts.tv_nsec - pht->m_comp_start_ts.tv_nsec);
            // Handle the Addition Overflow in Nanosecond part.
            while(pht->m_comp_total_ts.tv_nsec - 1e9 > 0)
            {
                pht->m_comp_total_ts.tv_nsec -= 1e9;
                pht->m_comp_total_ts.tv_sec++;
                /*
                fprintf(stderr, "Overflow: ETS (%ld:%09ld), STS (%ld:%09ld) => TTS (%ld:%09ld)\n",
                        pht->m_comp_end_ts.tv_sec, pht->m_comp_end_ts.tv_nsec,
                        pht->m_comp_start_ts.tv_sec, pht->m_comp_start_ts.tv_nsec,
                        pht->m_comp_total_ts.tv_sec, pht->m_comp_total_ts.tv_nsec);
                */
            }
            break;

        case HOSTTIME_IO_START:
            pht->m_io_start_count++;
            clock_gettime(pht->m_clk_id, & pht->m_io_start_ts);
            break;

        case HOSTTIME_IO_END:
            if(pht->m_io_start_count <= pht->m_io_end_count)
            {
                fprintf(stderr, "I/O Profile Wrong Usage Order: m_io_start_count = %d, m_io_end_count = %d\n",
                                pht->m_io_start_count, pht->m_io_end_count);
                return (3);
            }

            pht->m_io_end_count++;
            clock_gettime(pht->m_clk_id, & pht->m_io_end_ts);
            if(pht->m_io_end_ts.tv_nsec < pht->m_io_start_ts.tv_nsec)
            {
                pht->m_io_end_ts.tv_nsec += 1e9 / pht->m_clk_res.tv_nsec;
                pht->m_io_end_ts.tv_sec--;
            }

            pht->m_io_total_ts.tv_sec  += (pht->m_io_end_ts.tv_sec  - pht->m_io_start_ts.tv_sec);
            pht->m_io_total_ts.tv_nsec += (pht->m_io_end_ts.tv_nsec - pht->m_io_start_ts.tv_nsec);
            // Handle the Addition Overflow in Nanosecond part.
            while(pht->m_io_total_ts.tv_nsec - 1e9 > 0)
            {
                pht->m_io_total_ts.tv_nsec -= 1e9;
                pht->m_io_total_ts.tv_sec++;
            }
            break;

        case HOSTTIME_FLUSH_DATA:
            {
                double io_profile_overhead, comp_profile_overhead;
                double net_io_cost_ts, net_comp_cost_ts;
                struct timespec        total_cost;

                if(pht->m_estimate_cost_factor)
                {
                    struct timespec        curr_time;
                    clock_gettime(pht->m_clk_id, & curr_time);

                    pht->m_last_ts.tv_sec = curr_time.tv_sec;
                    pht->m_last_ts.tv_nsec = curr_time.tv_nsec;
                }

                if((pht->m_comp_start_count != pht->m_comp_end_count) ||
                   (pht->m_io_start_count != pht->m_io_end_count))
                {
                    fprintf(pht->m_host_file, "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
                    fprintf(stderr,           "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
                }

                io_profile_overhead = ONE_PROFILE_CALL_COST * (pht->m_io_start_count + pht->m_io_end_count);
                net_io_cost_ts = (pht->m_io_total_ts.tv_sec + pht->m_io_total_ts.tv_nsec * 1e-9) - io_profile_overhead;

                comp_profile_overhead = ONE_PROFILE_CALL_COST * (pht->m_comp_start_count + pht->m_comp_end_count);
                net_comp_cost_ts = (pht->m_comp_total_ts.tv_sec + pht->m_comp_total_ts.tv_nsec * 1e-9) - comp_profile_overhead;

                total_cost.tv_sec  = pht->m_io_total_ts.tv_sec + pht->m_comp_total_ts.tv_sec;
                total_cost.tv_nsec = pht->m_io_total_ts.tv_nsec + pht->m_comp_total_ts.tv_nsec;
                while(total_cost.tv_nsec - 1e9 > 0)
                {
                    total_cost.tv_nsec -= 1e9;
                    total_cost.tv_sec++;
                }

                fprintf(stderr,
                        "Total IO Time                : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        pht->m_io_total_ts.tv_sec, pht->m_io_total_ts.tv_nsec,
                        pht->m_io_start_count, pht->m_io_end_count,
                        io_profile_overhead, net_io_cost_ts);

                fprintf(stderr,
                        "Total Computation Time       : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        pht->m_comp_total_ts.tv_sec, pht->m_comp_total_ts.tv_nsec,
                        pht->m_comp_start_count, pht->m_comp_end_count,
                        comp_profile_overhead, net_comp_cost_ts);

                fprintf(stderr,
                        "Total I/O + Computation Time : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        total_cost.tv_sec, total_cost.tv_nsec,
                        pht->m_io_start_count + pht->m_comp_start_count,
                        pht->m_io_end_count + pht->m_comp_end_count,
                        io_profile_overhead + comp_profile_overhead,
                        net_io_cost_ts + net_comp_cost_ts);

                fprintf(pht->m_host_file,
                        "Total IO Time                : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        pht->m_io_total_ts.tv_sec, pht->m_io_total_ts.tv_nsec,
                        pht->m_io_start_count, pht->m_io_end_count,
                        io_profile_overhead, net_io_cost_ts);

                fprintf(pht->m_host_file,
                        "Total Computation Time       : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        pht->m_comp_total_ts.tv_sec, pht->m_comp_total_ts.tv_nsec,
                        pht->m_comp_start_count, pht->m_comp_end_count,
                        comp_profile_overhead, net_comp_cost_ts);

                fprintf(pht->m_host_file,
                        "Total I/O + Computation Time : %ld.%09ld    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        total_cost.tv_sec, total_cost.tv_nsec,
                        pht->m_io_start_count + pht->m_comp_start_count,
                        pht->m_io_end_count + pht->m_comp_end_count,
                        io_profile_overhead + comp_profile_overhead,
                        net_io_cost_ts + net_comp_cost_ts);

                if(pht->m_estimate_cost_factor)
                {
                    uint32_t cost_nsec, cost_sec;
                    double prof_call_factor = 0.0, total_cost = 0.0;
                    if((pht->m_last_ts.tv_nsec - pht->m_first_ts.tv_nsec) < 0)
                    {
                        pht->m_last_ts.tv_nsec += 1e9;
                        pht->m_last_ts.tv_sec--;
                    }

                    cost_sec  = pht->m_last_ts.tv_sec - pht->m_first_ts.tv_sec;
                    cost_nsec = pht->m_last_ts.tv_nsec - pht->m_first_ts.tv_nsec;

                    total_cost = cost_sec + (cost_nsec * 1e-9);
                    prof_call_factor = total_cost / (pht->m_io_start_count + pht->m_io_end_count +
                                                     pht->m_comp_start_count + pht->m_comp_end_count);

                    prof_call_factor /= 2.0;        // We do this because half of the Guest2Host mode switch time can't be measured.
                    fprintf(stderr, "Profile Call Factor Estimate : %0.9f\n", prof_call_factor);
                }

                fflush(pht->m_host_file);
                close_hosttime(pht);
            }
            break;

        case HOSTTIME_PROFILE_COST_FACTOR:
            {
                struct timespec        curr_time;

                pht->m_estimate_cost_factor = 1;
                clock_gettime(pht->m_clk_id, & curr_time);
                pht->m_first_ts.tv_sec = curr_time.tv_sec;
                pht->m_first_ts.tv_nsec = curr_time.tv_nsec;
            }
            break;

        case HOSTTIME_PROFILE_TIME_DELTA:
            {
                struct timespec        curr_time;
                struct timespec        curr_time_pristine;
                struct timespec        delta_time;

                clock_gettime(pht->m_clk_id, & curr_time);

                // Save current time for later copy to previous time stamp.
                curr_time_pristine.tv_sec = curr_time.tv_sec;
                curr_time_pristine.tv_nsec = curr_time.tv_nsec;

                if((curr_time.tv_nsec - pht->m_prev_ts.tv_nsec) < 0)
                {
                    curr_time.tv_nsec += 1e9;
                    curr_time.tv_sec--;
                }

                delta_time.tv_sec  = curr_time.tv_sec - pht->m_prev_ts.tv_sec;
                delta_time.tv_nsec = curr_time.tv_nsec - pht->m_prev_ts.tv_nsec;

                fprintf(stderr,
                        "Current Time                 : %ld.%09ld    Delta[%2d] : %ld.%09ld\n",
                        curr_time_pristine.tv_sec,  curr_time_pristine.tv_nsec,
                        pht->m_delta_count, delta_time.tv_sec, delta_time.tv_nsec);
                fprintf(pht->m_host_file,
                        "Current Time                 : %ld.%09ld    Delta[%2d] : %ld.%09ld\n",
                        curr_time_pristine.tv_sec,  curr_time_pristine.tv_nsec,
                        pht->m_delta_count, delta_time.tv_sec, delta_time.tv_nsec);

                // Save for Next Delta Time
                pht->m_prev_ts.tv_sec = curr_time_pristine.tv_sec;
                pht->m_prev_ts.tv_nsec = curr_time_pristine.tv_nsec;
                pht->m_delta_count++;
            }
            break;

        default:
            printf ("%s: Bad Port Value = %d\n", __func__, port_value);
            exit (1);
            break;
    }

    return 0;
}
