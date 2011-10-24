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

#ifndef BASIC_BLOCK_LIST_H
#define BASIC_BLOCK_LIST_H

#include "Common.h"
#include "Instruction.h"
#include "BasicBlock.h"

namespace native
{
    typedef list<BasicBlock *>                     BasicBlockList_t;
    typedef list<BasicBlock *> ::iterator          BasicBlockList_Iterator_t;
    typedef list<BasicBlock *> ::const_iterator    BasicBlockList_ConstIterator_t;
    
    class BasicBlockList
    {
    private:
        BasicBlockList_t               m_basic_blocks_list;
        
    public:
        BasicBlockList(ExecutePacketList * exec_packets_list);
        virtual ~BasicBlockList() {}

        virtual int32_t AddMarkerExecutePackets();

        virtual void Print(ostream *out) const
        {
            for(BasicBlockList_ConstIterator_t BBLCI = m_basic_blocks_list.begin(),
                BBLCE = m_basic_blocks_list.end(); BBLCI != BBLCE; ++BBLCI)
            {
                (*BBLCI)->Print(out);
                (*out) << setw(8) << setfill(' ') << "" << setw(72) << setfill('=') << "" << endl;
            }
        }

        virtual uint32_t GetSize() const
        {
            return (m_basic_blocks_list.size());
        }

        virtual const BasicBlockList_t * GetBasicBlockList() const
        {
            return (&m_basic_blocks_list);
        }

        virtual BasicBlock * GetBasicBlockByIndex(uint32_t index) const
        {
            uint32_t curr_index = 0;

            for(BasicBlockList_ConstIterator_t BBLCI = m_basic_blocks_list.begin(),
                BBLCE = m_basic_blocks_list.end(); BBLCI != BBLCE; ++BBLCI)
            {
                if(curr_index == index)
                {
                    return (*BBLCI);
                }
                curr_index++;
            }
            return (NULL);
        }
    };
}

#endif	/* BASIC_BLOCK_LIST_H */

