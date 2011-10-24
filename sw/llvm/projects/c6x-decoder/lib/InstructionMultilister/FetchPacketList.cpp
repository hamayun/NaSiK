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

#include "InstructionList.h"
#include "FetchPacketList.h"

namespace native
{
    FetchPacketList :: FetchPacketList(InstructionList * instr_list)
    {
        InstructionList_t * list = instr_list->GetInstructionList();
        FetchPacket       * fetch_packet = NULL;
        uint32_t            instr_count  = 0;

        for(InstructionList_ConstIterator_t ILI = list->begin(), ILE = list->end(); ILI != ILE; ++ILI)
        {
            if(!fetch_packet)
            {
                fetch_packet = new FetchPacket();
                if(!fetch_packet)
                {
                    DOUT << "Error: Allocating Memory for Fetch Packet" << endl;
                    return;
                }
            }

            fetch_packet->PushBack(*ILI);
            instr_count++;

            if(instr_count >= FETCH_PACKET_SIZE)
            {
                m_fetch_packets_list.push_back(fetch_packet);
                fetch_packet = NULL;
                instr_count = 0;
            }
        }

        ASSERT(instr_count == 0, "Instruction Count Not Zero, Means Fetch Packet Size is Probably Wrong");
    }
}
