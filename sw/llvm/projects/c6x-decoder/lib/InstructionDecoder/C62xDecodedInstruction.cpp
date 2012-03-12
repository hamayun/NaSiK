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

#include "BinaryReader.h"
#include "ExecutePacket.h"
#include "FetchPacket.h"
#include "C62xDecodedInstruction.h"
#include "LLVMGenerator.h"

using namespace llvm;

namespace native
{
    C62xDecodedInstruction :: C62xDecodedInstruction(Instruction * parent) :
        DecodedInstruction(parent)
    {
        parent->SetDecodedInstruction(this);
        m_creg_zero_test = false;
        m_is_parallel    = false;
        m_is_cross_path  = false;
        m_dest_side_id   = REG_BANK_A;
        m_opcode         = 0;
        p_unit           = NULL;
        p_condition_reg  = NULL;
        m_operand_count  = 0;
        m_delay_slots    = 0;
        p_dst_reg        = NULL;
        m_oprands.clear();
    }

    C62xDecodedInstruction :: C62xDecodedInstruction(C62xDecodedInstruction * dec_instr) :
        DecodedInstruction(dec_instr->GetParentInstruction())
    {
        dec_instr->GetParentInstruction()->SetDecodedInstruction(this);
        SetCondtional(dec_instr->IsCondtional());
        m_creg_zero_test = dec_instr->IsCRegZeroTest();
        m_is_parallel    = dec_instr->IsParallel();
        m_is_cross_path  = dec_instr->IsCrossPath();
        m_dest_side_id   = dec_instr->GetDestinationSideId();
        m_opcode         = dec_instr->GetOpcode();
        p_unit           = dec_instr->GetExecutionUnit();
        p_condition_reg  = dec_instr->GetConditionRegister();
        m_operand_count  = dec_instr->GetOperandCount();
        m_delay_slots    = dec_instr->GetDelaySlots();
        p_dst_reg        = dec_instr->GetDestRegister();
        m_oprands.clear();

        if(m_operand_count)
        {
            for(C62xOperandList_Iterator_t OLI = m_oprands.begin(),
                OLE = m_oprands.end(); OLI != OLE; ++OLI)
            {
                m_oprands.push_back(*OLI);
            }
        }

        ASSERT(m_operand_count == m_oprands.size(), "Operands Cound and List Size Mis-match !!!");
    }

    void C62xDecodedInstruction :: AddOperand(C62xOperand * operand)
    {
        if(operand)
        {
            m_oprands.push_back(operand);
            SetOperandCount(GetOperandCount() + 1);
            operand->SetParentInstruction(this);
        }
    }

    C62xOperand * C62xDecodedInstruction :: GetOperand(uint32_t index) const
    {
        uint32_t curr_index = 0;
        if (index >= GetOperandCount())
        {
            DOUT << "Index > OperandCount" << endl;
            return (NULL);
        }

        for(C62xOperandList_ConstIterator_t OLCI = m_oprands.begin(),
            OLCE = m_oprands.end(); OLCI != OLCE; ++OLCI)
        {
            if(curr_index == index)
            {
                return (*OLCI);
            }
            curr_index++;
        }

        return (NULL);
    }

    void C62xDecodedInstruction :: Print (ostream *out) const
    {
        #ifdef PRINT_BRANCH_TARGET_HINTS
        if(IsBranchTarget())
            (*out) << " >>---> ";
        else
            (*out) << setw(8) << setfill(' ') << "";
        #endif

        GetParentInstruction()->Print(out);

        /* See if we have a previous instruction ?
         * Then see whether the current instruction is mentioned as parallel or not ? */
        DecodedInstruction * prev_dec_instr = GetPrevInstruction();
        if(prev_dec_instr && prev_dec_instr->IsParallel())
            (*out) << " ||";
        else
            (*out) << "   ";

        if(IsCondtional())
        {
            (*out) << " [" << (IsCRegZeroTest() == true ? "!" : " ");
            GetConditionRegister()->Print(out);
            (*out) << "] ";
        }
        else
        {
            (*out) << setw(7) << setfill(' ') << left << "";
        }

        (*out) << setw(7) << setfill(' ') << right << GetMnemonic();
        #ifdef PRINT_INSTR_OPCODE
        (*out) << "(0x" << setw(2) << setfill('0') << right << hex << GetOpcode() << ")";
        #endif

        PrintExecUnit(out);
        PrintOperands(out);
    }

    void C62xDecodedInstruction :: PrintExecUnit (ostream *out) const
    {
        C62xExecutionUnit * execution_unit = GetExecutionUnit();
        if (execution_unit)
        {
            execution_unit->Print(out);
            if(IsCrossPath())
                (*out) << "X";
            else
                (*out) << " ";
        }
        (*out) << "    ";
    }

    void C62xDecodedInstruction :: PrintOperands (ostream *out) const
    {
        uint8_t operand_count = GetOperandCount();
        uint8_t current_opindex = 0;

        for(C62xOperandList_ConstIterator_t OLCI = m_oprands.begin(),
            OLCE = m_oprands.end(); OLCI != OLCE; ++OLCI)
        {
            (*OLCI)->Print(out);

            current_opindex++;
            if(current_opindex < operand_count)
                (*out) << ",";
        }

        if(GetDestRegister())
        {
            if(GetOperandCount()) (*out) << ",";
            GetDestRegister()->Print(out);
        }

        #ifdef PRINT_DELAY_SLOTS
        if(GetDelaySlots())   (*out) << "\t\t[+" << (uint32_t) GetDelaySlots() << "]";
        #endif

        (*out) << endl;
    }

