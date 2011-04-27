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
#include <DnaTools/DnaTools.h>

/******************
 * Module defs
 ******************/

/** extfifo_driver_write 
 *
 *  Description: According the number of data to send, initiate a rendezvous or send an eager pkt
 *  
 */

status_t extfifo_write (void * handler, void * source,
    int64_t offset, int32_t * p_count)
{
  extfifo_config_t * extfifo = handler;
  uint32_t nbytes = *p_count;
  extfifo_header_t fifo;

  //interrupt_status_t it_status = 0;

  /*--- There is a wild calculus down there. nbytes need to be a multiple of uint32 ! ---*/
  uint32_t nw = nbytes, avail = 0, end, rem, header_size;
  uint32_t * src_ptr = (uint32_t *)source;

  extfifo_header_t  * fifo_addr = extfifo -> header_address;

  header_size = sizeof (extfifo_header_t);

  while(nw) {
    //it_status = cpu_trap_mask_and_backup ();
    //lock_acquire (extfifo ->lock);

    //cpu_vector_read(UINT32, (uint32_t *)(& fifo), (uint32_t *)fifo_addr, header_size);
    dna_memcpy((void *)(& fifo), (void *)fifo_addr, header_size);

    rem = fifo . depth -  fifo . status;

    if ((avail = (nw > rem) ? rem : nw) != 0) {
      if (avail <= fifo . depth - fifo . head) {
        if (fifo . datatype == EXTFIFO_INT) {
          //cpu_vector_write(UINT32, (uint32_t *)(fifo . buffer + fifo . head), (uint32_t *)src_ptr, avail);
          dna_memcpy((void *)(fifo . buffer + fifo . head), (void *)src_ptr, avail);
        }
        else {
          //cpu_vector_write(SFLOAT, (float *)(fifo . buffer + fifo . head), (float *)src_ptr, avail);
          dna_memcpy((void *)(fifo . buffer + fifo . head), (void *)src_ptr, avail);
        }
      }
      else {
        end = fifo . depth - fifo . head;
        if (fifo . datatype == EXTFIFO_INT) {
          //cpu_vector_write(UINT32, (uint32_t *)(fifo . buffer + fifo . head) , (uint32_t *)src_ptr, end);
          dna_memcpy((void *)(fifo . buffer + fifo . head), (void *)src_ptr, end);
          //cpu_vector_write(UINT32, (uint32_t *)(fifo . buffer) , (uint32_t *)(src_ptr + end), (avail - end));
          dna_memcpy((void *)(fifo . buffer), (void *)(src_ptr + end), (avail - end));
        }
        else {
          //cpu_vector_write(SFLOAT, (float *)(fifo . buffer + fifo . head) , (float *)src_ptr, end);
          dna_memcpy((void *)(fifo . buffer + fifo . head), (void *)src_ptr, end);
          //cpu_vector_write(SFLOAT, (float *)(fifo . buffer), (float *)(src_ptr + end), avail - end);
          dna_memcpy((void *)(fifo . buffer), (void *)(src_ptr + end), (avail - end));
        }
      }

      fifo . head = (fifo . head + avail) % fifo . depth; 
      fifo . status +=  avail;
      src_ptr += avail;
      nw -= avail;

      //cpu_vector_write(UINT32, (uint32_t *)fifo_addr, (uint32_t *)(&fifo), header_size);
      dna_memcpy((void *)fifo_addr, (void *)(&fifo), header_size);

      //lock_release (extfifo -> lock);
      //cpu_trap_restore(it_status);
      if (fifo . depth - fifo . status == 0 && nw != 0) thread_yield();
    }
    else {
      //lock_release (extfifo -> lock);
      //cpu_trap_restore(it_status);
      thread_yield();
    }
  }
  return DNA_OK;
}
