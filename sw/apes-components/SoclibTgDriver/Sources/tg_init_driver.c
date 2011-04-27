/*
 * Copyright (C) 2007 TIMA Laboratory
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Private/Driver.h>
#include <MemoryManager/MemoryManager.h>
#include <DnaTools/DnaTools.h>

status_t tg_init_driver (void)
{
  char * base_path = "disk/tg/", ascii[64];

  watch (status_t)
  {
    tg_devices = kernel_malloc (5 * sizeof (char *), true);
    ensure (tg_devices != NULL, DNA_OUT_OF_MEM);

    /*
     * Create the associated path names.
     */
  
    for (int32_t i = 0; i < 5; i += 1)
    {
      tg_devices[i] = kernel_malloc (DNA_PATH_LENGTH, false);
      check (no_memory, tg_devices[i] != NULL, DNA_OUT_OF_MEM);

      dna_strcpy (tg_devices[i], (const char *)base_path);
      dna_itoa (i, ascii);
      dna_strcat (tg_devices[i], ascii);
    }

    return DNA_OK;
  }

  rescue (no_memory)
  {
    kernel_free (tg_devices);
    leave;
  }
}

