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

#ifndef FETCH_PACKET_H
#define	FETCH_PACKET_H

#include "Common.h"
#include "Instruction.h"
#include "InstructionDecoder.h"
#include "DecodedInstruction.h"

namespace native
{
    typedef list<Instruction *>                     InstructionList_t;
    typedef list<Instruction *> ::iterator          InstructionList_Iterator_t;
    typedef list<Instruction *> ::const_iterator    InstructionList_ConstIterator_t;

    class FetchPacket
    {
    private:
        InstructionList_t               m_instr_list;

    public:
        FetchPacket(){}
        virtual ~FetchPacket(){}

        virtual void PushBack(Instruction * instruction)
        {
            m_instr_list.push_back(instruction);
            instruction->SetParentFetchPacket(this);
        }

        virtual uint32_t GetSize() const { return (m_instr_list.size()); }

        virtual Instruction * GetInstrByIndex(uint32_t index) const
        {
            uint32_t curr_index = 0;

            for(InstructionList_ConstIterator_t ILCI = m_instr_list.begin(),
                ILCE = m_instr_list.end(); ILCI != ILCE; ++ILCI)
            {
                if(curr_index == index)
                {
                    return (*ILCI);
                }
                curr_index++;
            }
            return (NULL);
        }

        virtual void Print(ostream *out) const
        {
            for(InstructionList_ConstIterator_t ILCI = m_instr_list.begin(),
                ILCE = m_instr_list.end(); ILCI != ILCE; ++ILCI)
            {
                DecodedInstruction * dec_instr = NULL;

                dec_instr = (*ILCI)->GetDecodedInstruction();
                if(dec_instr)
                   dec_instr->Print(out);
                else
                {
                    (*ILCI)->Print(out);
                    (*out) << endl;
                }
            }
        }
    };
}
#endif	/* FETCH_PACKET_H */

