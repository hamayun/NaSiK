/*
 * Copyright (C) 2007 TIMA Laboratory
 *
 * Author(s):
 *   Patrice GERIN, patrice.gerin@imag.fr
 *   Xavier GUERIN, xavier.guerin@imag.fr
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
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdbool.h>
#include <malloc.h>

#include <Private/Dispatcher.h>

/*
 * The dispatcher function
 */

int32_t dispatcher (kpn_channel_t c[NB_DECODER + 1])
{
	uint8_t marker0, marker1;
  bool soi_started = false;
  int32_t next_decoder = 0;

  /*
   * Parse the stream
   */

  while (true)
  {
    kpn_channel_read (c[0], & marker0, 1);

    if (marker0 == M_SMS)
    {
      kpn_channel_read (c[0], & marker1, 1);

      switch (marker1)
      {
        case M_SOI :
          IPRINTF ("Found SOI\r\n");

          kpn_channel_write (c[next_decoder + 1], & marker0, 1);
          kpn_channel_write (c[next_decoder + 1], & marker1, 1);

          soi_started = true;
          break;

        case M_EOI :
          IPRINTF ("Found EOI\r\n");

          kpn_channel_write (c[next_decoder + 1], & marker0, 1);
          kpn_channel_write (c[next_decoder + 1], & marker1, 1);

          IPRINTF ("Flushing data to the decoder\r\n");
          kpn_channel_purge (c[next_decoder + 1], true);

          soi_started = false;
          next_decoder = (next_decoder + 1) % NB_DECODER;
          break;

        default:
          kpn_channel_write (c[next_decoder + 1], & marker0, 1);
          kpn_channel_write (c[next_decoder + 1], & marker1, 1);
          break;
      }
    }
    else if (soi_started)
    {
      kpn_channel_write (c[next_decoder + 1], & marker0, 1);
    }
  }
}

