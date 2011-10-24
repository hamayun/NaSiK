
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

#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <list>

#include "BinaryReader.h"
#include "SymbolTable.h"

using namespace std;

namespace native
{
    class FetchPacket;
    class ExecutePacket;
    class DecodedInstruction;
    class BinaryReader;

    class Instruction
    {
    protected:
        uint32_t                m_address;
        uint32_t                m_value;
        bool                    m_has_label;
        string                  m_label;

        FetchPacket            *p_parent_fetch_packet;
        ExecutePacket          *p_parent_execute_packet;
        DecodedInstruction     *p_decoded_instr;
        BinaryReader           *p_binary_reader;        // Handle to Binary Reader who read this Instruction

        Instruction(){};

        virtual void CheckLabel()
        {
            SymbolTable *  symbol_table    = p_binary_reader->GetSymbolTable();
            // The symbol table instance may be null; e.g. in case of Raw Binary Reader
            if(symbol_table)
            {
                char * symbol = symbol_table->GetRawSymbol((uint32_t) m_address);
                if(symbol)
                {
                    m_has_label = true;
                    m_label = symbol;
                }
            }
        }

    public:
        Instruction(uint32_t address, int32_t value, BinaryReader * binary_reader) :
            m_address (address), m_value (value), m_has_label(false), m_label(""),
            p_decoded_instr (NULL), p_binary_reader(binary_reader)
        {
            CheckLabel();
        }

        Instruction(Instruction * instruction) :
            m_address (instruction->m_address), m_value (instruction->m_value), m_has_label(false), m_label(""),
            p_decoded_instr (NULL), p_binary_reader(instruction->p_binary_reader)
        {
            CheckLabel();
        }

        virtual uint32_t GetAddress() const { return (m_address); }
        virtual uint32_t GetValue() const { return (m_value); }

        virtual bool HasLabel() const { return (m_has_label); }
        virtual string GetLabel() const { return (m_label); }

        virtual void SetParentFetchPacket(FetchPacket * parent_fetch_packet) { p_parent_fetch_packet = parent_fetch_packet; }
        virtual FetchPacket * GetParentFetchPacket() const { return (p_parent_fetch_packet); }

        virtual void SetParentExecutePacket(ExecutePacket * parent_execute_packet) { p_parent_execute_packet = parent_execute_packet; }
        virtual ExecutePacket * GetParentExecutePacket() const { return (p_parent_execute_packet); }

        virtual void SetDecodedInstruction(DecodedInstruction * decoded_instr) { p_decoded_instr = decoded_instr; }
        virtual DecodedInstruction * GetDecodedInstruction() { return(p_decoded_instr); }

        virtual void SetBinaryReader(BinaryReader * binary_reader) { p_binary_reader = binary_reader; }
        virtual BinaryReader * GetBinaryReader() const { return(p_binary_reader); }

        virtual void Print (ostream *out) const;

        virtual ~Instruction() {}
    };
}
#endif // INSTRUCTION_H
