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

#include <slave_device.h>

class hosttime: public slave_device
{
public:
    hosttime (const char *_name, const char *filename);
    virtual ~hosttime ();

public:
    /*
     *   Obtained from father
     *   void send_rsp (bool bErr);
     */
    virtual void rcv_rqst (unsigned int ofs, unsigned char be,
                           unsigned char *data, bool bWrite);

private:
    void write (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr);
    void read  (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr);

private:
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
};

#endif

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
