
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

#ifndef C62X_COMMON_H
#define	C62X_COMMON_H

#include "Common.h"

#ifdef __cplusplus
namespace native
{
#endif
    #define FETCH_PACKET_SIZE       8
    #define C62X_REG_BANKS          3
    #define C62X_REGS_PER_BANK      16

    // define condition register codes
    #define CREG_B0                 0x01
    #define CREG_B1                 0x02
    #define CREG_B2                 0x03
    #define CREG_A1                 0x04
    #define CREG_A2                 0x05

    // define different execution unit types
    #define IDLEUNIT                0x00
    #define LUNIT                   0x06
    #define SUNIT                   0x08
    #define SUNIT_MVK               0x0a
    #define SUNIT_ADDK              0x14
    #define SUNIT_IMMED             0x02
    #define SUNIT_BCOND             0x04
    #define MUNIT                   0x00
    #define DUNIT                   0x10
    #define DUNIT_LDSTOFFSET        0x03
    #define DUNIT_LDSTBASEROFFSET   0x01

    //#define NOP1                  0x00000000
    //#define NOP1_PARALLEL         0x00000001

    #define IDLEOP                  0x78
    #define IDLEINST                0x7800
    #define NOP                     0x000

    // Delay Slots
    #define C62X_STORE_DELAY        0x0
    #define C62X_SINGLE_CYCLE       0x0
    #define C62X_MULTIPLY_DELAY     0x1
    #define C62X_LOAD_DELAY         0x4
    #define C62X_BRANCH_DELAY       0x5
    #define C62X_MAX_DELAY_SLOTS    C62X_BRANCH_DELAY

    // serve to extract information from instructions
    #define getUnit_2bit(inst)      (inst >> 2 & 0x03)
    #define getUnit_3bit(inst)      (inst >> 2 & 0x07)
    #define getUnit_4bit(inst)      (inst >> 2 & 0x0f)
    #define getUnit_5bit(inst)      (inst >> 2 & 0x1f)
    #define getUnit_11bit(inst)     (inst >> 2 & 0x07ff)
    #define getUnit_16bit(inst)     (inst >> 2 & 0xffff)

    // Operand Types
    typedef enum C62xOperandType
    {
        REGISTER_OPERAND = 0,
        MULTIREG_OPERAND = 1,
        CONSTANT_OPERAND = 2
    } C62xOperandType_t;

    // Register Banks
    typedef enum C62xRegisterBank
    {
        REG_BANK_A = 0,         // Side A
        REG_BANK_B = 1,         // Side B
        REG_BANK_C = 2          // Control Registers Bank
    } C62xRegisterBank_t;

    typedef enum C62xControlRegister
    {
        REG_AMR  = 0,
        REG_CSR  = 1,
        REG_ISR  = 2,  /* REG_IFR  = 2 */
        REG_ICR  = 3,
        REG_IER  = 4,
        REG_ISTP = 5,
        REG_IRP  = 6,
        REG_NRP  = 7,
        REG_PCE1 = 15 /* Should Be: REG_PCE1 = 16 */
    } C62xControlRegister_t;

    // Operand Types
    typedef enum C62xExecutionUnitType
    {
        U_EXUNIT       = 0,     /* Unknown Unit */
        I_EXUNIT       = 1,     /* Idle Unit    */
        L_EXUNIT       = 2,
        S_EXUNIT       = 3,
        M_EXUNIT       = 4,
        D_EXUNIT       = 5
    } C62xExecutionUnitType_t;

    typedef enum C62xLoadStoreMode
    {
        CST_NEGATIVE_OFFSET  = 0x0,         // *-R[ucst5]
        CST_POSITIVE_OFFSET  = 0x1,         // *+R[ucst5]
        REG_NEGATIVE_OFFSET  = 0x4,         // *-R[offsetR]
        REG_POSITIVE_OFFSET  = 0x5,         // *+R[offsetR]
        CST_OFFSET_PRE_DECR  = 0x8,         // *--R[ucst5]
        CST_OFFSET_PRE_INCR  = 0x9,         // *++R[ucst5]
        CST_OFFSET_POST_DECR = 0xA,         // *R--[ucst5]
        CST_OFFSET_POST_INCR = 0xB,         // *R++[ucst5]
        REG_OFFSET_PRE_DECR  = 0xC,         // *--R[offsetR]
        REG_OFFSET_PRE_INCR  = 0xD,         // *++R[offsetR]
        REG_OFFSET_POST_DECR = 0xE,         // *R--[offsetR]
        REG_OFFSET_POST_INCR = 0xF          // *R++[offsetR]
    } C62xLoadStoreMode_t;

