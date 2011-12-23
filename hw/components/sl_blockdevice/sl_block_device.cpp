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

#include <systemc.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sl_block_device.h>
#include <errno.h>

//#define DEBUG_DEVICE_BLOCK

#ifdef DEBUG_DEVICE_BLOCK
#define DPRINTF(fmt, args...)                               \
    do { printf("%s: " fmt, get_name(), ##args); } while (0)
#define DCOUT if (1) cout
#else
#define DPRINTF(fmt, args...) do {} while(0)
#define DCOUT if (0) cout
#endif

#define EPRINTF(fmt, args...)                               \
    do { fprintf(stderr, "sl_block_device: " fmt , ##args); } while (0)

sl_block_device::sl_block_device (sc_module_name _name, uint32_t master_id, const char *fname, uint32_t block_size)
:sc_module(_name)
{
    char *buf = new char[strlen(_name) + 3];
    uint32_t slave_id = master_id;

    m_cs_regs = new sl_block_device_CSregs_t;

    m_cs_regs->m_status = 0;
    m_cs_regs->m_buffer = 0;
    m_cs_regs->m_op = 0;
    m_cs_regs->m_lba = 0;
    m_cs_regs->m_count = 0;
    m_cs_regs->m_size = 0;
    m_cs_regs->m_block_size = block_size;
    m_cs_regs->m_irqen = 0;
    m_cs_regs->m_irq   = 0;

    p_name = new char[strlen(_name)];
    strcpy(p_name, _name);

    strcpy(buf, _name);
    buf[strlen(_name)] = '_';
    buf[strlen(_name)+1] = 'm';
    buf[strlen(_name)+2] = '\0';
    master = new sl_block_device_master(buf, master_id);

    buf[strlen(_name)+1] = 's';
    slave = new sl_block_device_slave(buf, m_cs_regs, &ev_op_start, &ev_irq_update, slave_id);

    m_fd = -1;
    open_host_file (fname);

    SC_THREAD (irq_update_thread);
    SC_THREAD (control_thread);

}

sl_block_device::~sl_block_device ()
{
    DPRINTF("Destructor Called\n");
}

void sl_block_device::open_host_file (const char *fname)
{
    if (m_fd != -1)
    {
        ::close (m_fd);
        m_fd = -1;
    }
    if (fname)
    {
        m_fd = ::open(fname, O_RDWR | O_CREAT, 0644);
        if(m_fd < 0)
        {
            printf("Impossible to open file : %s\n", fname);
            return;
        }

        m_cs_regs->m_size = (lseek (m_fd, 0, SEEK_END) + m_cs_regs->m_block_size - 1) / m_cs_regs->m_block_size;
    }
}

master_device *
sl_block_device::get_master ()
{
    return (master_device *)master;
}

slave_device *
sl_block_device::get_slave ()
{
    return (slave_device *)slave;
}

void
sl_block_device::control_thread ()
{

    int func_ret = 0;
    uint32_t offset = 0;
    uint32_t addr = 0;
    off_t seek_result = 0;
    uint32_t transfer_size_words = 0;

    while(1)
    {

        switch(m_cs_regs->m_op)
        {

        case BLOCK_DEVICE_NOOP:
            DPRINTF("Got a BLOCK_DEVICE_NOOP\n");
            wait(ev_op_start);
            break;
        case BLOCK_DEVICE_READ:
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            m_transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            m_data = new uint8_t[m_transfer_size];

            DPRINTF("Got a BLOCK_DEVICE_READ of size %d bytes\n", m_transfer_size);

            /* Read in device */
            DPRINTF("LSEEK (SEEK_SET) with File Offset: %d.\n", m_cs_regs->m_lba*m_cs_regs->m_block_size);
            seek_result = lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            if (seek_result == -1) // need to check the errorno
            {
                switch (errno)
                {
                    case EBADF:
                            printf("fd is not an open file descriptor.\n");
                            break;
                    case EINVAL:
                            printf("whence is not one of SEEK_SET, SEEK_CUR, SEEK_END; "
                                   "or the resulting file offset would be negative, or beyond the end of a seekable device.\n");
                            break;
                    case EOVERFLOW:
                            printf("The resulting file offset cannot be represented in an off_t.\n");
                            break;
                    case ESPIPE:
                            printf("fd is associated with a pipe, socket, or FIFO.\n");
                            break;
                    default:
                            printf("errno is unknown.\n");
                            break;
                }
                break;
            }

            func_ret = ::read(m_fd, m_data, m_transfer_size);
            addr = m_cs_regs->m_buffer;
            if (func_ret < 0)
            {
                EPRINTF("Error in ::read()\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
                m_cs_regs->m_count = 0;
                m_cs_regs->m_finished_block_count = 0;
                break;
            }
#if 0
            if (func_ret == 0)
            {
                EPRINTF("Reach the EOF of source file.\n");
                printf("Reach the EOF of source file.\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_EOF;
                m_cs_regs->m_count = 0;
                m_cs_regs->m_finished_block_count = 0;
                break;
            }
#endif
            m_cs_regs->m_finished_block_count = func_ret / m_cs_regs->m_block_size;
            m_cs_regs->m_count = func_ret / m_cs_regs->m_block_size;

            /* Send data to memory */
            transfer_size_words = m_transfer_size / 4;

            /* Write Words */
            DPRINTF("Transferring %d Bytes _TO_ Memory Address 0x%08X\n", m_transfer_size, addr);
            for(offset = 0; offset < (transfer_size_words * 4); offset += 4){
                func_ret = master->cmd_write(addr + offset, m_data + offset, 4);
                if(!func_ret){
                    break;
                }
            }
            if(!func_ret){
                EPRINTF("Error in BlockDevice Read\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
                break;
            }

            /* Write Remaining Bytes */
            for(; offset < m_transfer_size; offset += 1){
                func_ret = master->cmd_write(addr + offset, m_data + offset, 1);
                if(!func_ret){
                    break;
                }
            }
            if(!func_ret){
                EPRINTF("Error in BlockDevice Read\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
                break;
            }

            /* Update everything */
            if(m_cs_regs->m_irqen){
                m_cs_regs->m_irq = 1;
                ev_irq_update.notify();
            }

            m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
            m_cs_regs->m_status = BLOCK_DEVICE_READ_SUCCESS;

            delete m_data;
            break;

        case BLOCK_DEVICE_WRITE:
            DPRINTF("Got a BLOCK_DEVICE_WRITE\n");
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            m_transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            m_data = new uint8_t[m_transfer_size];
            addr = m_cs_regs->m_buffer;

            /* Read data from memory */
            transfer_size_words = m_transfer_size / 4;

            /* Read Words */
            DPRINTF("Transferring %d Bytes _FROM_ Memory Address 0x%08X\n", m_transfer_size, addr);
            for(offset = 0; offset < (transfer_size_words * 4); offset += 4){
                func_ret = master->cmd_read(addr + offset, m_data + offset, 4);
                if(!func_ret){
                    break;
                }
            }
            if(!func_ret){
                EPRINTF("Error in BlockDevice Write\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
                break;
            }

            /* Read Remaining Bytes */
            for(; offset < m_transfer_size; offset += 1){
                func_ret = master->cmd_read(addr + offset, m_data + offset, 1);
                if(!func_ret){
                    break;
                }
            }
            if(!func_ret){
                EPRINTF("Error in BlockDevice Write\n");
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
                break;
            }

            /* Write in the device */
            lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            m_cs_regs->m_finished_block_count = ::write(m_fd, m_data, m_transfer_size);

            /* Update everything */
            if(m_cs_regs->m_irqen){
                m_cs_regs->m_irq = 1;
                ev_irq_update.notify();
            }

            m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
            m_cs_regs->m_status = BLOCK_DEVICE_WRITE_SUCCESS;
            delete m_data;

            break;

        default:
            EPRINTF("Error in command\n");
        }
    }
}

void sl_block_device::irq_update_thread ()
{
    while(1) {

        wait(ev_irq_update);

        if(m_cs_regs->m_irq == 1){
            DPRINTF("Raising IRQ\n");
            irq = 1;
        }else{
            DPRINTF("Clearing IRQ\n");
            irq = 0;
        }

    }
    return;
}

/*
 * sl_block_device_master
 */
sl_block_device_master::sl_block_device_master (const char *_name, uint32_t node_id)
: master_device(_name)
{
    m_crt_tid = 0;
    m_status  = MASTER_READY;
    m_node_id = node_id;

    p_name = new char[strlen(_name)];
    strcpy(p_name, _name);
}

sl_block_device_master::~sl_block_device_master()
{
}

int
sl_block_device_master::cmd_write (uint32_t addr, uint8_t *data, uint8_t nbytes)
{
    uint8_t tid;

    tid = m_crt_tid;

    //DPRINTF("Write to %x [0x%x]\n", addr, *(uint32_t *)data);

    send_req(tid, addr, data, nbytes, 1);

    wait(ev_cmd_done);

    if(m_status != MASTER_CMD_SUCCESS){
        return 0;
    }else{
        return 1;
    }
}

int
sl_block_device_master::cmd_read (uint32_t addr, uint8_t *data, uint8_t nbytes)
{
    uint8_t  tid;
    int i = 0;

    tid = m_crt_tid;

    m_tr_addr   = addr;
    m_tr_nbytes = nbytes;
    m_tr_rdata  = NULL;

    //DPRINTF("Read from 0x%x with nbytes 0x%x\n", addr, nbytes);

    send_req(tid, addr, data, nbytes, 0);

    wait(ev_cmd_done);

    if(m_tr_rdata){
        for( i = 0; i < nbytes; i++){
            data[i] = m_tr_rdata[i];
        }
    }

    if( m_tr_rdata ){
        //DPRINTF("Read val: [0x%08x]\n", *(uint32_t *)data);
    }else{
        DPRINTF("Read NULL\n");
    }

    if(m_status != MASTER_CMD_SUCCESS){
        return 0;
    }else{
        return 1;
    }
}

void
sl_block_device_master::rcv_rsp(uint8_t tid, uint8_t *data,
                                bool bErr, bool bWrite)
{
    if(tid != m_crt_tid){
        EPRINTF("Bad tid (%d / %d)\n", tid, m_crt_tid);
    }
    if(bErr){
        DPRINTF("Cmd KO\n");
        m_status = MASTER_CMD_ERROR;
    }else{
        //DPRINTF("Cmd OK\n");
        m_status = MASTER_CMD_SUCCESS;
    }

    if(!bWrite){
        //DPRINTF("Got data: 0x%08x - 0x%08x\n",
        //         *((uint32_t *)data + 0), *((uint32_t *)data + 1));
        //m_tr_rdata = *((uint8_t **)data);
        m_tr_rdata = (uint8_t *)data;
    }
    m_crt_tid++;

    ev_cmd_done.notify();

    return;
}

/*
 * sl_block_device_slave
 */
sl_block_device_slave::sl_block_device_slave (const char *_name,
                                              sl_block_device_CSregs_t *cs_regs,
                                              sc_event *op_start, sc_event *irq_update,
                                              uint32_t slave_id)
: slave_device (_name)
{

    m_cs_regs     = cs_regs;
    ev_op_start   = op_start;
    ev_irq_update = irq_update;
    m_slave_id    = slave_id;

    p_name = new char[strlen(_name)];
    strcpy(p_name, _name);
}

sl_block_device_slave::~sl_block_device_slave(){

}

void sl_block_device_slave::write (unsigned long ofs, unsigned char be,
                                    unsigned char *data, bool &bErr)
{
    uint32_t  *val = (uint32_t *)data;
    uint32_t   lofs = ofs;
    uint8_t    lbe  = be;

    bErr = false;

    lofs >>= 2;
    if (lbe & 0xF0)
    {
        lofs  += 1;
        lbe  >>= 4;
        val++;
    }

    switch(lofs){

    case BLOCK_DEVICE_BUFFER     :
        DPRINTF("BLOCK_DEVICE_BUFFER write: 0x%08X\n", *val);
        m_cs_regs->m_buffer = *val;
        break;
    case BLOCK_DEVICE_LBA        :
        DPRINTF("BLOCK_DEVICE_LBA write: %d\n", *val);
        m_cs_regs->m_lba = *val;
        break;
    case BLOCK_DEVICE_COUNT      :
        DPRINTF("BLOCK_DEVICE_COUNT write: %d\n", *val);
        m_cs_regs->m_count = *val;
        break;
    case BLOCK_DEVICE_OP         :
        DPRINTF("BLOCK_DEVICE_OP write: %d [NOP, READ, WRITE, FILE]\n", *val);
        if(m_cs_regs->m_status != BLOCK_DEVICE_IDLE){
            EPRINTF("Got a command while executing another one\n");
        }
        m_cs_regs->m_op = *val;
        ev_op_start->notify();
        break;
    case BLOCK_DEVICE_IRQ_ENABLE :
        DPRINTF("BLOCK_DEVICE_IRQ_ENABLE write: %d\n", *val);
        m_cs_regs->m_irqen = *val;
        ev_irq_update->notify();
        break;
    case BLOCK_DEVICE_STATUS     :
    case BLOCK_DEVICE_SIZE       :
    case BLOCK_DEVICE_BLOCK_SIZE :
    default:
        EPRINTF("Bad %s::%s ofs=0x%X, be=0x%X\n", name (), __FUNCTION__,
                (unsigned int) ofs, (unsigned int) be);
    }
    return;
}

void sl_block_device_slave::read (unsigned long ofs, unsigned char be,
                                  unsigned char *data, bool &bErr)
{

    uint32_t  *val = (uint32_t *)data;
    uint32_t   lofs = ofs;
    uint8_t    lbe  = be;

    bErr = false;

    lofs >>= 2;
    if (lbe & 0xF0)
    {
        lofs  += 1;
        lbe  >>= 4;
        val++;
    }

    switch(lofs){

    case BLOCK_DEVICE_BUFFER     :
        *val = m_cs_regs->m_buffer;
        DPRINTF("BLOCK_DEVICE_BUFFER read: 0x%08X\n", *val);
        break;
    case BLOCK_DEVICE_LBA        :
        *val = m_cs_regs->m_lba;
        DPRINTF("BLOCK_DEVICE_LBA read: %d\n", *val);
        break;
    case BLOCK_DEVICE_COUNT      :
        *val = m_cs_regs->m_count;
        DPRINTF("BLOCK_DEVICE_COUNT read: %d\n", *val);
        break;
    case BLOCK_DEVICE_OP         :
        *val = m_cs_regs->m_op;
        DPRINTF("BLOCK_DEVICE_OP read: %d [NOP, READ, WRITE, FILE]\n", *val);
        break;
    case BLOCK_DEVICE_STATUS     :
        *val = m_cs_regs->m_status;
        DPRINTF("BLOCK_DEVICE_STATUS read: %d [IDLE, BUSY, RDOK, WROK, RDER, WRER, DVER, REOF]\n", *val);
        if(m_cs_regs->m_status != BLOCK_DEVICE_BUSY){
            m_cs_regs->m_status = BLOCK_DEVICE_IDLE;
            m_cs_regs->m_irq = 0;
            ev_irq_update->notify();
        }
        break;
    case BLOCK_DEVICE_IRQ_ENABLE :
        *val = m_cs_regs->m_irqen;
        DPRINTF("BLOCK_DEVICE_IRQ_ENABLE read: %d\n", *val);
        break;
    case BLOCK_DEVICE_SIZE       :
        *val = m_cs_regs->m_size;
        DPRINTF("BLOCK_DEVICE_SIZE read: %d blocks\n", *val);
        break;
    case BLOCK_DEVICE_BLOCK_SIZE :
        *val = m_cs_regs->m_block_size;
        DPRINTF("BLOCK_DEVICE_BLOCK_SIZE read: %d bytes\n", *val);
        break;
    case BLOCK_DEVICE_FINISHED_BLOCK_COUNT :
        *val = m_cs_regs->m_finished_block_count;
        DPRINTF("BLOCK_DEVICE_FINISHED_BLOCK_COUNT read: %d\n", *val);
        break;
    default:
        EPRINTF("Bad %s::%s ofs=0x%X, be=0x%X\n", name (), __FUNCTION__,
                (unsigned int) ofs, (unsigned int) be);
    }
    return;
}

void sl_block_device_slave::rcv_rqst (unsigned long ofs, unsigned char be,
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
