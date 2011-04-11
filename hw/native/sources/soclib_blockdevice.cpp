/*************************************************************************************
 * File   : soclib_blockdevice.cpp,
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

#define DEBUG_OPTION "soclib_blockdevice"

#include "soclib_blockdevice.h"
#include "assertion.h"
#include "utils.h"
#include "debug.h"
#include "symbol.h"
#include "errno.h"
#include "symbol.h"
#include <fcntl.h>
#include <stdio.h>
#include <sstream>
#include <string>

using namespace native;
using namespace mapping;

#define DOUT_DATA if(0) cout

soclib_blockdevice::soclib_blockdevice(sc_module_name _name, uint32_t master_id, const char *fname, uint32_t block_size)
    :device_master_slave(_name)
{
    _generic_name = "BLOCKDEVICE";
    DOUT_CTOR << this->name() << std::endl;

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

    m_crt_tid = 0;
    m_status  = MASTER_READY;
    m_node_id = master_id;

    DOUT << name() << "[" << m_node_id << "]: " << "Opening File: " << fname << std::endl;
    m_fd = ::open(fname, O_RDWR);
    if(m_fd < 0){
        std::cerr << "[" << m_node_id << "]: " << "Error Opening File :" << fname << std::endl;
        return;
    }

    m_cs_regs->m_size = lseek(m_fd, 0, SEEK_END) / block_size;

    SC_THREAD (control_thread);
    SC_THREAD (irq_update_thread);
}

soclib_blockdevice::~soclib_blockdevice()
{
    ::close(m_fd);
    DOUT_DTOR << this->name() << std::endl;
}

sc_attribute < uint32_t > * soclib_blockdevice :: _nb_blockdevices;
std::vector< symbol_base *> soclib_blockdevice :: _symbols;
symbol<uint32_t>          * soclib_blockdevice :: blkdevs_baddrs_sym;

void soclib_blockdevice::end_of_elaboration()
{
    std::string             attr_name;
    std::string             strindex;
    std::ostringstream      ssindex;

    device_master_slave::end_of_elaboration();

    if (m_node_id == 0)
    {
        // Number of blockdevices; Push this symbol for blockdevice_0 only; 
        GET_ATTRIBUTE("NB_BLOCKDEVICES",_nb_blockdevices,uint32_t,false);
        DOUT << name() << ": NB_BLOCKDEVICES = " << _nb_blockdevices->value << std::endl;

        symbol<uint32_t>       *symbol_value;
        symbol_value = new symbol<uint32_t>("SOCLIB_BLOCK_DEVICES_NDEV");
        symbol_value->push_back(_nb_blockdevices->value);
        _symbols.push_back(symbol_value);
    }
    
    // These will be different for each block device instance. 
    _segments.push_back(mapping::init(name(), BLOCKDEVICE_SPAN * sizeof(uint32_t)));
    _registers.ptr = _segments[0]->base_addr;

    // The IRQ Stuff and Block Device Base Address
    if (m_node_id == 0)
    {
        blkdevs_baddrs_sym = new symbol<uint32_t>("SOCLIB_BLOCK_DEVICES");
    }

    ssindex << m_node_id;
    strindex = ssindex.str();

    attr_name = "ENABLE_IRQ_BLOCKDEVICE_";
    attr_name = attr_name + strindex;
    GET_ATTRIBUTE_STR(attr_name, p_should_enable_irq, uint32_t, false);
    DOUT << name() << ": " << attr_name << " = " << p_should_enable_irq->value << std::endl;
    blkdevs_baddrs_sym->push_back(p_should_enable_irq->value);

    attr_name = "IRQ_BLOCKDEVICE_";
    attr_name = attr_name + strindex;
    GET_ATTRIBUTE_STR(attr_name, p_irq, uint32_t, false);
    DOUT << name() << ": " << attr_name << " = " << p_irq->value << std::endl;
    blkdevs_baddrs_sym->push_back(p_irq->value);

    // Push Back the Base Address of Registers for this particuler block device instance.
    blkdevs_baddrs_sym->push_back(_registers.ptr);

    if ((blkdevs_baddrs_sym->size() / 3) == _nb_blockdevices->value)
    {
        // Now Actually Push the Symbol Values to the Symbol Vector.
        _symbols.push_back(blkdevs_baddrs_sym);
    }
}

std::vector< symbol_base* > * soclib_blockdevice :: get_symbols()
{
    // This function is called N times where N is the total number of blockdevices.
    // Also at the dynamic linking stage; our dna_linker will do the link N times, which is redundant
    // but should not create any problems for simulation. -- Hamayun
    return (&_symbols); 
}

void soclib_blockdevice::control_thread ()
{
    int func_ret = 0;
    uint32_t offset = 0;
    uint32_t addr = 0;

    while(1){

        switch(m_cs_regs->m_op){

        case BLOCK_DEVICE_NOOP:
            DOUT << "[" << m_node_id << "]: " << "Got a BLOCK_DEVICE_NOOP" << std::endl;
            wait(ev_op_start);
            break;
            
        case BLOCK_DEVICE_READ:
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            m_transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            DOUT << "[" << m_node_id << "]: " << "Got a BLOCK_DEVICE_READ of size: "
                 << std::dec << m_transfer_size << std::endl;

            m_data = new uint8_t[m_transfer_size];
            addr = m_cs_regs->m_buffer;

            /* Read in device */
            lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            func_ret = ::read(m_fd, m_data, m_transfer_size);
            if(func_ret < 0){
                std::cerr << __FUNCTION__ << "Error in ::read()\n";
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_READ_ERROR;
            }

            /* Send data to memory */
            for(offset = 0; offset < m_transfer_size; offset += 4){
                DOUT_DATA << "BDR: mst_write @" << std::hex << (uint32_t *)(addr + offset)
                          << " data: " << std::dec
                          << (uint8_t)*(m_data + offset + 0) << (uint8_t)*(m_data + offset + 1)
                          << (uint8_t)*(m_data + offset + 2) << (uint8_t)*(m_data + offset + 3) << std::endl;
                mst_write((uint32_t *)(addr + offset), *((uint32_t *)(m_data + offset)));
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
            m_cs_regs->m_status = BLOCK_DEVICE_BUSY;
            m_transfer_size = m_cs_regs->m_count * m_cs_regs->m_block_size;

            DOUT << "[" << m_node_id << "]: " << "Got a BLOCK_DEVICE_WRITE of size: "
                 << std::dec << m_transfer_size << std::endl;

            m_data = new uint8_t[m_transfer_size];
            addr = m_cs_regs->m_buffer;

            /* Read data from memory */
            for(offset = 0; offset < m_transfer_size; offset += 4){
                mst_read((uint32_t *)(addr + offset), (uint32_t *)(m_data + offset));
                DOUT_DATA << "BDW: mst_read @" << std::hex << (uint32_t *)(addr + offset)
                          << " data: " << std::dec
                          << (uint8_t)*(m_data + offset + 0) << (uint8_t)*(m_data + offset + 1)
                          << (uint8_t)*(m_data + offset + 2) << (uint8_t)*(m_data + offset + 3) << std::endl;
            }

            /* Write in the device */
            lseek(m_fd, m_cs_regs->m_lba*m_cs_regs->m_block_size, SEEK_SET);
            func_ret = ::write(m_fd, m_data, m_transfer_size);
            if(func_ret < 0){
                std::cerr << __FUNCTION__ << "Error in ::write()\n";
                m_cs_regs->m_op     = BLOCK_DEVICE_NOOP;
                m_cs_regs->m_status = BLOCK_DEVICE_WRITE_ERROR;
            }

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
            std::cerr << "Error in command\n";
        }
    }
}

