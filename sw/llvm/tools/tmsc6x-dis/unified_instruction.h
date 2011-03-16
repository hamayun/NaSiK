/*************************************************************************************
 * File   : unified_instruction.h,
 *
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

#ifndef _UNIFIED_INSTRUCTION_H
#define	_UNIFIED_INSTRUCTION_H

#include <iostream>
#include <iomanip>
#include <string>

#define NUM_OPCODES             10
#define MAX_NUM_OPERANDS        4

#define OPTYPE_SREG             0
#define OPTYPE_UREG             1
#define OPTYPE_SCST             2
#define OPTYPE_UCST             3

#define REG_BANK_SIZE           16

class unified_instruction
{
private:
    int                 m_address;
    int                 m_condition_reg;
    unsigned char       m_condition;
    char                m_opcode;
    char                m_nm_operands;
    char                m_optype[MAX_NUM_OPERANDS];
    unsigned int        m_opsize[MAX_NUM_OPERANDS];
    unsigned int        m_operand[MAX_NUM_OPERANDS];
    char                m_mode;
    bool                m_is_conditional;
    bool                m_is_parallel;
    
public:

    unified_instruction(){
        m_is_conditional = false;
        m_condition = 0xFF;             // Zero or One are Valid; By Default 0xFF means Invalid Condition
    }

    ~unified_instruction(){}

    void set_address(int address)
    {
        m_address = address; 
    }

    void set_condition_reg(int condition_reg)
    {
        m_condition_reg = condition_reg;
    }

    void set_condition(char condition)
    {
        m_condition = condition;
    }

    void set_opcode(char opcode)
    {
        m_opcode = opcode;
    }
    
    void set_nm_operands(char nm_operands)
    {
        m_nm_operands = nm_operands;
    }
    
    void set_optype(int index, char optype, char opsize)
    {
        m_optype[index] = optype;
        m_opsize[index] = opsize;
    }

    void set_operand(int index, unsigned int operand)
    {
        m_operand[index] = operand;
    }

    void set_mode(char mode)
    {
        m_mode = mode;
    }

    void set_conditional(bool is_conditional)
    {
        m_is_conditional = is_conditional;
    }

    void set_parallel()
    {
        m_is_parallel = true;
    }

    void print_operand(ostream * out, char optype, unsigned int opsize, unsigned int operand)
    {
        switch(optype)
        {
            case OPTYPE_SREG:
            case OPTYPE_UREG:
                if(opsize == 32)
                    (*out) << "R" << dec << operand;
                else if (opsize == 64)
                    (*out) << "R" << dec << operand+1 << ":" << "R" << operand;
                break;

            case OPTYPE_SCST:
            case OPTYPE_UCST:
            {
                /*
                if(opsize < 32)
                {
                    int mask = 0x80000000;
                    mask = ~(mask >> (31 - opsize));
                    (*out) << FMT_INT << (mask & operand);
                }
                else
                */
                (*out) << FMT_INT << operand;
            }
                break;
        }
    }

    void print(ostream *out)
    {
        // This is Temporary Declaration; Will Move it some common place
        string str_opcodes[NUM_OPCODES] = {
            "NOP", "ADD", "SUB", "MUL", "LDB", "LDW", "CMPEQ", "AND", "OR", "B"
        };

        (*out) << hex << setfill('0') << setw(8) << m_address << "   ";

        if(m_is_conditional)
            (*out) << dec << "[" << (m_condition ? "!":" ") << "R" << m_condition_reg << "]  ";
        else
            (*out) << "        ";

        (*out) << str_opcodes[(int)m_opcode] << "\t\t";

        for(int i=0; i<m_nm_operands; i++)
        {
            print_operand(out, m_optype[i], m_opsize[i], m_operand[i]);

            if(i < (m_nm_operands-1))
                (*out) << ", "; 
        }

        (*out) << endl; 
    }

    static int get_opcode_index(string opcode)
    {
        // This is Temporary Declaration; Will Move it some common place
        string str_opcodes[NUM_OPCODES] = {
            "NOP", "ADD", "SUB", "MUL", "LDB", "LDW", "CMPEQ", "AND", "OR", "B"
        };

        for(int i=0; i<NUM_OPCODES; i++)
        {
            if(str_opcodes[i] == opcode)
                return (i);
        }
        return(-1); 
    }
};

typedef unified_instruction ir_instr_t; 

#endif	/* _UNIFIED_INSTRUCTION_H */

