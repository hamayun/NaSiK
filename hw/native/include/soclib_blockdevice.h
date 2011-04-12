/*************************************************************************************
 * File   : soclib_blockdevice.h,
 *        : Adapted from Rabbits sl_block_device.
 * Copyright (C) 2011 TIMA Laboratory
 * Author(s) :      Nicolas, FOURNEL nicolas.fournel@imag.fr;
 *                  Mian-Muhammad, HAMAYUN mian-muhammad.hamayun@imag.fr; 
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

#ifndef __SOCLIB_BLOCKDEVICE_H__
#define __SOCLIB_BLOCKDEVICE_H__

#include "systemc.h"
#include "devices.h"

namespace native {

    enum sl_block_registers {
        BLOCK_DEVICE_BUFFER     = 0,
        BLOCK_DEVICE_LBA        = 1,
        BLOCK_DEVICE_COUNT      = 2,
        BLOCK_DEVICE_OP         = 3,
        BLOCK_DEVICE_STATUS     = 4,
        BLOCK_DEVICE_IRQ_ENABLE = 5,
        BLOCK_DEVICE_SIZE       = 6,
        BLOCK_DEVICE_BLOCK_SIZE = 7,
        BLOCK_DEVICE_FINISHED_BLOCK_COUNT = 8,
        BLOCKDEVICE_SPAN        = 9,
    };

    enum sl_block_op {
        BLOCK_DEVICE_NOOP,
        BLOCK_DEVICE_READ,
        BLOCK_DEVICE_WRITE,
        BLOCK_DEVICE_FILE_NAME,
    };

    enum sl_block_status {
        BLOCK_DEVICE_IDLE          = 0,
        BLOCK_DEVICE_BUSY          = 1,
        BLOCK_DEVICE_READ_SUCCESS  = 2,
        BLOCK_DEVICE_WRITE_SUCCESS = 3,
        BLOCK_DEVICE_READ_ERROR    = 4,
        BLOCK_DEVICE_WRITE_ERROR   = 5,
        BLOCK_DEVICE_ERROR         = 6,
        BLOCK_DEVICE_READ_EOF      = 7,
    };

    typedef struct sl_block_device_CSregs sl_block_device_CSregs_t;

    struct sl_block_device_CSregs {
        uint32_t m_status;
        uint32_t m_buffer;
        uint32_t m_op;
        uint32_t m_lba;
        uint32_t m_count;
        uint32_t m_size;
        uint32_t m_block_size;
        uint32_t m_irqen;
        uint32_t m_irq;
        uint32_t m_finished_block_count;
    };

    enum sl_block_master_status {
        MASTER_READY        = 0,
        MASTER_CMD_SUCCESS  = 1,
        MASTER_CMD_ERROR    = 2,
    };

    class soclib_blockdevice:
        public device_master_slave
    {
    public:
        sc_out<bool>        irq;

        SC_HAS_PROCESS(soclib_blockdevice);

        soclib_blockdevice(sc_module_name _name, uint32_t master_id, const char *fname, uint32_t block_size);
        ~soclib_blockdevice();

        void end_of_elaboration();
        std::vector< symbol_base* > * get_symbols();

        // Slave Interface i.e. for blockdevice user modules like EU etc. (R/W Registers)
        void slv_read (uint32_t *addr, uint32_t *data);
        void slv_write (uint32_t *addr, uint32_t data);

    private:
        // Master Attributes
        uint32_t   m_status;
        uint8_t    m_crt_tid;
        uint32_t   m_node_id;

        uint32_t   m_transfer_size;
        uint8_t   *m_data;
        int        m_fd;

        /* simulation facilities */
        sc_event  ev_irq_update;
        sc_event  ev_op_start;

        /* Control and status register bank */
        sl_block_device_CSregs_t *m_cs_regs;

        void control_thread(void);
        void irq_update_thread(void);
        void open_host_file (const char *fname);

        static sc_attribute < uint32_t >    *_nb_blockdevices;          // Shared between all Block Device Objects
        sc_attribute < uint32_t >           *p_should_enable_irq;
        sc_attribute < uint32_t >           *p_irq;

        // I overriding the non-static symbols vector of device_master_slave
        // class with a static class member here. Required for the dna_link stage. -- Hamayun
        static std::vector< symbol_base *>         _symbols;
        static symbol<uint32_t>                    *blkdevs_baddrs_sym;
    };
}
#endif				// __SOCLIB_BLOCKDEVICE_H__
