
/*************************************************************************************
 * File   : hosttime.cpp,
 *        : Adapted from Rabbits hosttime.
 * Copyright (C) 2011 TIMA Laboratory
 * Author(s) :      Mian-Muhammad, HAMAYUN mian-muhammad.hamayun@imag.fr;
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

#define DEBUG_OPTION "hosttime"

#include "hosttime.h"
#include "debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/times.h>

using namespace native;

double hosttime::get_clock(void)
{
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

hosttime::hosttime (sc_module_name _name, const char *filename) : device_slave (_name)
{
    _generic_name = "HOSTTIME";
    DOUT_CTOR << this->name() << std::endl;
    
    host_file = fopen (filename, "wb");
    if(host_file == NULL)
    {
        std::cerr << "Error in" <<  name () << " : file " << filename << " can not be opened!" << std::endl; 
        exit (1);
    }
}

hosttime::~hosttime()
{
    DOUT_DTOR << this->name() << std::endl;
    
    if (host_file)
    {
        fclose (host_file);
        host_file = NULL; 
    }
}

void hosttime::end_of_elaboration()
{
    device_slave::end_of_elaboration();

    _segments.push_back(mapping::init(name(), HOSTTIME_SPAN * sizeof(uint32_t)));
    _registers.ptr = _segments[0]->base_addr;

    symbol<uint32_t>       *symbol_value;
    symbol_value = new symbol<uint32_t>("PLATFORM_HOSTTIME_BASE");
    symbol_value->push_back(_registers.ptr);
    _symbols.push_back(symbol_value);
}

void hosttime::slv_write (uint32_t *addr, uint32_t data)
{
  switch(REGISTER_INDEX(UINT32, addr) & 0x01)
  {
    case HOSTTIME_WRITE:
    {
        // Discard what processor writes, only remember the time when we write.
        fprintf(host_file, "[%d] Time is %lf\n", data, get_clock());
        DOUT << name() << ": [" << data << "] Time is: " << get_clock() << std::endl;
        fflush(host_file);
        break;
    }
    case HOSTTIME_READ:
    {
        ASSERT_MSG( false, "HOSTTIME_READ readonly register");
        break;
    }
    default :
      ASSERT_MSG( false, "Invalide address");
  }
}

void hosttime::slv_read (uint32_t *addr, uint32_t *data)
{
    *((unsigned long *)data + 0) = 0x12345678;

    switch(REGISTER_INDEX(UINT32, addr) & 0x01)
    {
    }
    
    std::cerr << __FUNCTION__ << ": Read Request for @0x" << std::hex << addr << std::endl; 
    exit (1);
}
