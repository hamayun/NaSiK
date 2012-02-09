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
#include <stdint.h>
#include <sl_tg_device.h>

sl_tg_device::sl_tg_device (const char *_name, const char *filename) : slave_device (_name)
{
    tg_file = fopen (filename, "rb");
    if(tg_file == NULL)
    {
        fprintf(stderr, "Error in %s: file %s can not be opened!\n", name (), filename);
        exit (1);
    }

    reset_input ();
}

sl_tg_device::~sl_tg_device ()
{
    if (tg_file)
        fclose (tg_file);
}

void sl_tg_device::reset_input ()
{
    fseek (tg_file, 0, SEEK_SET);
}

void sl_tg_device::write (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    switch (ofs)
    {
        case 0x480:
            break;
        default:
            printf ("Bad %s:%s ofs=0x%X, be=0x%X, data=0x%X-%X!\n", name (),
                    __FUNCTION__, (unsigned int) ofs, (unsigned int) be,
                                   (unsigned int) *((unsigned int *)data + 0), (unsigned int) *((unsigned int *)data + 1));
            //exit (1);
            break;
    }
    bErr = false;
}

void sl_tg_device::read (unsigned long ofs, unsigned char be, unsigned char *data, bool &bErr)
{
    int             i;
	int				offset_dd = 0;
	int				mod = 0;
	int             rval = 0;

    *((unsigned int *)data + 0) = 0;
    *((unsigned int *)data + 1) = 0;

	switch (be)
	{
		//byte access uint8_t
		case 0x01: offset_dd = 0; mod = 1; break;
        case 0x02: offset_dd = 1; mod = 1; break;
        case 0x04: offset_dd = 2; mod = 1; break;
        case 0x08: offset_dd = 3; mod = 1; break;
        case 0x10: offset_dd = 4; mod = 1; break;
        case 0x20: offset_dd = 5; mod = 1; break;
        case 0x40: offset_dd = 6; mod = 1; break;
        case 0x80: offset_dd = 7; mod = 1; break;
        //word access uint16_t
        case 0x03: offset_dd = 0; mod = 2; break;
        case 0x0C: offset_dd = 1; mod = 2; break;
        case 0x30: offset_dd = 2; mod = 2; break;
        case 0xC0: offset_dd = 3; mod = 2; break;
        //dword access uint32_t
        case 0x0F: offset_dd = 0; mod = 4; break;
        case 0xF0: offset_dd = 1; mod = 4; break;
        //qword access uint64_t
        case 0xFF: offset_dd = 0; mod = 8; break;

	}

    switch (ofs)
    {
        case 0x00:
        case 0x10:
            for (i = 0; i < mod; i++)
            {
                if(!fread (data + i + offset_dd, sizeof (uint8_t), 1, tg_file))
				{
                	reset_input ();
					rval = fread (data + i + offset_dd, sizeof (uint8_t), 1, tg_file);
				}
				//*(data + 4 + i) = *(data + i);
                //tg_bytes_left --;
            }
            break;
        case 0x80:
            if (!tg_bytes_left)
                reset_input ();
            *((unsigned int *)data + 0) = (tg_bytes_left + 3) / 4;
            break;
        default:
            printf ("Bad %s:%s ofs=0x%X, be=0x%X!\n",  name (), __FUNCTION__, (unsigned int) ofs, (unsigned int) be);
            exit (1);
    }
    bErr = false;
}

void sl_tg_device::rcv_rqst (unsigned long ofs, unsigned char be,
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
