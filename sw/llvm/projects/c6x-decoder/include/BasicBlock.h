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

#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include "Common.h"
#include "ExecutePacket.h"

namespace native
{
    typedef set<uint32_t>                           ConstOffsetsSet_t;
    typedef set<uint32_t>::iterator                 ConstOffsetsSet_Iterator_t;
    typedef set<uint32_t>::const_iterator           ConstOffsetsSet_ConstIterator_t;

    typedef list<ExecutePacket *>                     ExecutePacketList_t;
    typedef list<ExecutePacket *> ::iterator          ExecutePacketList_Iterator_t;
    typedef list<ExecutePacket *> ::const_iterator    ExecutePacketList_ConstIterator_t;

    typedef set <ExecutePacket *>                     ExecutePacketSet_t;
    typedef set <ExecutePacket *> :: iterator         ExecutePacketSet_Iterator_t;
    typedef set <ExecutePacket *> :: const_iterator   ExecutePacketSet_ConstIterator_t;

    typedef list<ExecutePacketList_Iterator_t>              ExecutePacketIteratorList_t;
    typedef list<ExecutePacketList_Iterator_t> ::iterator   ExecutePacketIteratorList_Iterator_t;

    class BasicBlock
    {
    private:
        uint32_t                          m_bb_id;
        ExecutePacketList_t               m_exec_packets_list;

        // Some Target Basic Block Level Branch Statistics
        uint32_t                          m_total_br_cnt;
        uint32_t                          m_ucond_br_cnt;
        uint32_t                          m_cond_br_cnt;
        uint32_t                          m_offset_br_cnt;
        uint32_t                          m_register_br_cnt;

        ConstOffsetsSet_t                 m_offsets_set;  // A set that stores the constant offsets of branch instructions

    public:
        BasicBlock(uint32_t bb_id) : m_bb_id(bb_id), m_total_br_cnt(0), m_ucond_br_cnt(0),
                m_cond_br_cnt(0), m_offset_br_cnt(0), m_register_br_cnt(0) {}
        virtual ~BasicBlock() {}

        virtual int32_t AddMarkerExecutePackets()
        {
            ExecutePacketIteratorList_t branch_packet_ilist;     // List of Iterators to Packets Containing Branch Instructions

            // Find Iterators for Execute Packets Containing Branch Instructions.
            for(ExecutePacketList_Iterator_t EPLI = m_exec_packets_list.begin(),
                    EPLE = m_exec_packets_list.end(); EPLI != EPLE; ++EPLI)
            {
                if((*EPLI)->GetBrInstrCount())
                {
                    branch_packet_ilist.push_back(EPLI);
                }
            }

            if(branch_packet_ilist.size())
            {
                int32_t delay_slot_count;
                for(ExecutePacketIteratorList_Iterator_t EPILI = branch_packet_ilist.begin(),
                        EPILE = branch_packet_ilist.end(); EPILI != EPILE; ++EPILI)
                {
                    ExecutePacketList_Iterator_t EPLI = *EPILI;
                    ExecutePacketList_Iterator_t EPLE = m_exec_packets_list.end();

                    delay_slot_count = 0;
                    // Skip 'C62X_BRANCH_DELAY' number of Normal Execute Packets
                    while(delay_slot_count <= C62X_BRANCH_DELAY)
                    {
                        EPLI++;

                        /* This break can make us exit even before the delay slot
                         * count is satisfied; Like when we have less execute packets
                         * after a Branch Instruction then the required number  */
                        if(EPLI == EPLE) break;

                        if((*EPLI)->GetPacketType() == NORMAL_EXEC_PACKET)
                        {
                            delay_slot_count++;
                        }
                    }

                    ExecutePacket * marker_packet = new ExecutePacket(SPECIAL_EXEC_PACKET);

                    if(EPLI == EPLE)
                        marker_packet->SetSpecialFlags(BRANCH_TAKEN);
                    else
                        marker_packet->SetSpecialFlags(POSSIBLE_BRANCH);

                    m_exec_packets_list.insert(EPLI, marker_packet);
                }
            }

            return(0);
        }

