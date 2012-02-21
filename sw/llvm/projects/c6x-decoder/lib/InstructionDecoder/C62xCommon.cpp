
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

#include "C62xCommon.h"
#include "SymbolTable.h"
#include "BinaryReader.h"
#include "DecodedInstruction.h"

namespace native
{
    string RegisterName[C62X_REG_BANKS][C62X_REGS_PER_BANK] =
    {
        {"A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "A10", "A11", "A12", "A13", "A14", "A15"},
        {"B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "B10", "B11", "B12", "B13", "B14", "B15"},
        {"AMR", "CSR", "ISR", "ICR", "IER", "ISTP", "IRP", "NRP", "C8", "C9", "C10", "C11", "C12", "C13", "C14", "PCE1"}
    };

    uint64_t BitsMask[40] = { 0x0000000000ULL, 0x0000000001ULL, 0x0000000003ULL, 0x0000000007ULL,
                              0x000000000FULL, 0x000000001FULL, 0x000000003FULL, 0x000000007FULL,
                              0x00000000FFULL, 0x00000001FFULL, 0x00000003FFULL, 0x00000007FFULL,
                              0x0000000FFFULL, 0x0000001FFFULL, 0x0000003FFFULL, 0x0000007FFFULL,
                              0x000000FFFFULL, 0x000001FFFFULL, 0x000003FFFFULL, 0x000007FFFFULL,
                              0x00000FFFFFULL, 0x00001FFFFFULL, 0x00003FFFFFULL, 0x00007FFFFFULL,
                              0x0000FFFFFFULL, 0x0001FFFFFFULL, 0x0003FFFFFFULL, 0x0007FFFFFFULL,
                              0x000FFFFFFFULL, 0x001FFFFFFFULL, 0x003FFFFFFFULL, 0x007FFFFFFFULL,
                              0x00FFFFFFFFULL, 0x01FFFFFFFFULL, 0x03FFFFFFFFULL, 0x07FFFFFFFFULL,
                              0x0FFFFFFFFFULL, 0x1FFFFFFFFFULL, 0x3FFFFFFFFFULL, 0x7FFFFFFFFFULL };

    void C62xRegister :: Print(ostream *out) const
    {
        (*out) << RegisterName[m_bank_id][m_reg_id];
    }

    void C62xMultiRegister :: Print(ostream *out) const
    {
        (*out) << RegisterName[m_bank_id][m_hreg_id] << ":" << RegisterName[m_bank_id][m_lreg_id];
    }

    void C62xConstant :: Print(ostream *out) const
    {
        DecodedInstruction * instruction = GetParentInstruction();
        C62xPrintMode_t mode = instruction->GetPreferredPrintMode();

        switch(mode)
        {
            case MODE_DEC:
                if(IsSigned())
                    (*out) << dec << (int64_t) m_value;
                else
                    (*out) << dec << (uint64_t) m_value;
            break;

            case MODE_HEX:
            {
                uint32_t nbits   = GetSizeBits();
                ASSERT(nbits <= 39, "Too Big Constant");

                uint64_t mask    = BitsMask[nbits];
                uint32_t nibbles = (nbits + 3) / 4;

                (*out) << "0x" << hex << setw(nibbles) << setfill('0') << (mask & m_value);
            }
            break;
        }
    }

    void Instruction :: Print (ostream *out) const
    {
        if(HasLabel())
        {
            (*out) << hex << setfill('0') << setw(8) << m_address;
            (*out) << setw(13) << setfill(' ') << "" << GetLabel() << ":" << endl;

            #ifdef PRINT_BRANCH_TARGET_HINTS
                (*out) << setw(8) << setfill(' ') << "";
            #endif
        }

        (*out) << hex << setfill('0') << setw(8) << right << m_address << "   ";
        (*out) << hex << setfill('0') << setw(8) << right << m_value;
    }
}
