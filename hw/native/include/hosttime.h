
/*************************************************************************************
 * File   : hosttime.h,
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

#ifndef _HOSTTIME_H_
#define _HOSTTIME_H_

#include "systemc.h"
#include "devices.h"

namespace native
{
    class hosttime: public device_slave
    {
    public:
        hosttime (sc_module_name _name, const char *filename);
        virtual ~hosttime ();

        void end_of_elaboration();

        void slv_read (uint32_t *addr, uint32_t *data);
        void slv_write (uint32_t *addr, uint32_t data);

        typedef enum {
            HOSTTIME_WRITE = 0,
            HOSTTIME_READ = 1,
            HOSTTIME_SPAN = 2,
        } REGISTERS;

    private:
        double get_clock(void);

        FILE                *host_file;
    };
}
#endif