void soclib_blockdevice::irq_update_thread ()
{
    while(1) {

        wait(ev_irq_update);

        if(m_cs_regs->m_irq == 1){
            DOUT << "[" << m_node_id << "]: " << "Raising IRQ" << std::endl;
            irq = 1;
        }else{
            DOUT << "[" << m_node_id << "]: " << "Clearing IRQ" << std::endl;
            irq = 0;
        }
    }
    return;
}

void soclib_blockdevice::slv_write (uint32_t *addr, uint32_t data)
{
    uint32_t  *val = (uint32_t *)&data;

    switch (REGISTER_INDEX(UINT32,addr))
    {
    case BLOCK_DEVICE_BUFFER     :
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_BUFFER write: Ox" << std::hex << *val << std::endl;
        m_cs_regs->m_buffer = *val;
        break;
    case BLOCK_DEVICE_LBA        :
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_LBA write: " << std::dec << *val << std::endl;
        m_cs_regs->m_lba = *val;
        break;
    case BLOCK_DEVICE_COUNT      :
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_COUNT write: " << std::dec << *val << std::endl;
        m_cs_regs->m_count = *val;
        break;
    case BLOCK_DEVICE_OP         :
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_OP write: " << std::dec << *val << std::endl;
        if(m_cs_regs->m_status != BLOCK_DEVICE_IDLE){
            std::cerr << "Got a command while executing another one" << std::endl;
        }
        m_cs_regs->m_op = *val;
        ev_op_start.notify();
        break;
    case BLOCK_DEVICE_IRQ_ENABLE :
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_IRQ_ENABLE write: " << std::dec << *val << std::endl;
        m_cs_regs->m_irqen = *val;
        ev_irq_update.notify();
        break;
    case BLOCK_DEVICE_STATUS     :
    case BLOCK_DEVICE_SIZE       :
    case BLOCK_DEVICE_BLOCK_SIZE :
    default:
        std::cerr << __FUNCTION__ << ": Bad Register Index: " << std::dec << REGISTER_INDEX(UINT32,addr)
                  << " Address: 0x" << std::hex << addr << " Data: " << std::dec << data << std::endl; 
    }
    return;
}

