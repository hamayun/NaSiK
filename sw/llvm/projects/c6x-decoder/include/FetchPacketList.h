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

#ifndef FETCH_PACKET_LIST_H
#define	FETCH_PACKET_LIST_H

#include <list>

#include "Common.h"
#include "InstructionList.h"
#include "FetchPacket.h"

namespace native
{
    typedef list<FetchPacket *>                   FetchPacketList_t;
    typedef list<FetchPacket *> :: iterator       FetchPacketList_Iterator_t;
    typedef list<FetchPacket *> :: const_iterator FetchPacketList_ConstIterator_t;

    class FetchPacketList
    {
    private:
        // List of instructions that make-up this execute packet
        FetchPacketList_t               m_fetch_packets_list;

    public:
        FetchPacketList(InstructionList * instr_list);
        virtual ~FetchPacketList(){}

        virtual void Print(ostream *out) const
        {
            for(FetchPacketList_ConstIterator_t FPLI = m_fetch_packets_list.begin(),
                    FPLE = m_fetch_packets_list.end(); FPLI != FPLE; ++FPLI)
            {
                (*FPLI)->Print(out);
                (*out) << endl;
            }
        }

        virtual uint32_t GetSize() const { return (m_fetch_packets_list.size()); }
        virtual FetchPacketList_t * GetFetchPacketList() { return (&m_fetch_packets_list); }
    };
}

#endif	/* FETCH_PACKET_LIST_H */