    void C62xBranchInstr :: PrintOperands (ostream *out) const
    {
        C62xOperand * operand = GetOperand(0);

        if(operand->GetOperandType() != CONSTANT_OPERAND)
        {
            C62xDecodedInstruction :: PrintOperands(out);
            return;
        }

        // Specialized Operand Printing in case of Branch to Constant Addresses.
        C62xConstant       * cst_operand = (C62xConstant *) operand;
        DecodedInstruction * dec_instr   = operand->GetParentInstruction();
        Instruction        * instr       = dec_instr->GetParentInstruction();

        // Get the Reader Object Who Read this instruction.
        BinaryReader * binary_reader   = instr->GetBinaryReader();
        SymbolTable *  symbol_table    = binary_reader->GetSymbolTable();
        FetchPacket *  fetch_packet    = instr->GetParentFetchPacket();

        // The symbol table instance may be null; e.g. in case of Raw Binary Reader
        if(!symbol_table)
        {
            // Fallback to Default Operand Printing
            C62xDecodedInstruction :: PrintOperands(out);
            return;
        }

        char * symbol = symbol_table->GetSymbol((uint32_t) cst_operand->GetValue());

        if(symbol)
        {
            int32_t jump_offset = cst_operand->GetValue() - fetch_packet->GetInstrByIndex(0)->GetAddress();

            if(memcmp((void *) symbol, ".text:", 6) == 0) symbol += 6;

            (*out) << symbol << "(PC" << (jump_offset >= 0 ? "+" : "") << dec << jump_offset << " = 0x"
                   << hex << setw(8) << setfill('0') << cst_operand->GetValue() << ")";

            #ifdef PRINT_DELAY_SLOTS
            if(GetDelaySlots())   (*out) << "\t\t[+" << (uint32_t) GetDelaySlots() << "]";
            #endif

            (*out) << endl;
        }
        else
        {
            C62xDecodedInstruction :: PrintOperands(out);
            return;
        }
    }

    /* Generic Function Call Implementation for All Instructions */
    llvm::Value * C62xDecodedInstruction :: CreateLLVMFunctionCall(LLVMGenerator * llvm_gen, Module * out_mod, llvm::Value * result) const
    {
        string              func_name    = GetFunctionBaseName();
        llvm::IRBuilder<> & irbuilder    = llvm_gen->GetIRBuilder();
        llvm::Function    * func_ptr     = NULL;
        llvm::Value       * func_value   = NULL;
        C62xOperand       * curr_operand = NULL;
        Value * argument                 = NULL;
        std::vector<llvm::Value*> args;

        args.push_back(llvm_gen->p_proc_state);

        if(IsCondtional())
        {
            args.push_back(llvm_gen->Geti8Value(1));
            args.push_back(llvm_gen->Geti8Value(IsCRegZeroTest()));
            args.push_back(llvm_gen->Geti16Value(GetConditionRegister()->GetRegUID()));
        }
        else
        {
            args.push_back(llvm_gen->Geti8Value(0));
            args.push_back(llvm_gen->Geti8Value(0));
            args.push_back(llvm_gen->Geti16Value(0));
        }

        /* For Operand Registers And/Or Constants; If we have any */
        for (uint32_t index = 0; index < GetOperandCount(); index++)
        {
            curr_operand = GetOperand(index);
            func_name += curr_operand->GetTypeString();
            switch(curr_operand->GetOperandType())
            {
                case REGISTER_OPERAND:
                    args.push_back(llvm_gen->Geti16Value(((C62xRegister *) curr_operand)->GetRegUID()));
                    break;

                case MULTIREG_OPERAND:
                    args.push_back(llvm_gen->Geti16Value(((C62xMultiRegister *) curr_operand)->GetHighRegUID()));
                    args.push_back(llvm_gen->Geti16Value(((C62xMultiRegister *) curr_operand)->GetLowRegUID()));
                    break;

                case CONSTANT_OPERAND:
                    args.push_back(llvm_gen->Geti32Value(((C62xConstant *) curr_operand)->GetValue()));
                    break;
            }
        }

        /* For Destination Register; If we have one */
        curr_operand = GetDestRegister();
        if(curr_operand)
        {
            func_name += curr_operand->GetTypeString();

            if(curr_operand->GetOperandType() == REGISTER_OPERAND)
            {
                args.push_back(llvm_gen->Geti16Value(((C62xRegister *) curr_operand)->GetRegUID()));
            }
            else if (curr_operand->GetOperandType() == MULTIREG_OPERAND)
            {
                args.push_back(llvm_gen->Geti16Value(((C62xMultiRegister *) curr_operand)->GetHighRegUID()));
                args.push_back(llvm_gen->Geti16Value(((C62xMultiRegister *) curr_operand)->GetLowRegUID()));
            }
        }

        /* For Load/Store Mode */
        if(IsLoadStoreInstruction())
        {
            argument = llvm_gen->Geti8Value(((const C62xLDSTInstr *) this)->GetLoadStoreMode());
            args.push_back(argument);
        }

        /* For Delay Slots */
        argument = llvm_gen->Geti8Value(GetDelaySlots());
        args.push_back(argument);

        /* Push the Result Node Pointer; Which will be filled by ISA */
        ASSERT(result != NULL, "Result Node Pointer is NULL");
        args.push_back(result);

        func_ptr = out_mod->getFunction(StringRef(func_name));
        if(!func_ptr)
        {
          COUT << "Could not Get Function Call for: "  << func_name << endl;
          ASSERT(func_ptr != NULL, "Failed to Get Function Call");
        }

        INFO << "    Call to: " << func_name << "(...)" << endl;
        func_value = irbuilder.CreateCall(func_ptr, args.begin(), args.end(), "rval" + GetMnemonic());

        return (func_value);
    }
}
