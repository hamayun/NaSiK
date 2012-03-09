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

#ifndef EXECUTE_PACKET_H
#define	EXECUTE_PACKET_H

#include <ostream>

#include "Instruction.h"
#include "InstructionDecoder.h"
#include "DecodedInstruction.h"

using namespace std;

namespace native
{
    class BasicBlock;

    typedef list<Instruction *>                     InstructionList_t;
    typedef list<Instruction *> ::iterator          InstructionList_Iterator_t;
    typedef list<Instruction *> ::const_iterator    InstructionList_ConstIterator_t;

    typedef set <BasicBlock *>                      BasicBlockSet_t;
    typedef set <BasicBlock *> ::iterator           BasicBlockSet_Iterator_t;
    typedef set <BasicBlock *> ::const_iterator     BasicBlockSet_ConstIterator_t;

    typedef enum ExecutePacketType
    {
        NORMAL_EXEC_PACKET = 0,       /* Contains Instructions */
        SPECIAL_EXEC_PACKET = 1       /* Contains Special Hints; Branch Taken / Not Taken */
    } ExecutePacketType_t;

    typedef enum SpecialPacketFlags
    {
        BRANCH_TAKEN = 0,             /* Indicates that a Branch is Taken; Must Return to Simulation Driver */
        POSSIBLE_BRANCH = 1           /* A Branch May be Taken Here. Check If PC has been updated in the last execute packet */
    } SpecialPacketFlags_t;

    class ExecutePacket
    {
    private:
        ExecutePacketType_t             m_packet_type;
        InstructionList_t               m_instr_list;
        SpecialPacketFlags_t            m_special_flags;

        BasicBlockSet_t                 m_parent_bb_set;  // We can have multiple parents basic blocks
        InstructionList_Iterator_t      m_instr_iterator; // Iterator Used to Get Instructions One by One

    public:
        ExecutePacket(ExecutePacketType_t packet_type) : m_packet_type(packet_type), m_special_flags(BRANCH_TAKEN) {}
        virtual ~ExecutePacket(){}

        virtual void SetPacketType(ExecutePacketType_t packet_type) { m_packet_type = packet_type; }
        virtual ExecutePacketType_t GetPacketType() const { return (m_packet_type); }

        virtual void SetSpecialFlags(SpecialPacketFlags_t special_flags) { m_special_flags = special_flags; }
        virtual SpecialPacketFlags_t GetSpecialFlags() const { return (m_special_flags); }

        virtual void PushBack(Instruction * instruction)
        {
            m_instr_list.push_back(instruction);
            instruction->SetParentExecutePacket(this);
        }

        virtual void AddParentBasicBlock(BasicBlock * basic_block)
        {
            m_parent_bb_set.insert(basic_block);
        }

        virtual uint32_t GetSize() const { return (m_instr_list.size()); }

        virtual uint32_t GetTargetAddress() const { return (GetInstrByIndex(0)->GetAddress()); }

        virtual string GetName() const
        {
            std::string ep_name = "EP_";
            std::stringstream addr_string;

            if(GetSize())
            {
                addr_string << setw(8) << setfill('0') << hex << GetTargetAddress();
                ep_name += addr_string.str();
            }
            return (ep_name);
        }

        virtual uint32_t GetBranchTargetInstructionAddress() const
        {
            DecodedInstruction * dec_instr = NULL;

            for(InstructionList_ConstIterator_t ILCI = m_instr_list.begin(),
                ILCE = m_instr_list.end(); ILCI != ILCE; ++ILCI)
            {
                dec_instr = (*ILCI)->GetDecodedInstruction();
                ASSERT(dec_instr != NULL, "Instructions need to be decoded first.");

                if(dec_instr->IsBranchTarget())
                    return((*ILCI)->GetAddress());
            }
            return(NULL);
        }

        virtual uint32_t GetBranchTargetInstructionCount() const
        {
            DecodedInstruction * dec_instr = NULL;
            uint32_t branch_target_count = 0;

            for(InstructionList_ConstIterator_t ILCI = m_instr_list.begin(),
                ILCE = m_instr_list.end(); ILCI != ILCE; ++ILCI)
            {
                dec_instr = (*ILCI)->GetDecodedInstruction();
                ASSERT(dec_instr != NULL, "Instructions need to be decoded first.");

                if(dec_instr->IsBranchTarget())
                    branch_target_count++;
            }
            return(branch_target_count);
        }

        virtual uint32_t GetBranchInstructionsCount() const
        {
            DecodedInstruction * dec_instr = NULL;
            uint32_t branch_instrs_count = 0;

            for(InstructionList_ConstIterator_t ILCI = m_instr_list.begin(),
                ILCE = m_instr_list.end(); ILCI != ILCE; ++ILCI)
            {
                dec_instr = (*ILCI)->GetDecodedInstruction();
                ASSERT(dec_instr != NULL, "Instructions need to be decoded first.");

                if(dec_instr->IsBranchInstruction())
                    branch_instrs_count++;
            }
            return(branch_instrs_count);
        }

        virtual uint32_t GetUnconditionalBranchInstructionsCount() const
        {
            DecodedInstruction * dec_instr = NULL;
            uint32_t uncond_branch_count = 0;

            for(InstructionList_ConstIterator_t ILCI = m_instr_list.begin(),
                ILCE = m_instr_list.end(); ILCI != ILCE; ++ILCI)
            {
                dec_instr = (*ILCI)->GetDecodedInstruction();
                ASSERT(dec_instr != NULL, "Instructions need to be decoded first.");

                if(dec_instr->IsBranchInstruction() && (!dec_instr->IsCondtional()))
                    uncond_branch_count++;
            }
            return(uncond_branch_count);
        }

        virtual void Print(ostream *out) const
        {
            if(GetPacketType() == NORMAL_EXEC_PACKET)
            {
                DecodedInstruction * dec_instr = NULL;

                for(InstructionList_ConstIterator_t ILCI = m_instr_list.begin(),
                    ILCE = m_instr_list.end(); ILCI != ILCE; ++ILCI)
                {
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
            else if(GetPacketType() == SPECIAL_EXEC_PACKET)
            {
                switch(GetSpecialFlags())
                {
                    case BRANCH_TAKEN:
                        (*out) << setw(8) << setfill(' ') << "" << "<< Return >>" << endl;
                        break;

                    case POSSIBLE_BRANCH:
                        (*out) << setw(8) << setfill(' ') << "" << "<< Branch Taken / Not Taken >>" << endl;
                        break;
                }
            }
            (*out) << setw(8) << setfill(' ') << "" << setw(72) << setfill('-') << "" << endl;
        }

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

        virtual void ResetInstrIterator()
        {
            m_instr_iterator = m_instr_list.begin();
        }

        virtual Instruction * GetNextInstruction()
        {
            Instruction * instruction = NULL;

            if(m_instr_iterator != m_instr_list.end())
            {
                instruction = (*m_instr_iterator);
                m_instr_iterator++;
            }

            return(instruction);
        }
    };
}
#endif	/* EXECUTE_PACKET_H */

