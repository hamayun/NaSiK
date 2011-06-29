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
#include <hosttime.h>
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

hosttime::hosttime (const char *_name, const char *filename) : slave_device (_name)
{
    host_file = fopen (filename, "wb");
    if(host_file == NULL)
    {
        fprintf(stderr, "Error in %s: file %s can not be opened!\n", name (), filename);
        exit (1);
    }

    m_io_total = 0.0;
    m_comp_total = 0.0;

    m_comp_start_count = 0;
    m_comp_end_count = 0;
    m_io_start_count = 0;
    m_io_end_count = 0;
}

hosttime::~hosttime()
{
    if (host_file)
    {
        fclose (host_file);
        host_file = NULL;
    }
}

void hosttime::write (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    switch (ofs)
    {
        case 0x00:
            // Discard what processor writes, only remember the time when we write.
            fprintf(host_file, "Current Time is : %lf\n", get_clock());
            fprintf(stderr,    "Current Time is : %lf\n", get_clock());
            fflush(host_file);
            break;

        case 0x08:
            m_comp_start_count++;
            m_comp_start = get_clock();
            //fprintf(stderr, "Computation Start: %lf\n", m_comp_start);
            break;

        case 0x10:
            m_comp_end_count++;
            m_comp_end = get_clock();
            //fprintf(stderr, "Computation End  : %lf\n", m_comp_end);
            if(m_comp_end >= m_comp_start){
                m_comp_total += (m_comp_end - m_comp_start);
            }
            /*
            else{
                printf ("Invalide Computation Start (%lf) and End Times (%lf)\n", m_comp_start, m_comp_end);
                exit(1);
            }*/
            break;

        case 0x18:
            m_io_start_count++;
            m_io_start = get_clock();
            //fprintf(stderr, "I/O Start        : %lf\n", m_io_start);
            break;

        case 0x20:
            m_io_end_count++;
            m_io_end = get_clock();
            //fprintf(stderr, "I/O End          : %lf\n", m_io_end);
            if(m_io_end >= m_io_start){
                m_io_total += (m_io_end - m_io_start);
            }
            /*
            else{
                printf ("Invalide I/O Start (%lf) and End Times (%lf)\n", m_io_start, m_io_end);
                exit(1);
            }*/
            break;

        case 0x28:
            if((m_comp_start_count != m_comp_end_count) || (m_io_start_count != m_io_end_count))
            {
                fprintf(host_file, "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
                fprintf(stderr,    "\n***WARNING: Profile Results Possibly Incorrect; Mismatch in Start/End Counters\n\n");
            }

            fprintf(host_file, "Total I/O Time              : %lf    [%06d/%06d]\n", m_io_total, m_io_start_count, m_io_end_count);
            fprintf(stderr,    "Total I/O Time              : %lf    [%06d/%06d]\n", m_io_total, m_io_start_count, m_io_end_count);

            fprintf(host_file, "Total Computation Time      : %lf    [%06d/%06d]\n", m_comp_total, m_comp_start_count, m_comp_end_count);
            fprintf(stderr,    "Total Computation Time      : %lf    [%06d/%06d]\n", m_comp_total, m_comp_start_count, m_comp_end_count);

            fprintf(host_file, "Total I/O + Computation Time: %lf    [%06d/%06d]\n",
                    m_comp_total + m_io_total, m_io_start_count + m_comp_start_count, m_io_end_count + m_comp_end_count);
            fprintf(stderr,    "Total I/O + Computation Time: %lf    [%06d/%06d]\n",
                    m_comp_total + m_io_total, m_io_start_count + m_comp_start_count, m_io_end_count + m_comp_end_count);
            fflush(host_file);
            break;

        default:
            printf ("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n", name (),
                    __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                    (unsigned int) *((unsigned int *)data + 0), (unsigned int) *((unsigned int*)data + 1));
            exit (1);
            break;
    }
    bErr = false;
}

void hosttime::read (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    *((unsigned int *)data + 0) = 0x1234;
    *((unsigned int *)data + 1) = 0x5678;

    printf ("Bad %s:%s ofs=0x%X, be=0x%X!\n",  name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
    exit (1);
    bErr = false;
}

void hosttime::rcv_rqst (unsigned int ofs, unsigned char be,
                          unsigned char *data, bool bWrite)
{

    bool bErr = false;

    if(bWrite){
        this->write(ofs, be, data, bErr);
    }else{
        this->read(ofs, be, data, bErr);
    }

    send_rsp(bErr);

    return;
}

/*
 * Vim standard variables
 * vim:set ts=4 expandtab tw=80 cindent syntax=c:
 *
 * Emacs standard variables
 * Local Variables:
 * mode: c
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