        virtual void Print(ostream *out) const
        {
            (*out) << setw(8) << setfill(' ') << "" << "BasicBlock #: " << dec
                   << GetID() << " (" << GetName() << ")" << " [Branches: " << m_total_br_cnt
                   << ", UC: " << m_ucond_br_cnt << ", C: " << m_cond_br_cnt
                   << ", REG: " << m_register_br_cnt << ", CST: " << m_offset_br_cnt << "]" << endl;

            for(ExecutePacketList_ConstIterator_t EPLCI = m_exec_packets_list.begin(),
                    EPLCE = m_exec_packets_list.end(); EPLCI != EPLCE; ++EPLCI)
            {
                (*EPLCI)->Print(out);
            }
        }

        virtual uint32_t GetSize() const
        {
            return (m_exec_packets_list.size());
        }

        virtual uint32_t GetID() const
        {
            return (m_bb_id);
        }

        virtual uint32_t GetTargetAddress()
        {
            ExecutePacket * exec_pkt = GetFirstExecPacket();
            return (exec_pkt->GetInstrByIndex(0)->GetAddress());
        }

        virtual string GetName() const
        {
            std::string bb_name = "BB_";
            std::stringstream addr_string;

            if(GetSize())
            {
                ExecutePacketList_ConstIterator_t EPLCI = m_exec_packets_list.begin();
                addr_string << setw(8) << setfill('0') << hex << (*EPLCI)->GetInstrByIndex(0)->GetAddress();
                bb_name += addr_string.str();
            }
            return (bb_name);
        }

        virtual void PushBack(ExecutePacket * exec_packet)
        {
            m_exec_packets_list.push_back(exec_packet);
            exec_packet->AddParentBasicBlock(this);
        }

        virtual ExecutePacket * GetExecutePacketByIndex(uint32_t index) const
        {
            uint32_t curr_index = 0;

            for(ExecutePacketList_ConstIterator_t EPLCI = m_exec_packets_list.begin(),
                EPLCE = m_exec_packets_list.end(); EPLCI != EPLCE; ++EPLCI)
            {
                if(curr_index == index)
                {
                    return (*EPLCI);
                }
                curr_index++;
            }
            return (NULL);
        }

        virtual ExecutePacket * GetFirstExecPacket()
        {
            ExecutePacketList_Iterator_t EPI = m_exec_packets_list.begin();
            ExecutePacketList_Iterator_t EPE = m_exec_packets_list.end();

            if(EPI == EPE)
                return (NULL);
            else
                return (*EPI);
        }

        virtual uint32_t FillBranchInstrCounts()
        {
            uint32_t total_br_count    = 0;
            uint32_t ucond_br_count    = 0;
            uint32_t register_br_count = 0;

            for(ExecutePacketList_Iterator_t EPLI = m_exec_packets_list.begin(),
                EPLE = m_exec_packets_list.end(); EPLI != EPLE; ++EPLI)
            {
                total_br_count    += (*EPLI)->GetBrInstrCount();
                ucond_br_count    += (*EPLI)->GetUCBrInstrCount();
                register_br_count += (*EPLI)->GetRegBrInstrCount();

                ConstOffsetsSet_t * offsets =  (*EPLI)->GetConstOffsetsSet();
                for(ConstOffsetsSet_Iterator_t OI = offsets->begin(), OE = offsets->end(); OI != OE; ++OI)
                {
                    m_offsets_set.insert(*OI);
                }
            }

            m_total_br_cnt    = total_br_count;
            m_ucond_br_cnt    = ucond_br_count;
            m_cond_br_cnt     = total_br_count - ucond_br_count;
            m_register_br_cnt = register_br_count;
            m_offset_br_cnt   = total_br_count - register_br_count;

            return (0);
        }

        virtual uint32_t GetRegisterBrCount() { return(m_register_br_cnt); }
        virtual uint32_t GetOffsetBrCount() { return(m_offset_br_cnt); }
        virtual ConstOffsetsSet_t * GetConstOffsetsSet() { return(& m_offsets_set); }
    };
}

#endif /* BASIC_BLOCK_H */