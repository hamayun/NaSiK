/*************************************************************************************
 * Copyright (C)    2011 TIMA Laboratory
 * Author(s) :      Mian Muhammad, HAMAYUN mian-muhammad.hamayun@imag.fr
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

#ifndef EXECUTE_PACKET_LIST_H
#define EXECUTE_PACKET_LIST_H

#include "Common.h"
#include "InstructionList.h"
#include "ExecutePacket.h"

namespace native
{
    typedef list<ExecutePacket *>                   ExecutePacketList_t;
    typedef list<ExecutePacket *> :: iterator       ExecutePacketList_Iterator_t;
    typedef list<ExecutePacket *> :: const_iterator ExecutePacketList_ConstIterator_t;

    class ExecutePacketList
    {
    private:
        ExecutePacketList_t               m_exec_packets_list;

    public:
        ExecutePacketList(InstructionList * instr_list);
        virtual ~ExecutePacketList(){}

        virtual void Print(ostream *out) const
        {
            for(ExecutePacketList_ConstIterator_t EPLI = m_exec_packets_list.begin(),
                    EPLE = m_exec_packets_list.end(); EPLI != EPLE; ++EPLI)
            {
                (*EPLI)->Print(out);
            }
        }

        virtual uint32_t GetSize() const
        {
            return (m_exec_packets_list.size());
        }

        virtual void Remove(ExecutePacket * exec_pkt)
        {
            m_exec_packets_list.remove(exec_pkt);
        }

        virtual ExecutePacketList_t * GetExecutePacketList()
        {
            return (&m_exec_packets_list);
        }
    };
}

#endif	/* EXECUTE_PACKET_LIST_H */

