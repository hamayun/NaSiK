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

#include <Private/Serializer.h>
#include <Processor/Processor.h>

/*
 * The serializer function
 */

int32_t serializer (kpn_channel_t c[NB_DECODER + 1])
{
  uint8_t * buffer = 0;
  int32_t next_decoder = 0;

#ifdef SHOW_TIME
  bigtime_t old, new;
#endif

  /*
   * Allocate a dummy sized buffer
   */

  buffer = malloc (256 * 144 * 2);

  /*
   * Parse the flows
   */

#ifdef SHOW_TIME
  cpu_timer_get (0, & old);
#endif

  while (true)
  {
    kpn_channel_read (c[next_decoder + 1], buffer, 256 * 144 * 2);
    kpn_channel_write (c[0], buffer, 256 * 144 * 2);
    next_decoder = (next_decoder + 1) % NB_DECODER;

#ifdef SHOW_TIME
    cpu_timer_get (0, & new);
    printf ("1 frame in %ld ns\r\n", new - old);
    old = new;
#endif
  }
}

