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
#include <mem_device.h>

//#define DEBUG_MEM

#ifdef DEBUG_MEM
#define DPRINTF(fmt, args...)                   \
    do { printf(fmt , ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

mem_device::mem_device (const char *_name, unsigned int _size) : slave_device (_name)
{
    m_write_invalidate = true;
    mem = NULL;
    size = _size;
    mem = new unsigned char [size];
    memset (mem, 0, size);

    printf("mem_device: Memory area location: 0x%08x\n", (int)mem);

}

mem_device::mem_device (const char *_name, unsigned int _size, unsigned char* _mem) : slave_device (_name)
{
    m_write_invalidate = false;
    size = _size;
	mem = _mem;
}

mem_device::~mem_device ()
{
    if (mem)
        delete [] mem;
}

void mem_device::write (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int                 offset_dd = 0;
    int                 mod = 0;
    int                 err = 0;

    bErr = false;
    wait (1, SC_NS);


    if (ofs > size || be == 0)
        err = 1;

    if (!err)
    {
        switch (be)
        {
            //byte access
        case 0x01: offset_dd = 0; mod = 1; break;
        case 0x02: offset_dd = 1; mod = 1; break;
        case 0x04: offset_dd = 2; mod = 1; break;
        case 0x08: offset_dd = 3; mod = 1; break;
        case 0x10: offset_dd = 4; mod = 1; break;
        case 0x20: offset_dd = 5; mod = 1; break;
        case 0x40: offset_dd = 6; mod = 1; break;
        case 0x80: offset_dd = 7; mod = 1; break;
            //word access
        case 0x03: offset_dd = 0; mod = 2; break;
        case 0x0C: offset_dd = 1; mod = 2; break;
        case 0x30: offset_dd = 2; mod = 2; break;
        case 0xC0: offset_dd = 3; mod = 2; break;
            //dword access
        case 0x0F: offset_dd = 0; mod = 4; break;
        case 0xF0: offset_dd = 1; mod = 4; break;
            //qword access
        case 0xFF: offset_dd = 0; mod = 8; break;

        default:
        {
            unsigned int        tbe = be;
            while ((tbe & 1) == 0)
            {
                tbe >>= 1;
                offset_dd++;
            }
            mod = offset_dd;
            while (tbe & 1)
            {
                *((unsigned char*) (mem + ofs) + offset_dd) =
                    *((unsigned char*) data + offset_dd);
                tbe >>= 1;
                offset_dd++;
            }
            mod = offset_dd - mod;
            if ((mod != 1 && mod != 2 && mod != 4 && mod != 8) || tbe)
                err = 1;
            else
                err = 2;
        }
        }

        if (!err)
            switch (mod)
            {
            case 1:
                *((uint8_t *) (mem + ofs) + offset_dd) =
                    *((uint8_t *) data + offset_dd);
                break;
            case 2:
                *((uint16_t *) (mem + ofs) + offset_dd) =
                    *((uint16_t *) data + offset_dd);
                break;
            case 4:
                *((uint32_t *) (mem + ofs) + offset_dd) =
                    *((uint32_t *) data + offset_dd);
                break;
            case 8:
                *((uint64_t *) (mem + ofs)) = *((uint64_t *) data);
                break;
            default:
                err = 1;
            }
		//std::cerr << "WRITE: shared_memory " << "addr: 0x" << std::hex << ofs << " be: 0x" << std::hex << (int) be << " data: 0x" << std::hex << (int64_t)*(data+offset_dd) << " after: 0x" << std::hex << (int64_t)*((uint64_t *)(mem + ofs)) << std::endl;	
    }

    if (err == 1)
    {
        printf ("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n",
                name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                (unsigned int) *((unsigned int *)data + 0), (unsigned int) *((unsigned int *)data + 1));
        //exit (1);
    }
}

void mem_device::read (unsigned int ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    uint32_t be_off = 0;

    bErr = false;
    //printf ("mem_device::read %s, ofs = %lu\n", name (), ofs);

#if 1

    ((uint64_t *) data)[0] = (uint64_t)*((uint64_t *)(mem + ofs));
	//std::cerr << "READ: shared_memory " << "addr: 0x" << std::hex << ofs << " be: 0x" << std::hex << (int) be << " data: 0x" << std::hex << ((uint64_t *)data)[0] << " origin: 0x" << std::hex << (uint64_t)*((uint64_t *)(mem + ofs)) << std::endl;
#else
    if(be == 0xF0){
        be_off = 4;
    }

    *((unsigned int *) (data + be_off)) = (unsigned int ) (mem + ofs + be_off);

    DPRINTF("read rsp: 0x%08x\n", *((uint32_t *)data));
#endif

    wait (26, SC_NS);
}

void mem_device::rcv_rqst (unsigned int ofs, unsigned char be,
                           unsigned char *data, bool bWrite)
{
    bool bErr = false;

    if (bWrite)
        this->write (ofs, be, data, bErr);
    else
        this->read (ofs, be, data, bErr);

    send_rsp (bErr);
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
