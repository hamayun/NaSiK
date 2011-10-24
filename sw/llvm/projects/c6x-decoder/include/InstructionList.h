
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

#ifndef INSTRUCTION_LIST_H
#define INSTRUCTION_LIST_H

#include "Instruction.h"
#include "DecodedInstruction.h"

using namespace std;

namespace native
{
    typedef list<Instruction *>                     InstructionList_t;
    typedef list<Instruction *> :: iterator         InstructionList_Iterator_t;
    typedef list<Instruction *> :: const_iterator   InstructionList_ConstIterator_t;

    class InstructionList
    {
    protected:
        InstructionList_t       m_instr_list;

    public:
        InstructionList(){};
        virtual ~InstructionList() {}

        void PushBack(Instruction * instruction)
        {
            m_instr_list.push_back(instruction);
        }

        virtual uint32_t GetSize() const { return (m_instr_list.size()); }
        virtual InstructionList_t * GetInstructionList() { return (&m_instr_list); }

        virtual Instruction * GetInstructionByAddress (uint32_t address)
        {
            for(InstructionList_Iterator_t ILI = m_instr_list.begin(),
                ILE = m_instr_list.end(); ILI != ILE; ++ILI)
            {
                if((*ILI)->GetAddress() == address)
                {
                    return (*ILI);
                }
            }

            return(NULL);
        }

        virtual int32_t LinkDecodedInstructions()
        {
            DecodedInstruction * prev_dec_instr = NULL;
            DecodedInstruction * curr_dec_instr = NULL;

            for(InstructionList_Iterator_t ILI = m_instr_list.begin(), ILE = m_instr_list.end();
                ILI != ILE; ++ILI)
            {
                curr_dec_instr = (*ILI)->GetDecodedInstruction();

                if(curr_dec_instr)
                    curr_dec_instr->SetPrevInstruction(prev_dec_instr);

                if(prev_dec_instr)
                    prev_dec_instr->SetNextInstruction(curr_dec_instr);

                prev_dec_instr = curr_dec_instr;
            }

            return (0);
        }

        virtual int32_t MarkBranchTargets()
        {
            for(InstructionList_ConstIterator_t ILI = m_instr_list.begin(),
                ILE = m_instr_list.end(); ILI != ILE; ++ILI)
            {
                DecodedInstruction * dec_instr = (*ILI)->GetDecodedInstruction();
                ASSERT(dec_instr != NULL, "Instructions need to be Decoded First, For Marking Branch Targets");

                if(dec_instr->IsBranchInstruction())
                {
                    uint32_t branch_target_addr  = dec_instr->GetBranchConstAddress();

                    // If the address is a valid one
                    if(branch_target_addr)
                    {
                        Instruction * target_instr = GetInstructionByAddress(branch_target_addr);

                        DecodedInstruction * target_dec_instr = target_instr->GetDecodedInstruction();
                        ASSERT(target_dec_instr != NULL, "Instructions need to be Decoded First, For Marking Branch Targets");

                        // Now mark the target decoded instruction as branch target.
                        target_dec_instr->SetBranchTarget(true);
                    }
                }

                // Also Check if the current Instruction is a label? If yes Mark it as Branch Target as well.
                if((*ILI)->HasLabel())
                {
                    DecodedInstruction * dec_instr = (*ILI)->GetDecodedInstruction();
                    ASSERT(dec_instr != NULL, "Instructions need to be Decoded First, For Marking Branch Targets");
                    dec_instr->SetBranchTarget(true);
                }
            }

            return (0);
        }

        virtual void Print (ostream *out) const
        {
            for(InstructionList_ConstIterator_t ILI = m_instr_list.begin(),
                ILE = m_instr_list.end(); ILI != ILE; ++ILI)
            {
                DecodedInstruction * dec_instr = NULL;

                dec_instr = (*ILI)->GetDecodedInstruction();
                if(dec_instr)
                   dec_instr->Print(out);
                else
                {
                    (*ILI)->Print(out);
                    (*out) << endl;
                }
            }
        }
    };
}
#endif // INSTRUCTION_LIST_H
