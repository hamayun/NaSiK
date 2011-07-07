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
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>

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

int init_hosttime(hosttime_t* phosttime, const char *filename)
{
    phosttime->host_file = fopen (filename, "wb");
    if(phosttime->host_file == NULL)
    {
        fprintf(stderr, "Error in %s: file %s can not be opened!\n", __func__, filename);
        return (1);
    }

    phosttime->m_io_total = 0.0;
    phosttime->m_comp_total = 0.0;

    phosttime->m_comp_start_count = 0;
    phosttime->m_comp_end_count = 0;
    phosttime->m_io_start_count = 0;
    phosttime->m_io_end_count = 0;

    return 0;
}

void close_hosttime(hosttime_t* phosttime)
{
    if (phosttime->host_file)
    {
        fclose (phosttime->host_file);
        phosttime->host_file = NULL;
    }
}

int hosttime_handler(void *opaque, int size, int is_write, uint64_t addr, uint64_t *value)
{
    hosttime_t* phosttime = (hosttime_t *) opaque;
    hosttime_port_t port = (hosttime_port_t) *value;

    if((!phosttime) || phosttime->host_file == NULL)
    {
        fprintf(stderr, "Invalide Hosttime File Handle\n");
        return (1);
    }

    switch (port)
    {
        case HOSTTIME_CURRENT_TIME:
            fprintf(phosttime->host_file, "Current Time is : %lf\n", get_clock());
            fprintf(stderr,    "Current Time is : %lf\n", get_clock());
            fflush(phosttime->host_file);
            break;

        case HOSTTIME_COMP_START:
            phosttime->m_comp_start_count++;
            phosttime->m_comp_start = get_clock();
            //fprintf(stderr, "Computation Start: %lf\n", m_comp_start);
            break;

        case HOSTTIME_COMP_END:
            phosttime->m_comp_end_count++;
            phosttime->m_comp_end = get_clock();
            //fprintf(stderr, "Computation End  : %lf\n", m_comp_end);
            if(phosttime->m_comp_end >= phosttime->m_comp_start){
                phosttime->m_comp_total += (phosttime->m_comp_end - phosttime->m_comp_start);
            }
            /*
            else{
                printf ("Invalide Computation Start (%f) and End Times (%f) Delta (%f)\n", m_comp_start, m_comp_end, (m_comp_end - m_comp_start));
                exit(1);
            }*/
            break;

        case HOSTTIME_IO_START:
            phosttime->m_io_start_count++;
            phosttime->m_io_start = get_clock();
            //fprintf(stderr, "I/O Start        : %lf\n", m_io_start);
            break;

        case HOSTTIME_IO_END:
            phosttime->m_io_end_count++;
            phosttime->m_io_end = get_clock();
            //fprintf(stderr, "I/O End          : %lf\n", m_io_end);
            if(phosttime->m_io_end >= phosttime->m_io_start){
                phosttime->m_io_total += (phosttime->m_io_end - phosttime->m_io_start);
            }
            /*
            else{
                printf ("Invalide I/O Start (%lf) and End Times (%lf)\n", m_io_start, m_io_end);
                exit(1);
            }*/
            break;

        case HOSTTIME_FLUSH_DATA:
            {
                double io_profile_overhead = 0.0, net_io_cost = 0.0;
                double comp_profile_overhead = 0.0, net_comp_cost = 0.0;

                if((phosttime->m_comp_start_count != phosttime->m_comp_end_count) ||
                   (phosttime->m_io_start_count != phosttime->m_io_end_count))
                {
                    fprintf(phosttime->host_file, "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
                    fprintf(stderr,    "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
                }

                io_profile_overhead = ONE_PROFILE_CALL_COST * (phosttime->m_io_start_count + phosttime->m_io_end_count);
                net_io_cost = phosttime->m_io_total - io_profile_overhead;

                comp_profile_overhead = ONE_PROFILE_CALL_COST * (phosttime->m_comp_start_count + phosttime->m_comp_end_count);
                net_comp_cost = phosttime->m_comp_total - comp_profile_overhead;

                fprintf(phosttime->host_file,
                        "Total I/O Time              : %lf    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        phosttime->m_io_total, phosttime->m_io_start_count, phosttime->m_io_end_count,
                        io_profile_overhead, net_io_cost);
                fprintf(stderr,
                        "Total I/O Time              : %lf    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        phosttime->m_io_total, phosttime->m_io_start_count, phosttime->m_io_end_count,
                        io_profile_overhead, net_io_cost);

                fprintf(phosttime->host_file,
                        "Total Computation Time      : %lf    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        phosttime->m_comp_total, phosttime->m_comp_start_count, phosttime->m_comp_end_count,
                        comp_profile_overhead, net_comp_cost);
                fprintf(stderr,
                        "Total Computation Time      : %lf    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        phosttime->m_comp_total, phosttime->m_comp_start_count, phosttime->m_comp_end_count,
                        comp_profile_overhead, net_comp_cost);

                fprintf(phosttime->host_file,
                        "Total I/O + Computation Time: %lf    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        phosttime->m_io_total + phosttime->m_comp_total,
                        phosttime->m_io_start_count + phosttime->m_comp_start_count,
                        phosttime->m_io_end_count + phosttime->m_comp_end_count,
                        io_profile_overhead + comp_profile_overhead,
                        net_io_cost + net_comp_cost);
                fprintf(stderr,    "Total I/O + Computation Time: %lf    [%06d/%06d] Profile Overhead: %4.6f, Net Cost: %4.6f\n",
                        phosttime->m_io_total + phosttime->m_comp_total,
                        phosttime->m_io_start_count + phosttime->m_comp_start_count,
                        phosttime->m_io_end_count + phosttime->m_comp_end_count,
                        io_profile_overhead + comp_profile_overhead,
                        net_io_cost + net_comp_cost);
                fflush(phosttime->host_file);
                close_hosttime(phosttime);
            }
            break;

        default:
            printf ("%s: Bad Port Address = %d\n", __func__, port);
            exit (1);
            break;
    }

    return 0;
}
