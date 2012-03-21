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

#ifndef DECODED_INSTRUCTION_H
#define DECODED_INSTRUCTION_H

#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/GlobalVariable.h"

#include "C62xCommon.h"
#include "Instruction.h"
#include "DecodedInstruction.h"

using namespace llvm;

namespace native
{
    class LLVMGenerator;

    // Generic Base Class for All Types of Decoded Instructions.
    class DecodedInstruction
    {
    private:
        Instruction            *p_parent_instr;
        bool                    m_is_conditional;
        string                  m_mnemonic;
        C62xPrintMode_t         m_preferred_print_mode;     // The preferred format for Constant Operands Printing.
        bool                    m_is_load_store;            // Is this a Load or Store Instruction.
        bool                    m_is_load;
        bool                    m_is_store;
        bool                    m_is_branch_instr;          // Is this a branch instruction.
        bool                    m_is_reg_branch;            // Is this a register branch.
        bool                    m_is_branch_target;         // Is this instruction target of a branch instruction.
        bool                    m_is_nop_instr;             // Is this a NOP instruction
        uint32_t                m_branch_cst_addr;          // The branch constant address. Needed for Marking Branch Targets in InstructionList class

        DecodedInstruction     *p_prev_instr;               // So that we can track our neighbours.
        DecodedInstruction     *p_next_instr;

    public:
        DecodedInstruction(Instruction * instr) :
            p_parent_instr(instr), m_is_conditional(false), m_preferred_print_mode(MODE_HEX),
            m_is_load_store(false), m_is_branch_instr(false), m_is_reg_branch(false), m_is_branch_target(false),
            m_is_nop_instr(false),  m_branch_cst_addr(0x0), p_prev_instr(NULL), p_next_instr(NULL) {}

        virtual Instruction * GetParentInstruction() const { return (p_parent_instr); }

        virtual void SetMnemonic(string mnemonic) { m_mnemonic = mnemonic; }
        virtual string GetMnemonic() const { return(m_mnemonic); }

        virtual void SetCondtional(bool is_conditional) { m_is_conditional = is_conditional; }
        virtual bool IsCondtional() const { return (m_is_conditional); }

        virtual void SetLoadStoreInstruction(bool is_load_store) { m_is_load_store = is_load_store; }
        virtual bool IsLoadStoreInstruction() const { return (m_is_load_store); }

        virtual void SetLoadInstr(bool is_load) { m_is_load = is_load; }
        virtual bool IsLoadInstr() const { return (m_is_load); }

        virtual void SetStoreInstr(bool is_store) { m_is_store = is_store; }
        virtual bool IsStoreInstr() const { return (m_is_store); }

        virtual void SetBranchInstruction(bool is_branch_instr) { m_is_branch_instr = is_branch_instr; }
        virtual bool IsBranchInstruction() const { return (m_is_branch_instr); }

        virtual void SetRegisterBranch(bool is_reg_branch) { m_is_reg_branch = is_reg_branch; }
        virtual bool IsRegisterBranch() const { return (m_is_reg_branch); }

        virtual void SetBranchTarget(bool is_branch_target) { m_is_branch_target = is_branch_target; }
        virtual bool IsBranchTarget() const { return (m_is_branch_target); }

        virtual void SetNOPInstruction(bool is_nop_instr) { m_is_nop_instr = is_nop_instr; }
        virtual bool IsNOPInstruction() const { return (m_is_nop_instr); }
        virtual uint8_t GetNOPCount() const { DOUT << "Not Implemented !!!"; return (0); }

        virtual void SetBranchConstAddress(uint32_t branch_cst_addr) { m_branch_cst_addr = branch_cst_addr; }
        virtual uint32_t GetBranchConstAddress() const { return (m_branch_cst_addr); }

        virtual void SetDelaySlots(uint8_t delay_slots) = 0;
        virtual uint8_t GetDelaySlots() const = 0;

        virtual void SetParallel(bool is_parallel) = 0;
        virtual bool IsParallel() const = 0;

        virtual void SetPreferredPrintMode(C62xPrintMode_t preferred_print_mode) { m_preferred_print_mode = preferred_print_mode; }
        virtual C62xPrintMode_t GetPreferredPrintMode() const { return(m_preferred_print_mode); }

        virtual void SetPrevInstruction(DecodedInstruction * prev_instr) { p_prev_instr = prev_instr; }
        virtual DecodedInstruction * GetPrevInstruction() const { return (p_prev_instr); }

        virtual void SetNextInstruction(DecodedInstruction * next_instr) { p_next_instr = next_instr; }
        virtual DecodedInstruction * GetNextInstruction() const { return (p_next_instr); }

        virtual string GetFunctionBaseName() const { return("C62x" + m_mnemonic); }

        virtual void Print (ostream *out) const = 0;

        // This function will Insert a call to LLVM IR Function using details provided by LLVMGenerator Object
        virtual llvm::Value * CreateLLVMFunctionCall(LLVMGenerator * llvm_gen, Module * out_mod, llvm::Value * result) const = 0;

        virtual ~DecodedInstruction() {}
    };
}
#endif // DECODED_INSTRUCTION_H
