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
}

hosttime::~hosttime()
{
    if (host_file)
        fclose (host_file);
}

void hosttime::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
#if 0
    switch (ofs)
    {
    case 0x00:
#endif
		// Discard what processor writes, only remember the time when we write.
		fprintf(host_file, "time is %lf\n", get_clock());
		fprintf(stderr, "time is %lf\n", get_clock());
		fflush(host_file);
#if 0
        break;
    default:
        printf ("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n", name (),
                __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned long*)data + 0), (unsigned int) *((unsigned long*)data + 1));
        exit (1);
        break;
    }
#endif
    bErr = false;
}

void hosttime::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;

    *((unsigned long *)data + 0) = 0x1234;
    *((unsigned long *)data + 1) = 0x5678;

    printf ("Bad %s:%s ofs=0x%X, be=0x%X!\n",  name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
    exit (1);
    bErr = false;
}

void hosttime::rcv_rqst (unsigned long ofs, unsigned char be,
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