    typedef enum C62xLoadStoreType
    {
        LOAD_INSTR  = 0x0,
        STORE_INSTR = 0x1
    } C62xLoadStoreType_t;

    typedef enum C62xPrintMode
    {
        MODE_DEC  = 0x0,
        MODE_HEX  = 0x1
    } C62xPrintMode_t;

#ifdef __cplusplus

    class DecodedInstruction;
    class C62xOperand
    {
    private:
        bool                    m_is_signed;           // Use Signed = 1 or Unsigned = 0 value
        uint8_t                 m_size_bits;           // Operand Size in bits i.e. 5, 16, 32, 40, 64
        C62xOperandType_t       m_operand_type;        // Register, Constants ?
        DecodedInstruction     *p_parent_instr;        // Instruction for which this operand instance has been created

    public:
        C62xOperand(bool is_signed, uint8_t size_bits) :
            m_is_signed (is_signed), m_size_bits (size_bits), m_operand_type (REGISTER_OPERAND) {}

        C62xOperand(bool is_signed, uint8_t size_bits, C62xOperandType_t operand_type) :
            m_is_signed (is_signed), m_size_bits (size_bits), m_operand_type (operand_type) {}

        virtual void SetSigned(bool is_signed) { m_is_signed = is_signed; }
        virtual bool IsSigned() const { return (m_is_signed); }

        virtual void SetSizeBits(uint8_t size_bits) { m_size_bits = size_bits; }
        virtual uint8_t GetSizeBits() const { return (m_size_bits); }

        virtual void SetOperandType(C62xOperandType_t operand_type) { m_operand_type = operand_type; }
        virtual C62xOperandType_t GetOperandType() const { return (m_operand_type); }

        virtual void SetParentInstruction(DecodedInstruction * parent_instr) { p_parent_instr = parent_instr; }
        virtual DecodedInstruction * GetParentInstruction() const { return (p_parent_instr); }

        virtual string GetTypeString() const
        {
            string type_string = "_";
            std::stringstream size_string;

            if(IsSigned())
                type_string += "S";
            else
                type_string += "U";

            type_string += GetSubTypeString();
            size_string << (uint32_t) GetSizeBits();
            type_string += size_string.str();

            return (type_string);
        }

        virtual string GetSubTypeString() const = 0;
        virtual void Print(ostream *out) const { DOUT << "Not Implemented !!!"; }

        virtual ~C62xOperand() {}
    };

    typedef list<C62xOperand *>                     C62xOperandList_t;
    typedef list<C62xOperand *> :: iterator         C62xOperandList_Iterator_t;
    typedef list<C62xOperand *> :: const_iterator   C62xOperandList_ConstIterator_t;

    class C62xRegister : public C62xOperand
    {
    private:
        C62xRegisterBank_t m_bank_id;      // 0 = Bank A, 1 = Bank B, 2 = Bank C (Control Registers)
        uint8_t            m_reg_id;
        uint16_t           m_reg_uid;      // Registers Unique ID

    public:
        C62xRegister() : C62xOperand(true, 32, REGISTER_OPERAND), m_bank_id(REG_BANK_A), m_reg_id(0)
        {
            m_reg_uid = ((uint16_t) m_bank_id * C62X_REGS_PER_BANK) + m_reg_id;
        }

        C62xRegister(bool is_signed, uint8_t size_bits, C62xRegisterBank_t bank_id, uint8_t reg_id) :
            C62xOperand(is_signed, size_bits, REGISTER_OPERAND), m_bank_id (bank_id), m_reg_id (reg_id)
        {
            m_reg_uid = ((uint16_t) m_bank_id * C62X_REGS_PER_BANK) + m_reg_id;
        }

        virtual void SetBankId(C62xRegisterBank_t bank_id) { m_bank_id = bank_id; }
        virtual C62xRegisterBank_t GetBankId() const { return (m_bank_id); }

        virtual void SetRegId(uint8_t reg_id) { m_reg_id = reg_id; }
        virtual uint8_t GetRegId() const { return (m_reg_id); }

        virtual uint16_t GetRegUID() const { return (m_reg_uid); }

        virtual string GetSubTypeString() const { return ("R"); }
        virtual void Print(ostream *out) const;

        virtual ~C62xRegister() {}
    };

    class C62xMultiRegister : public C62xOperand
    {
    private:
        C62xRegisterBank_t m_bank_id;      // 0 = Bank A, 1 = Bank B, 2 = Bank C (Control Registers)
        uint8_t            m_hreg_id;      // High Word Register ID
        uint8_t            m_lreg_id;      // Low Word Register ID
        uint16_t           m_hreg_uid;     // High Word Registers Unique ID
        uint16_t           m_lreg_uid;     // Low Word Registers Unique ID

