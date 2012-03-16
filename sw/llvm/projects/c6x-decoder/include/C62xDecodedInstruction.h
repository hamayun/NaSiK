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

#ifndef C62X_DECODED_INSTRUCTION_H
#define C62X_DECODED_INSTRUCTION_H

#include "Common.h"
#include "C62xCommon.h"
#include "DecodedInstruction.h"

namespace native
{
    class C62xDecodedInstruction : public DecodedInstruction
    {
    private:
        bool                m_creg_zero_test;           // m_z; Test for Equality with Zero or Non-Zero
        bool                m_is_parallel;              // 0 = next instruction not executed in parallel; 1 = next instruction is executed in parallel
        bool                m_is_cross_path;            // m_x; Cross path for src2; 0 = do not use cross path, 1 = use cross path
        bool                m_dest_side_id;             // m_s; 0 = side A, 1 = side B
        uint16_t            m_opcode;                   // m_op
        C62xExecutionUnit  *p_unit;                     // m_unit
        C62xRegister       *p_condition_reg;            // m_creg
        uint8_t             m_operand_count;            // How many operands for this instruction ?
        uint8_t             m_delay_slots;              // How many delays slots for this instruction ?

        C62xOperand        *p_dst_reg;                  // Destination Register (Source Register in Case of Store Instructions)
        C62xOperandList_t   m_oprands;                  // List of Operands

    protected:
        virtual void PrintExecUnit (ostream *out) const;
        virtual void PrintOperands (ostream *out) const;

    public:
        C62xDecodedInstruction(Instruction * parent);
        C62xDecodedInstruction(C62xDecodedInstruction * dec_instr);

        virtual void SetCRegZeroTest(bool creg_zero_test) { m_creg_zero_test = creg_zero_test; }
        virtual bool IsCRegZeroTest() const { return (m_creg_zero_test); }

        virtual void SetParallel(bool is_parallel) { m_is_parallel = is_parallel; }
        virtual bool IsParallel() const { return (m_is_parallel); }

        virtual void SetCrossPath(bool is_cross_path) { m_is_cross_path = is_cross_path; }
        virtual bool IsCrossPath() const { return (m_is_cross_path); }

        virtual void SetDestinationSideId(bool dest_side_id) { m_dest_side_id = dest_side_id; }
        virtual bool GetDestinationSideId() const { return (m_dest_side_id); }

        virtual void SetOpcode(uint16_t opcode) { m_opcode = opcode; }
        virtual uint16_t GetOpcode() const { return (m_opcode); }

        virtual void SetExecutionUnit(C62xExecutionUnit * unit) { p_unit = unit; }
        virtual C62xExecutionUnit * GetExecutionUnit() const { return (p_unit); }

        virtual void SetConditionRegister(C62xRegister * condition_reg) { p_condition_reg = condition_reg; }
        virtual C62xRegister * GetConditionRegister() const { return (p_condition_reg); }

        virtual void SetOperandCount(uint8_t operand_count) { m_operand_count = operand_count; }
        virtual uint8_t GetOperandCount() const { return (m_operand_count); }

        virtual void SetDelaySlots(uint8_t delay_slots) { m_delay_slots = delay_slots; }
        virtual uint8_t GetDelaySlots() const { return (m_delay_slots); }

        virtual void SetDestRegister(C62xOperand * dst_reg) { p_dst_reg = dst_reg; }
        virtual C62xOperand * GetDestRegister() const { return (p_dst_reg); }

        virtual void AddOperand(C62xOperand * operand);
        virtual C62xOperand * GetOperand(uint32_t index) const;

        virtual void SetBranchConstAddress(uint32_t branch_cst_addr)
                { DecodedInstruction :: SetBranchConstAddress(branch_cst_addr); }
        virtual uint32_t GetBranchConstAddress() const
                { return (DecodedInstruction :: GetBranchConstAddress() & 0x7FFFFF ); }

        virtual void Print (ostream *out) const;

//        virtual llvm::Value * CreateLLVMFunctionCall(LLVMGenerator * llvm_gen, Module * out_mod,
//                                                     llvm::BasicBlock * update_exit_bb, llvm::Value * result) const;
        virtual llvm::Value * CreateLLVMFunctionCall(LLVMGenerator * llvm_gen, Module * out_mod, llvm::Value * result) const;

