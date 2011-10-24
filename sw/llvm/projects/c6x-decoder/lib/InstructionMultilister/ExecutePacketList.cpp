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

#include "ExecutePacketList.h"

namespace native
{
    ExecutePacketList :: ExecutePacketList(InstructionList * instr_list)
    {
        InstructionList_t * list = instr_list->GetInstructionList();
        ExecutePacket     * exec_packet = NULL;

        for(InstructionList_Iterator_t ILI = list->begin(), ILE = list->end(); ILI != ILE; ++ILI)
        {
            if(!exec_packet)
            {
                exec_packet = new ExecutePacket(NORMAL_EXEC_PACKET);
                if(!exec_packet)
                {
                    DOUT << "Error: Allocating Memory for Execute Packet" << endl;
                    return;
                }
            }

            // Push the Current Instruction into the Execute Packet
            exec_packet->PushBack(*ILI);

            // Check if this marks the end of Execute Packet
            if(!((*ILI)->GetValue() & 0x1))
            {
                // Insert into list
                m_exec_packets_list.push_back(exec_packet);
                exec_packet = NULL;
            }
        }
    }
}
