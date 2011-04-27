/************************************************************************
 * Copyright (C) 2008 TIMA Laboratory                                    *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *************************************************************************/

#include <Private/Driver.h>
#include <MemoryManager/MemoryManager.h>
#include <DnaTools/DnaTools.h>

status_t extfifo_init_driver (void) {
  uint32_t  header_size = 0;
  extfifo_header_t *header;
  char * base_path = "disk/extfifo/", * buffer, ascii[64];

  interrupt_status_t it_status = 0;
  int32_t i = 0;

  watch (status_t)
  {
    header_size = sizeof (extfifo_header_t) / sizeof (uint32_t);
    extfifo_devices = kernel_malloc (sizeof (const char *) * (EXTFIFO_CHANNEL_NDEV + 1), false);
    ensure (extfifo_devices != NULL, DNA_OUT_OF_MEM);
  
    for (i = 0; i < EXTFIFO_CHANNEL_NDEV; i++) {
      buffer = kernel_malloc (DNA_FILENAME_LENGTH, false);
      ensure (buffer != NULL, DNA_OUT_OF_MEM);
  		
      dna_itoa (i, ascii);
      dna_strcpy (buffer, base_path);
      dna_strcat (buffer, ascii);
      extfifo_devices[i] = buffer;
  
  /************************************************************
   *      Initialize header
   * ********************************************************/
  
      it_status = cpu_trap_mask_and_backup();
      //lock_acquire (EXTFIFO_CHANNELS[i].lock);
  
      header = (extfifo_header_t *) EXTFIFO_CHANNELS[i] . header_address;
  
      if (header -> initialized != 0xDEADBEEF) {
        header -> initialized = 0xDEADBEEF;
        header -> head = 0;
        header -> tail = 0;
        header -> status = 0;
        header -> depth =     EXTFIFO_CHANNELS[i] . depth;
        header -> cell_size = EXTFIFO_CHANNELS[i] . cell_size;
        header -> datatype =  EXTFIFO_CHANNELS[i] . dtype;
      }
  
      //lock_release (EXTFIFO_CHANNELS[i] . lock);
      cpu_trap_restore(it_status);
    }
  
    extfifo_devices[i] = NULL;
    return DNA_OK;
  }

#if 0
  rescue (no_memory)
  {
    leave;
  }
#endif
}