        virtual ~C62xDecodedInstruction() {}
    };

    class C62xDecodeHelper
    {
    private:
        uint8_t                 m_dest;
        uint8_t                 m_src1;
        uint8_t                 m_src2;
        int32_t                 m_sicst1;   /* Calculated from Source 1 */
        uint32_t                m_uicst1;   /* Calculated from Source 1 */
        int64_t                 m_slcst1;   /* Calculated from Source 1 */
        uint64_t                m_ulcst1;   /* Calculated from Source 1 */
        C62xRegisterBank_t      m_bank_id_dest;
        C62xRegisterBank_t      m_crossbank_id;
        uint32_t                m_uicst15;  /* Unsigned 15-bit Constant */
        uint8_t                 m_dunit_id; /* Referred as y in TMS320C62x Documents; 0 for .D1, 1 .D2 Unit */
        C62xLoadStoreMode_t     m_addr_mode;/* Load/Store Address Generator Mode */

    public:
        C62xDecodeHelper(C62xDecodedInstruction * dec_instr)
        {
            uint32_t instr  = dec_instr->GetParentInstruction()->GetValue();

            m_dest          = (instr >> 23) & 0x1f;
            m_src2          = (instr >> 18) & 0x1f;
            m_src1          = (instr >> 13) & 0x1f;
            m_sicst1        = ((int32_t) m_src1 << 27) >> 27; /* taking care of sign bit extn */
            m_uicst1        = (uint32_t) m_src1;
            m_slcst1        = ((int64_t) m_src1 << 59) >> 59; /* taking care of sign bit extn */
            m_ulcst1        = (uint64_t) m_src1;
            m_bank_id_dest  = (C62xRegisterBank_t) dec_instr->GetDestinationSideId();
            m_crossbank_id  = m_bank_id_dest;     // By default we assume its not a cross path access.

            if(dec_instr->IsCrossPath())
            {
                if(m_bank_id_dest == REG_BANK_A)
                    m_crossbank_id = REG_BANK_B;
                else
                    m_crossbank_id = REG_BANK_A;
            }

            m_uicst15       = instr >> 8 & 0x7fff;
            m_dunit_id      = instr >> 7 & 0x1;
            m_addr_mode     = (C62xLoadStoreMode_t) (instr >> 9 & 0xf);
        }

        virtual uint8_t  GetDest()        const { return (m_dest); }
        virtual uint8_t  GetSrc1()        const { return (m_src1); }
        virtual uint8_t  GetSrc2()        const { return (m_src2); }
        virtual int32_t  GetSIntConst1()  const { return (m_sicst1); }
        virtual uint32_t GetUIntConst1()  const { return (m_uicst1); }
        virtual int64_t  GetSLongConst1() const { return (m_slcst1); }
        virtual uint64_t GetULongConst1() const { return (m_ulcst1); }
        virtual C62xRegisterBank_t GetDestBankId() const { return (m_bank_id_dest); }
        virtual C62xRegisterBank_t GetCrossBankId() const { return (m_crossbank_id); }
        virtual uint32_t GetUIntConst15() const { return (m_uicst15); }
        virtual uint8_t  GetDUnitId()     const { return (m_dunit_id); }
        virtual C62xLoadStoreMode_t GetAddressingMode() const { return (m_addr_mode); }

        virtual ~C62xDecodeHelper() {}
    };

