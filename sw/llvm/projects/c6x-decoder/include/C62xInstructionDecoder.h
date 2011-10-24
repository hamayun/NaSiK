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

#ifndef C62X_INSTRUCTION_DECODER_H
#define C62X_INSTRUCTION_DECODER_H

#include "C62xCommon.h"
#include "InstructionDecoder.h"
#include "C62xDecodedInstruction.h"

namespace native
{
    class C62xInstructionDecoder : public InstructionDecoder
    {
    private:
        // Some Private Helper Methods
        C62xRegister * GetConditionRegister(uint32_t instr);
        C62xExecutionUnit * GetExecutionUnit(uint32_t instr);
        uint16_t GetOpcode(uint32_t instr);
        bool GetCrossPathAccessFlag(uint32_t instr);

        DecodedInstruction * DecInstrByExecUnit(C62xDecodedInstruction * dec_instr);

        DecodedInstruction * DecInstrByUnitL(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);

        DecodedInstruction * DecInstrByUnitS(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);
        DecodedInstruction * DecInstrByUnitS_MVK(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);
        DecodedInstruction * DecInstrByUnitS_ADDK(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);
        DecodedInstruction * DecInstrByUnitS_IMMED(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);
        DecodedInstruction * DecInstrByUnitS_BCOND(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);

        DecodedInstruction * DecInstrByUnitM(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);

        DecodedInstruction * DecInstrByUnitD(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);
        DecodedInstruction * DecInstrByUnitD_LDSTOFFSET(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);
        DecodedInstruction * DecInstrByUnitD_LDSTBASEROFFSET(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);

        DecodedInstruction * DecInstrByUnitI(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh);

    public:
        C62xInstructionDecoder();

        virtual DecodedInstruction * DecodeInstruction(Instruction * instruction);

        virtual ~C62xInstructionDecoder() {}
    };
}
#endif // C62X_INSTRUCTION_DECODER_H
