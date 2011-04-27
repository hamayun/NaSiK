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

#include <stdio.h>
#include <Private/Driver.h>
#include <DnaTools/DnaTools.h>

status_t extfifo_read(void *handler, void * destination,
    int64_t offset, int32_t * p_count)
{
  extfifo_config_t * extfifo =  handler; 	 
  uint32_t * dst_ptr = (uint32_t *)destination;
  extfifo_header_t fifo;
  uint32_t nbytes = *p_count;

  //interrupt_status_t it_status = 0;

/*--- There is a wild calculus down there. nbytes need to be a multiple of uint32 ! ---*/
  uint32_t nr = nbytes, avail = 0, end, rem;

  extfifo_header_t * fifo_addr = extfifo -> header_address;

  watch (status_t)
  {
    ensure (fifo_addr != NULL, DNA_ERROR);

    while(nr) {
      //printf("Extfifo Read is working!\n");
      //it_status = cpu_trap_mask_and_backup();
      //lock_acquire (extfifo -> lock);

      //cpu_vector_read(UINT32, (uint32_t *)(& fifo), (uint32_t *)fifo_addr, sizeof(extfifo_header_t)>>2);
      dna_memcpy((void *)(& fifo), (void *)fifo_addr, sizeof(extfifo_header_t));

      rem = fifo . status;

      if ((avail = (nr > rem) ? rem : nr) != 0) {
        if (avail <= fifo . depth - fifo . tail) {
          if (fifo . datatype == EXTFIFO_INT) {
            //cpu_vector_read(UINT32, (uint32_t *)dst_ptr, (uint32_t *)(fifo . buffer + fifo . tail), avail);
            dna_memcpy((void *)dst_ptr, (void *)(fifo . buffer + fifo . tail), avail);
          }
          else { // Here, the data is in the float type
            //cpu_vector_read(SFLOAT, (float *)dst_ptr, (float *)(fifo . buffer + fifo . tail), avail);
            dna_memcpy((void *)dst_ptr, (void *)(fifo . buffer + fifo . tail), avail);
          }
        }
        else {
          end = fifo . depth - fifo . tail;
          if (fifo . datatype == EXTFIFO_INT) {
            //cpu_vector_read(UINT32, (uint32_t *)dst_ptr, (uint32_t *)(fifo . buffer + fifo . tail), end);
            dna_memcpy((void *)dst_ptr, (void *)(fifo . buffer + fifo . tail), end);
            //cpu_vector_read(UINT32, (uint32_t *)(dst_ptr + end), (uint32_t *)(fifo . buffer), (avail - end));
            dna_memcpy((void *)(dst_ptr + end), (void *)(fifo . buffer), (avail - end));
          }
          else {
            //cpu_vector_read(SFLOAT, (float *)dst_ptr, (float *)(fifo . buffer + fifo . tail), end);
            dna_memcpy((void *)dst_ptr, (void *)(fifo . buffer + fifo . tail), end);
            //cpu_vector_read(SFLOAT, (float *)(dst_ptr + end), (float *)(fifo . buffer), (avail - end));
            dna_memcpy((void *)(dst_ptr + end), (void *)(fifo . buffer), (avail - end));
          }
        }

        fifo . tail = (fifo . tail + avail) % fifo . depth; 
        fifo . status -= avail; 
        dst_ptr += avail;
        nr -= avail;

        //cpu_vector_write(UINT32, (uint32_t *)fifo_addr, (uint32_t *)(& fifo), sizeof(extfifo_header_t)>>2);
        dna_memcpy((void *)fifo_addr, (void *)(& fifo), sizeof(extfifo_header_t));
        //lock_release (extfifo -> lock);
        //cpu_trap_restore (it_status);

        if (fifo . status == 0 && nr != 0) thread_yield(); //??????
      }
      else {
        //lock_release (extfifo -> lock);
        //cpu_trap_restore (it_status);
        thread_yield(); //??????
      }
    }
  }
  // Here: exceptions.
  return DNA_OK;
}