    class C62xABSInstr : public C62xDecodedInstruction
    {
    public:
        C62xABSInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg, C62xOperand * src2_opr) :
            C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ABS");
            SetDestRegister(dst_reg); AddOperand(src2_opr);
        }

        // IDEA: We can use the 'm_opcode' to get an 'appropriate' function call from the LLVM Library.
        virtual ~C62xABSInstr() {}
    };

    class C62xADDInstr : public C62xDecodedInstruction
    {
    public:
        C62xADDInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ADD");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xADDInstr() {}
    };

    class C62xMVInstr : public C62xDecodedInstruction
    {
    public:
        C62xMVInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MV");
            SetDestRegister(dst_reg); AddOperand(src2_opr);
        }

        virtual ~C62xMVInstr() {}
    };

    class C62xADDKInstr : public C62xDecodedInstruction
    {
    public:
        C62xADDKInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg, C62xOperand * src1_opr) :
                      C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ADDK");
            SetDestRegister(dst_reg); AddOperand(src1_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xADDKInstr() {}
    };

    class C62xADD2Instr : public C62xDecodedInstruction
    {
    public:
        C62xADD2Instr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ADD2");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xADD2Instr() {}
    };

    class C62xSUBInstr : public C62xDecodedInstruction
    {
    public:
        C62xSUBInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SUB");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xSUBInstr() {}
    };

    class C62xNEGInstr : public C62xDecodedInstruction
    {
    public:
        C62xNEGInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("NEG");
            SetDestRegister(dst_reg); AddOperand(src2_opr);
        }

        virtual ~C62xNEGInstr() {}
    };

    class C62xSUBCInstr : public C62xDecodedInstruction
    {
    public:
        C62xSUBCInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SUBC");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSUBCInstr() {}
    };

    class C62xSUBUInstr : public C62xDecodedInstruction
    {
    public:
        C62xSUBUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SUBU");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSUBUInstr() {}
    };

    class C62xSUB2Instr : public C62xDecodedInstruction
    {
    public:
        C62xSUB2Instr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SUB2");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSUB2Instr() {}
    };

    class C62xSUBABInstr : public C62xDecodedInstruction
    {
    public:
        C62xSUBABInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SUBAB");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSUBABInstr() {}
    };

    class C62xSUBAHInstr : public C62xDecodedInstruction
    {
    public:
        C62xSUBAHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SUBAH");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSUBAHInstr() {}
    };

    class C62xSUBAWInstr : public C62xDecodedInstruction
    {
    public:
        C62xSUBAWInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SUBAW");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSUBAWInstr() {}
    };

    class C62xADDUInstr : public C62xDecodedInstruction
    {
    public:
        C62xADDUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ADDU");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xADDUInstr() {}
    };

    class C62xADDABInstr : public C62xDecodedInstruction
    {
    public:
        C62xADDABInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ADDAB");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xADDABInstr() {}
    };

    class C62xADDAHInstr : public C62xDecodedInstruction
    {
    public:
        C62xADDAHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ADDAH");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xADDAHInstr() {}
    };

    class C62xADDAWInstr : public C62xDecodedInstruction
    {
    public:
        C62xADDAWInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ADDAW");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xADDAWInstr() {}
    };

    class C62xANDInstr : public C62xDecodedInstruction
    {
    public:
        C62xANDInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("AND");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xANDInstr() {}
    };

    class C62xORInstr : public C62xDecodedInstruction
    {
    public:
        C62xORInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("OR");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xORInstr() {}
    };

    class C62xXORInstr : public C62xDecodedInstruction
    {
    public:
        C62xXORInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("XOR");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xXORInstr() {}
    };

    class C62xNOTInstr : public C62xDecodedInstruction
    {
    public:
        C62xNOTInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("NOT");
            SetDestRegister(dst_reg); AddOperand(src2_opr);
        }

        virtual ~C62xNOTInstr() {}
    };

    class C62xSADDInstr : public C62xDecodedInstruction
    {
    public:
        C62xSADDInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SADD");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSADDInstr() {}
    };

    class C62xSATInstr : public C62xDecodedInstruction
    {
    public:
        C62xSATInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SAT");
            SetDestRegister(dst_reg); AddOperand(src1_opr);
        }

        virtual ~C62xSATInstr() {}
    };

    class C62xSSUBInstr : public C62xDecodedInstruction
    {
    public:
        C62xSSUBInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SSUB");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSSUBInstr() {}
    };

    class C62xSHLInstr : public C62xDecodedInstruction
    {
    public:
        C62xSHLInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SHL");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSHLInstr() {}
    };

    class C62xSSHLInstr : public C62xDecodedInstruction
    {
    public:
        C62xSSHLInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SSHL");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSSHLInstr() {}
    };

    class C62xSHRInstr : public C62xDecodedInstruction
    {
    public:
        C62xSHRInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SHR");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSHRInstr() {}
    };

    class C62xSHRUInstr : public C62xDecodedInstruction
    {
    public:
        C62xSHRUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SHRU");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSHRUInstr() {}
    };

    class C62xSETInstr : public C62xDecodedInstruction
    {
    public:
        C62xSETInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                    C62xOperand * src1_opr, C62xOperand * src2_opr, C62xOperand * src3_opr = NULL) :
                    C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SET");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr); AddOperand(src3_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xSETInstr() {}
    };

    class C62xCLRInstr : public C62xDecodedInstruction
    {
    public:
        C62xCLRInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xOperand * src3_opr = NULL) :
                     C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("CLR");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr); AddOperand(src3_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xCLRInstr() {}
    };

    class C62xEXTInstr : public C62xDecodedInstruction
    {
    public:
        C62xEXTInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xOperand * src3_opr = NULL) :
                     C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("EXT");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr); AddOperand(src3_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xEXTInstr() {}
    };

    class C62xEXTUInstr : public C62xDecodedInstruction
    {
    public:
        C62xEXTUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xOperand * src3_opr = NULL) :
                     C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("EXTU");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr); AddOperand(src3_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xEXTUInstr() {}
    };

    class C62xCMPEQInstr : public C62xDecodedInstruction
    {
    public:
        C62xCMPEQInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("CMPEQ");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xCMPEQInstr() {}
    };

    class C62xCMPGTInstr : public C62xDecodedInstruction
    {
    public:
        C62xCMPGTInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("CMPGT");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xCMPGTInstr() {}
    };

    class C62xCMPGTUInstr : public C62xDecodedInstruction
    {
    public:
        C62xCMPGTUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("CMPGTU");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xCMPGTUInstr() {}
    };

    class C62xCMPLTInstr : public C62xDecodedInstruction
    {
    public:
        C62xCMPLTInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("CMPLT");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xCMPLTInstr() {}
    };

    class C62xCMPLTUInstr : public C62xDecodedInstruction
    {
    public:
        C62xCMPLTUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("CMPLTU");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xCMPLTUInstr() {}
    };

    class C62xLMBDInstr : public C62xDecodedInstruction
    {
    public:
        C62xLMBDInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("LMBD");
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xLMBDInstr() {}
    };

    class C62xNORMInstr : public C62xDecodedInstruction
    {
    public:
        C62xNORMInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("NORM");
            SetDestRegister(dst_reg); AddOperand(src1_opr);
        }

        virtual ~C62xNORMInstr() {}
    };

    class C62xMVKInstr : public C62xDecodedInstruction
    {
    public:
        C62xMVKInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg, C62xOperand * src1_opr) :
                     C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MVK");
            SetDestRegister(dst_reg); AddOperand(src1_opr);
        }

        virtual ~C62xMVKInstr() {}
    };

    class C62xMVKHInstr : public C62xDecodedInstruction
    {
    public:
        C62xMVKHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg, C62xOperand * src1_opr) :
                      C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MVKH");
            SetDestRegister(dst_reg); AddOperand(src1_opr);
        }

        virtual ~C62xMVKHInstr() {}
    };

    class C62xMPYInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPY"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYInstr() {}
    };

    class C62xMPYUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYUInstr() {}
    };

    class C62xMPYUSInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYUSInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYUS"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYUSInstr() {}
    };

    class C62xMPYSUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYSUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYSU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYSUInstr() {}
    };

    class C62xMPYHInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYH"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYHInstr() {}
    };

    class C62xMPYHUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYHU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYHUInstr() {}
    };

    class C62xMPYHUSInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHUSInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                        C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYHUS"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xMPYHUSInstr() {}
    };

    class C62xMPYHSUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHSUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                        C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYHSU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYHSUInstr() {}
    };

    class C62xMPYHLInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHLInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYHL"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYHLInstr() {}
    };

    class C62xMPYHLUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHLUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYHLU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYHLUInstr() {}
    };

    class C62xMPYHULSInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHULSInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYHULS"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xMPYHULSInstr() {}
    };

    class C62xMPYHSLUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYHSLUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYHSLU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xMPYHSLUInstr() {}
    };

    class C62xMPYLHInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYLHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYLH"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYLHInstr() {}
    };

    class C62xMPYLHUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYLHUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                        C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYLHU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xMPYLHUInstr() {}
    };

    class C62xMPYLUHSInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYLUHSInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                        C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYLUHS"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xMPYLUHSInstr() {}
    };

    class C62xMPYLSHUInstr : public C62xDecodedInstruction
    {
    public:
        C62xMPYLSHUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                        C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MPYLSHU"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xMPYLSHUInstr() {}
    };

    class C62xMVCInstr : public C62xDecodedInstruction
    {
    public:
        C62xMVCInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg, C62xOperand * src2_opr) :
                     C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("MVC");
            SetDestRegister(dst_reg); AddOperand(src2_opr);
        }

        virtual ~C62xMVCInstr() {}
    };

    class C62xSMPYInstr : public C62xDecodedInstruction
    {
    public:
        C62xSMPYInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SMPY"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual ~C62xSMPYInstr() {}
    };

    class C62xSMPYHLInstr : public C62xDecodedInstruction
    {
    public:
        C62xSMPYHLInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SMPYHL"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xSMPYHLInstr() {}
    };

    class C62xSMPYLHInstr : public C62xDecodedInstruction
    {
    public:
        C62xSMPYLHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                        C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SMPYLH"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xSMPYLHInstr() {}
    };

    class C62xSMPYHInstr : public C62xDecodedInstruction
    {
    public:
        C62xSMPYHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                       C62xOperand * src1_opr, C62xOperand * src2_opr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("SMPYH"); SetDelaySlots((uint8_t) C62X_MULTIPLY_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
            SetPreferredPrintMode(MODE_DEC);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xSMPYHInstr() {}
    };

    class C62xLDSTInstr : public C62xDecodedInstruction
    {
    private:
        C62xLoadStoreMode_t         m_ldst_mode;
        C62xLoadStoreType_t         m_type;         /* Load = 0, Store = 1*/
        bool                        m_requires_left_shift;
        uint8_t                     m_left_shift_bits;

    protected:
        virtual C62xOperand * GetBaseRegister() const
        {
            return (GetOperand(1));
        }

        virtual C62xOperand * GetOffsetOperand() const
        {
            return (GetOperand(0));
        }

        void PrintExecUnit (ostream *out) const
        {
            GetExecutionUnit()->Print(out);
            (*out) << "T" << GetDestinationSideId() + 1 << "   ";
        }

        virtual void PrintOperands (ostream *out) const
        {
            C62xOperand * base_reg    = GetBaseRegister();
            C62xOperand * offset_opr  = GetOffsetOperand();
            C62xOperand * source_reg  = GetSourceRegister();
            C62xLoadStoreType_t type  = GetLoadStoreType();

            ASSERT(base_reg != NULL, "Base Register Null !!!");
            ASSERT(offset_opr != NULL, "Offset Operand Null !!!");
            ASSERT(source_reg != NULL, "Source Register Null !!!");

            // If its a store instruction, print the source register here.
            if(type == STORE_INSTR)
            {
                source_reg->Print(out);
                (*out) << ",";
            }

            switch(GetLoadStoreMode())
            {
                case CST_NEGATIVE_OFFSET:       // *-R[ucst5]
                    (*out) << "*-"; base_reg->Print(out);
                    break;

                case CST_POSITIVE_OFFSET:       // *+R[ucst5]
                    (*out) << "*+"; base_reg->Print(out);
                    break;

                case REG_NEGATIVE_OFFSET:       // *-R[offsetR]
                    (*out) << "*-"; base_reg->Print(out);
                    break;

                case REG_POSITIVE_OFFSET:       // *+R[offsetR]
                    (*out) << "*+"; base_reg->Print(out);
                    break;

                case CST_OFFSET_PRE_DECR:       // *--R[ucst5]
                    (*out) << "*--"; base_reg->Print(out);
                    break;

                case CST_OFFSET_PRE_INCR:       // *++R[ucst5]
                    (*out) << "*++"; base_reg->Print(out);
                    break;

                case CST_OFFSET_POST_DECR:      // *R--[ucst5]
                    (*out) << "*"; base_reg->Print(out); (*out) << "--";
                    break;

                case CST_OFFSET_POST_INCR:      // *R++[ucst5]
                    (*out) << "*"; base_reg->Print(out); (*out) << "++";
                    break;

                case REG_OFFSET_PRE_DECR:       // *--R[offsetR]
                    (*out) << "*--"; base_reg->Print(out);
                    break;

                case REG_OFFSET_PRE_INCR:       // *++R[offsetR]
                    (*out) << "*++"; base_reg->Print(out);
                    break;

                case REG_OFFSET_POST_DECR:      // *R--[offsetR]
                    (*out) << "*"; base_reg->Print(out); (*out) << "--";
                    break;

                case REG_OFFSET_POST_INCR:      // *R++[offsetR]
                    (*out) << "*"; base_reg->Print(out); (*out) << "++";
                    break;
            }

            (*out) << "["; offset_opr->Print(out); (*out) << "]";

            // If its a store instruction, print the source register here.
            if(type == LOAD_INSTR)
            {
                (*out) << ",";
                source_reg->Print(out);
            }

            #ifdef PRINT_DELAY_SLOTS
            if(GetDelaySlots())   (*out) << "\t\t[+" << (uint32_t) GetDelaySlots() << "]";
            #endif

            (*out) << endl;
        }

    public:
        C62xLDSTInstr(C62xDecodedInstruction *dec_instr, C62xLoadStoreMode_t mode, C62xLoadStoreType_t type) :
            C62xDecodedInstruction(dec_instr), m_ldst_mode (mode), m_type (type), m_requires_left_shift(false), m_left_shift_bits(0)
        {
            SetLoadStoreInstruction(true);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual void SetLoadStoreMode(C62xLoadStoreMode_t ldst_mode) { m_ldst_mode = ldst_mode; }
        virtual C62xLoadStoreMode_t GetLoadStoreMode() const { return (m_ldst_mode); }

        virtual void SetLoadStoreType(C62xLoadStoreType_t type) { m_type = type; }
        virtual C62xLoadStoreType_t GetLoadStoreType() const { return (m_type); }

        virtual void SetRequiresLeftShift(bool requires_left_shift) { m_requires_left_shift = requires_left_shift; }
        virtual bool GetRequiresLeftShift() const { return (m_requires_left_shift); }

        virtual void SetLeftShiftBits(uint8_t left_shift_bits) { m_left_shift_bits = left_shift_bits; }
        virtual uint8_t GetLeftShiftBits() const { return (m_left_shift_bits); }

        // This method operates on the destination register; Just provides a renamed interface.
        virtual void SetSourceRegister(C62xOperand * src_reg) { SetDestRegister(src_reg); }
        virtual C62xOperand * GetSourceRegister() const { return (GetDestRegister()); }

        virtual ~C62xLDSTInstr() {}
    };

    class C62xLDBInstr : public C62xLDSTInstr
    {
    public:
        C62xLDBInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                     C62xLDSTInstr(dec_instr, mode, LOAD_INSTR)
        {
            SetMnemonic("LDB"); SetDelaySlots((uint8_t) C62X_LOAD_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xLDBInstr() {}
    };

    class C62xLDBUInstr : public C62xLDSTInstr
    {
    public:
        C62xLDBUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                      C62xLDSTInstr(dec_instr, mode, LOAD_INSTR)
        {
            SetMnemonic("LDBU"); SetDelaySlots((uint8_t) C62X_LOAD_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xLDBUInstr() {}
    };

    class C62xLDHInstr : public C62xLDSTInstr
    {
    public:
        C62xLDHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                     C62xLDSTInstr(dec_instr, mode, LOAD_INSTR)
        {
            SetMnemonic("LDH"); SetDelaySlots((uint8_t) C62X_LOAD_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xLDHInstr() {}
    };

    class C62xLDHUInstr : public C62xLDSTInstr
    {
    public:
        C62xLDHUInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                      C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                      C62xLDSTInstr(dec_instr, mode, LOAD_INSTR)
        {
            SetMnemonic("LDHU"); SetDelaySlots((uint8_t) C62X_LOAD_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xLDHUInstr() {}
    };

    class C62xLDWInstr : public C62xLDSTInstr
    {
    public:
        C62xLDWInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                     C62xLDSTInstr(dec_instr, mode, LOAD_INSTR)
        {
            SetMnemonic("LDW"); SetDelaySlots((uint8_t) C62X_LOAD_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xLDWInstr() {}
    };

    class C62xSTBInstr : public C62xLDSTInstr
    {
    public:
        C62xSTBInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                     C62xLDSTInstr(dec_instr, mode, STORE_INSTR)
        {
            SetMnemonic("STB"); SetDelaySlots((uint8_t) C62X_STORE_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSTBInstr() {}
    };

    class C62xSTHInstr : public C62xLDSTInstr
    {
    public:
        C62xSTHInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                     C62xLDSTInstr(dec_instr, mode, STORE_INSTR)
        {
            SetMnemonic("STH"); SetDelaySlots((uint8_t) C62X_STORE_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSTHInstr() {}
    };

    class C62xSTWInstr : public C62xLDSTInstr
    {
    public:
        C62xSTWInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg,
                     C62xOperand * src1_opr, C62xOperand * src2_opr, C62xLoadStoreMode_t mode) :
                     C62xLDSTInstr(dec_instr, mode, STORE_INSTR)
        {
            SetMnemonic("STW"); SetDelaySlots((uint8_t) C62X_STORE_DELAY);
            SetDestRegister(dst_reg); AddOperand(src1_opr); AddOperand(src2_opr);
        }

        virtual ~C62xSTWInstr() {}
    };

    class C62xBranchInstr : public C62xDecodedInstruction
    {
    public:
        C62xBranchInstr(C62xDecodedInstruction *dec_instr, C62xOperand * src2_opr) :
            C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("B"); SetDelaySlots((uint8_t) C62X_BRANCH_DELAY); AddOperand(src2_opr);
            SetBranchInstruction(true);

            // Save the Branch Constant Address.
            if(src2_opr->GetOperandType() == CONSTANT_OPERAND)
            {
                uint32_t branch_cst_address = (uint32_t) ((C62xConstant *)src2_opr)->GetValue();
                SetBranchConstAddress(branch_cst_address);
                SetRegisterBranch(false);
            }
            else
            {
                SetRegisterBranch(true);
            }
        }

        virtual void PrintOperands (ostream *out) const;

        virtual ~C62xBranchInstr() {}
    };

    class C62xBranchIRPInstr : public C62xDecodedInstruction
    {
    public:
        C62xBranchIRPInstr(C62xDecodedInstruction *dec_instr, C62xOperand * src2_opr) :
            C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("B"); SetDelaySlots((uint8_t) C62X_BRANCH_DELAY); AddOperand(src2_opr);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
            SetBranchInstruction(true); SetRegisterBranch(true);
        }

        virtual ~C62xBranchIRPInstr() {}
    };

    class C62xBranchNRPInstr : public C62xDecodedInstruction
    {
    public:
        C62xBranchNRPInstr(C62xDecodedInstruction *dec_instr, C62xOperand * src2_opr) :
            C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("B"); SetDelaySlots((uint8_t) C62X_BRANCH_DELAY); AddOperand(src2_opr);
            SetBranchInstruction(true); SetRegisterBranch(true);
            WARN << "Non-Tested Instruction Instance (" << GetMnemonic() << ")" << endl;
        }

        virtual ~C62xBranchNRPInstr() {}
    };

    class C62xIDLEInstr : public C62xDecodedInstruction
    {
    public:
        C62xIDLEInstr(C62xDecodedInstruction *dec_instr) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("IDLE");
        }

        virtual ~C62xIDLEInstr() {}
    };

    class C62xNOPInstr : public C62xDecodedInstruction
    {
    public:
        C62xNOPInstr(C62xDecodedInstruction *dec_instr, C62xOperand * src1_opr = NULL) : C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("NOP"); AddOperand(src1_opr);
            SetNOPInstruction(true);
            SetPreferredPrintMode(MODE_DEC);
        }

        virtual uint8_t GetNOPCount() const
        {
            uint8_t nop_count = 0;

            if(GetOperandCount())
            {
                nop_count = (uint8_t)((C62xConstant *) GetOperand(0))->GetValue();
            }

            return (nop_count);
        }

        virtual ~C62xNOPInstr() {}
    };

    class C62xZEROInstr : public C62xDecodedInstruction
    {
    public:
        C62xZEROInstr(C62xDecodedInstruction *dec_instr, C62xOperand * dst_reg) :
                      C62xDecodedInstruction(dec_instr)
        {
            SetMnemonic("ZERO"); SetDestRegister(dst_reg);
        }

        virtual ~C62xZEROInstr() {}
    };

}
#endif // C62X_DECODED_INSTRUCTION_H