    public:
        C62xMultiRegister() : C62xOperand(true, 40, MULTIREG_OPERAND), m_bank_id(REG_BANK_A), m_hreg_id(0), m_lreg_id(0)
        {
            m_hreg_uid = ((uint16_t) m_bank_id * C62X_REGS_PER_BANK) + m_hreg_id;
            m_lreg_uid = ((uint16_t) m_bank_id * C62X_REGS_PER_BANK) + m_lreg_id;
        }

        C62xMultiRegister(bool is_signed, uint8_t size_bits, C62xRegisterBank_t bank_id, uint8_t hreg_id, uint8_t lreg_id) :
            C62xOperand(is_signed, size_bits, MULTIREG_OPERAND), m_bank_id (bank_id), m_hreg_id (hreg_id), m_lreg_id (lreg_id)
        {
            m_hreg_uid = ((uint16_t) m_bank_id * C62X_REGS_PER_BANK) + m_hreg_id;
            m_lreg_uid = ((uint16_t) m_bank_id * C62X_REGS_PER_BANK) + m_lreg_id;
        }

        virtual void SetBankId(C62xRegisterBank_t bank_id) { m_bank_id = bank_id; }
        virtual C62xRegisterBank_t GetBankId() const { return (m_bank_id); }

        virtual void SetHighRegId(uint8_t hreg_id) { m_hreg_id = hreg_id; }
        virtual uint8_t GetHighRegId() const { return (m_hreg_id); }

        virtual void SetLowRegId(uint8_t lreg_id) { m_lreg_id = lreg_id; }
        virtual uint8_t GetLowRegId() const { return (m_lreg_id); }

        virtual uint16_t GetHighRegUID() const { return (m_hreg_uid); }
        virtual uint16_t GetLowRegUID() const { return (m_lreg_uid); }

        virtual string GetSubTypeString() const { return ("R"); }
        virtual void Print(ostream *out) const;

        virtual ~C62xMultiRegister() {}
    };

    class C62xConstant : public C62xOperand
    {
    private:
        uint64_t        m_value;               // Value of the Constant

    public:
        C62xConstant() : C62xOperand(true, 32, CONSTANT_OPERAND), m_value(0) {}

        C62xConstant(bool is_signed, uint8_t size_bits, uint64_t value) :
            C62xOperand(is_signed, size_bits, CONSTANT_OPERAND), m_value (value) {}

        virtual void SetValue(uint64_t value) { m_value = value; }
        virtual uint64_t GetValue() const { return (m_value); }

        virtual string GetSubTypeString() const { return ("C"); }
        virtual void Print(ostream *out) const;

        virtual ~C62xConstant() {}
    };

    class C62xExecutionUnit
    {
    private:
        C62xExecutionUnitType_t m_unit_type;        // Which Unit to Execute on? S, L, D, M, I, U
        uint8_t                 m_unit_subtype;     // Specific Type (Original One found in Instruction); SUNIT, SUNIT_MVK ...
        uint8_t                 m_unit_id;          // Unit ID 0, 1

    public:
        C62xExecutionUnit() :
            m_unit_type(U_EXUNIT), m_unit_subtype(IDLEUNIT), m_unit_id(0) {}

        C62xExecutionUnit(C62xExecutionUnitType_t unit_type, uint8_t unit_subtype, uint8_t unit_id) :
            m_unit_type(unit_type), m_unit_subtype(unit_subtype), m_unit_id(unit_id) {}

        virtual void SetUnitType(C62xExecutionUnitType_t unit_type) { m_unit_type = unit_type; }
        virtual C62xExecutionUnitType_t GetUnitType() const { return (m_unit_type); }

        virtual void SetUnitSubType(uint8_t unit_subtype) { m_unit_subtype = unit_subtype; }
        virtual uint8_t GetUnitSubType() const { return (m_unit_subtype); }

        virtual void SetUnitId(uint8_t unit_id) { m_unit_id = unit_id; }
        virtual uint8_t GetUnitId() const { return (m_unit_id); }

        virtual void Print (ostream *out) const
        {
            (*out) << ".";
            switch(GetUnitType())
            {
                case L_EXUNIT:
                    (*out) << "L" << (uint32_t) GetUnitId();
                    break;
                case S_EXUNIT:
                    (*out) << "S" << (uint32_t) GetUnitId();
                    break;
                case M_EXUNIT:
                    (*out) << "M" << (uint32_t) GetUnitId();
                    break;
                case D_EXUNIT:
                    (*out) << "D" << (uint32_t) GetUnitId();
                    break;
                case U_EXUNIT:
                case I_EXUNIT:
                    break;
            }
        }

        virtual ~C62xExecutionUnit() {}
    };
}
#endif
#endif	/* C62X_COMMON_H */