void soclib_blockdevice::slv_read (uint32_t *addr, uint32_t *data)
{
    uint32_t  *val = (uint32_t *)data;

    switch (REGISTER_INDEX(UINT32,addr))
    {
    case BLOCK_DEVICE_BUFFER     :
        *val = m_cs_regs->m_buffer;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_BUFFER read: Ox" << std::hex << *val << std::endl;
        break;
    case BLOCK_DEVICE_LBA        :
        *val = m_cs_regs->m_lba;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_LBA read: " << std::dec << *val << std::endl;
        break;
    case BLOCK_DEVICE_COUNT      :
        *val = m_cs_regs->m_count;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_COUNT read: " << std::dec << *val << std::endl;
        break;
    case BLOCK_DEVICE_OP         :
        *val = m_cs_regs->m_op;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_OP read: " << std::dec << *val << std::endl;
        break;
    case BLOCK_DEVICE_STATUS     :
        *val = m_cs_regs->m_status;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_STATUS read: " << std::dec << *val << std::endl;
        if(m_cs_regs->m_status != BLOCK_DEVICE_BUSY){
            m_cs_regs->m_status = BLOCK_DEVICE_IDLE;
            m_cs_regs->m_irq = 0;
            ev_irq_update.notify();
        }
        break;
    case BLOCK_DEVICE_IRQ_ENABLE :
        *val = m_cs_regs->m_irqen;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_IRQ_ENABLE read: " << std::dec << *val << std::endl;
        break;
    case BLOCK_DEVICE_SIZE       :
        *val = m_cs_regs->m_size;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_SIZE read: " << std::dec << *val << std::endl;
        break;
    case BLOCK_DEVICE_BLOCK_SIZE :
        *val = m_cs_regs->m_block_size;
        DOUT << "[" << m_node_id << "]: " << "BLOCK_DEVICE_BLOCK_SIZE read: " << std::dec << *val << std::endl;
        break;
    default:
        std::cerr << __FUNCTION__ << ": Bad Register Index: " << std::dec << REGISTER_INDEX(UINT32,addr)
                  << " Address: 0x" << std::hex << addr << " Data: " << std::dec << data << std::endl;
    }
    return;
}





