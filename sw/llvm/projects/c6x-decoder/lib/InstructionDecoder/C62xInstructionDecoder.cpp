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

#include "FetchPacket.h"
#include "ExecutePacket.h"
#include "C62xInstructionDecoder.h"
#include "C62xDecodedInstruction.h"

namespace native
{
    C62xInstructionDecoder :: C62xInstructionDecoder()
    {}

    DecodedInstruction * C62xInstructionDecoder :: DecodeInstruction(Instruction * instruction)
    {
        C62xDecodedInstruction * dec_instr = new C62xDecodedInstruction(instruction); // Temporary Object for Instruction Decoding
        uint32_t instr = instruction->GetValue();

        dec_instr->SetCondtional((instr >> 29) & 0x07);
        dec_instr->SetCRegZeroTest((instr >> 28) & 0x01);
        dec_instr->SetConditionRegister(GetConditionRegister(instr));
        dec_instr->SetParallel(instr & 0x1);
        dec_instr->SetExecutionUnit(GetExecutionUnit(instr));
        dec_instr->SetOpcode(GetOpcode(instr));
        dec_instr->SetCrossPath(GetCrossPathAccessFlag(instr));
        dec_instr->SetDestinationSideId((instr >> 1) & 0x01);
        dec_instr->GetExecutionUnit()->SetUnitId(dec_instr->GetDestinationSideId() + 1);

        return (DecInstrByExecUnit(dec_instr));
    }

    // Private Helper Methods
    DecodedInstruction * C62xInstructionDecoder :: DecInstrByExecUnit(C62xDecodedInstruction * dec_instr)
    {
        DecodedInstruction * dec_instr_eu = NULL;
        static DecodedInstruction * prev_dec_instr = NULL;
        C62xExecutionUnit  * exec_unit    = dec_instr->GetExecutionUnit();
        C62xDecodeHelper     helper(dec_instr);

        switch(exec_unit->GetUnitSubType())
        {
            case LUNIT:
                dec_instr_eu = DecInstrByUnitL(dec_instr, &helper);
                break;

            case SUNIT:
                dec_instr_eu = DecInstrByUnitS(dec_instr, &helper);
                break;

            case SUNIT_MVK:
                dec_instr_eu = DecInstrByUnitS_MVK(dec_instr, &helper);
                break;

            case SUNIT_ADDK:
                dec_instr_eu = DecInstrByUnitS_ADDK(dec_instr, &helper);
                break;

            case SUNIT_IMMED:
                dec_instr_eu = DecInstrByUnitS_IMMED(dec_instr, &helper);
                break;

            case SUNIT_BCOND:
                dec_instr_eu = DecInstrByUnitS_BCOND(dec_instr, &helper);
                break;

            case MUNIT:
                if(dec_instr->GetOpcode() == 0x00 || dec_instr->GetOpcode() == 0x78)
                    dec_instr_eu = DecInstrByUnitI(dec_instr, &helper);
                else
                    dec_instr_eu = DecInstrByUnitM(dec_instr, &helper);
                break;

            case DUNIT:
                dec_instr_eu = DecInstrByUnitD(dec_instr, &helper);
                break;

            case DUNIT_LDSTOFFSET:
                dec_instr_eu = DecInstrByUnitD_LDSTOFFSET(dec_instr, &helper);
                break;

            case DUNIT_LDSTBASEROFFSET:
                dec_instr_eu = DecInstrByUnitD_LDSTBASEROFFSET(dec_instr, &helper);
                break;

            default:
                dec_instr_eu = NULL;
                break;
        }

        delete dec_instr;
        dec_instr = NULL;

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");

        // Chain this instruction to the previous decoded instruction.
        if(prev_dec_instr)
        {
            prev_dec_instr->SetNextInstruction(dec_instr_eu);
            dec_instr_eu->SetPrevInstruction(prev_dec_instr);
        }

        // For next time; when we decode another instruction.
        prev_dec_instr = dec_instr_eu;
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitL(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;

        switch(dec_instr->GetOpcode())
        {
            case 0x1a :
            {
                /*ABS xsint, sint*/
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xABSInstr(dec_instr, dest_reg, src2_opr);
            }
            break;

            case 0x38 :
            {
                /*ABS slong, slong*/
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xABSInstr(dec_instr, dest_reg, src2_opr);
            }
            break;

            case 0x03 :
            {
                /*ADD sint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x23 :
            {
                /*ADD sint, xsint, slong*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x2b :
            {
                /*ADDU usint, xusint, uslong*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xADDUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x21 :
            {
                /*ADD xsint, slong, slong*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x29 :
            {
                /*ADDU xusint, uslong, uslong*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xADDUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x02 :
            {
                if(dh->GetSIntConst1() == 0x0)
                {
                    /*MV xsint, sint*/
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xMVInstr(dec_instr, dest_reg, src2_opr);
                }
                else if(dh->GetSIntConst1() < 0)
                {
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * src2_opr = new C62xConstant(true, 5, dh->GetSIntConst1()*(-1)); // Make it +ve
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                    dec_instr_eu->SetPreferredPrintMode(MODE_HEX);
                }
                else
                {
                    /*ADD scst5, xsint, sint*/
                    C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                    dec_instr_eu->SetPreferredPrintMode(MODE_DEC);
                }
            }
            break;

            case 0x20 :
            {
                if(dh->GetSIntConst1() == -1)
                {
                    C62xOperand * src1_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                    C62xOperand * src2_opr = new C62xConstant(true, 5, 0x1);
                    C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                    dec_instr_eu->SetPreferredPrintMode(MODE_HEX);
                }
                else
                {
                    /*ADD scst5, slong, slong*/
                    C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                    C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                    dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            case 0x7b :
            {
                /*AND usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xANDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x7a :
            {
                /*AND scst5, xusint, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xANDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x53 :
            {
                /*CMPEQ sint, xsint, usint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPEQInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x52 :
            {
                /*CMPEQ scst5, xsint, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPEQInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x51 :
            {
                /*CMPEQ xsint, slong, usint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPEQInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x50 :
            {
                /*CMPEQ scst5, slong, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPEQInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x47 :
            {
                /*CMPGT sint, xsint, usint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x46 :
            {
                /*CMPGT scst5, xsint, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x45 :
            {
                /*CMPGT xsint, slong, usint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x44 :
            {
                /*CMPGT scst5, slong, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x4f :
            {
                /*CMPGTU usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x4e :
            {
                /*CMPGTU ucst4, xusint, usint*/
                C62xOperand * src1_opr = new C62xConstant(false, 4, dh->GetUIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x4d :
            {
                /*CMPGTU xusint, uslong, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x4c :
            {
                /*CMPGTU ucst4, uslong, usint*/
                C62xOperand * src1_opr = new C62xConstant(false, 4, dh->GetUIntConst1());
                C62xOperand * src2_opr = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPGTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x57 :
            {
                /*CMPLT sint, xsint, usint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x56 :
            {
                /*CMPLT scst5, xsint, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x55 :
            {
                /*CMPLT xsint, slong, usint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x54 :
            {
                /*CMPLT scst5, slong, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x5f :
            {
                /*CMPLTU usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x5e :
            {
                /*CMPLTU ucst5, xusint, usint*/
                C62xOperand * src1_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x5d :
            {
                /*CMPLTU xusint, uslong, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x5c :
            {
                /*CMPLTU ucst5, uslong, usint*/
                C62xOperand * src1_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * src2_opr = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCMPLTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x6b :
            {
                /*LMBD - Left Most Bit Detection usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xLMBDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x6a :
            {
                /*LMBD - Left Most Bit Detection ucst5, xusint, usint*/
                C62xOperand * src1_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xLMBDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x63 :
            {
                /*NORM Normalize Integer, # of redundant sign bits are found xsint, usint*/
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xNORMInstr(dec_instr, dest_reg, src2_opr);
            }
            break;

            case 0x60 :
            {
                /*NORM Normalize Integer, # of redundant sign bits are found slong, usint*/
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xNORMInstr(dec_instr, dest_reg, src2_opr);
            }
            break;

            case 0x7f :
            {
                /*Bitwise OR usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x7e :
            {
                /*Bitwise OR scst5, xusint, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x13 :
            {
                /*SADD Integer addition with saturation to result size sint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x31 :
            {
                /*SADD Integer addition with saturation to result size xsint, slong, slong*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x12 :
            {
                /*SADD Integer addition with saturation to result size scst5, xsint, sint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x30 :
            {
                /*SADD Integer addition with saturation to result size scst5, slong, slong*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x40 :
            {
                if(dh->GetSrc1() == 0x00)
                {
                    /*SAT saturate a 40bit integer to a 32bit integer slong, sint*/
                    C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xSATInstr(dec_instr, dest_reg, src2_opr);
                }
                else
                {
                    DOUT << "Error: Invalid Instruction.src1 = 0x" << hex << dh->GetSrc1() << endl;
                }
            }
            break;

            case 0x0f :
            {
                /*SSUB Integer addition with saturation to result size sint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1f :
            {
                /*SSUB Integer addition with saturation to result size xsint, sint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0e :
            {
                /*SSUB Integer addition with saturation to result size scst5, xsint, sint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x2c :
            {
                /*SSUB Integer addition with saturation to result size scst5, slong, slong*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x07 :
            {
                if((dh->GetSrc1() == dh->GetSrc2()) && (dh->GetDestBankId() == dh->GetCrossBankId()))
                {
                    /*ZERO without saturation sint*/
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xZEROInstr(dec_instr, dest_reg);
                }
                else
                {
                    /*SUB without saturation sint, xsint, sint*/
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            case 0x17 :
            {
                /*SUB without saturation xsint, sint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x27 :
            {
                if((dh->GetSrc1() == dh->GetSrc2()) && (dh->GetDestBankId() == dh->GetCrossBankId()))
                {
                    /*ZERO without saturation slong*/
                    C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                    dec_instr_eu = new C62xZEROInstr(dec_instr, dest_reg);
                }
                else
                {
                    /*SUB without saturation sint, xsint, slong*/
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            case 0x37 :
            {
                /*SUB without saturation xsint, sint, slong*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x2f :
            {
                /*SUBU without saturation usint, xusint, uslong*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSUBUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x3f :
            {
                /*SUBU without saturation xusint, usint, uslong*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSUBUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x06 :
            {
                if(dh->GetSIntConst1() == 0x0)
                {
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xNEGInstr(dec_instr, dest_reg, src2_opr);
                }
                else
                {
                    /*SUB without saturation scst5, xsint, sint*/
                    C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            case 0x24 :
            {
                if(dh->GetSIntConst1() == 0x0)
                {
                    /*NEG without saturation slong, slong*/
                    C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                    dec_instr_eu = new C62xNEGInstr(dec_instr, dest_reg, src2_opr);
                }
                else
                {
                    /*SUB without saturation scst5, slong, slong*/
                    C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                    C62xOperand * src2_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            case 0x4b :
            {
                /*SUBC Conditional Integer Subtract and Shift - used for division usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBCInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x6f :
            {
                /*XOR usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xXORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x6e :
            {
                if(dh->GetSIntConst1() == -1)
                {
                    /*NOT xusint, usint*/
                    C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xNOTInstr(dec_instr, dest_reg, src2_opr);
                }
                else
                {
                    /*XOR scst5, xusint, usint*/
                    C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                    C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xXORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            default:
            {
                DOUT << "Error: Unknown Instruction for LUNIT" << endl;
            }
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitS(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;

        switch(dec_instr->GetOpcode())
        {
            case 0x07 :
            {
                /*ADD sint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x06 :
            {
                if(dh->GetSIntConst1() == 0x0)
                {
                    /*MV xsint, sint*/
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xMVInstr(dec_instr, dest_reg, src2_opr);
                }
                else if(dh->GetSIntConst1() == -1)
                {
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * src2_opr = new C62xConstant(true, 5, 0x1);
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                    dec_instr_eu->SetPreferredPrintMode(MODE_HEX);
                }
                else
                {
                    /*ADD scst5, xsint, sint*/
                    C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                    dec_instr_eu->SetPreferredPrintMode(MODE_DEC);
                }
            }
            break;

            case 0x01 :
            {
                /*ADD2 sint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADD2Instr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1f :
            {
                /*AND usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xANDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1e :
            {
                /*AND scst5, xusint, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xANDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0d :
            {
                /*Branch using a Register, xusint*/
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                dec_instr_eu = new C62xBranchInstr(dec_instr, src2_opr);
            }
            break;

            case 0x03 :
            {
                if(dh->GetSrc2() == 0x06)
                {
                    /*Branch Using an IRP - Interrupt Return Pointer*/
                    C62xOperand * src2_opr = new C62xRegister(false, 32, REG_BANK_C, (uint8_t) REG_IRP);
                    dec_instr_eu = new C62xBranchIRPInstr(dec_instr, src2_opr);
                }
                else if(dh->GetSrc2() == 0x07)
                {
                    /*Branch using NMI Return Pointer & set NMIE in crf[IER]*/
                    C62xOperand * src2_opr = new C62xRegister(false, 32, REG_BANK_C, (uint8_t) REG_NRP);
                    dec_instr_eu = new C62xBranchNRPInstr(dec_instr, src2_opr);
                }
                else
                {
                    DOUT << "Error: Unknown Instruction" << endl;
                }
            }
            break;

            case 0x3f :
            {
                /*CLR Clear Bit Fields of m_src2 whose bounds are given by 5-9 & 0-4 of m_src1*/
                /*usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCLRInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x2f :
            {
                /*EXT Extract & Sign Extend a bit field*/
                /*usint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xEXTInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x2b :
            {
                /*EXTU Extract & Zero Extend a bit field*/
                /*usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xEXTUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0f :
            {
                /*MVC Move between Control File & Register File usint, usint*/// Control File --> Register File
                C62xOperand * src2_opr = new C62xRegister(false, 32, REG_BANK_C, dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMVCInstr(dec_instr, dest_reg, src2_opr);
            }
            break;

            case 0x0e :
            {
                /*MVC Move between Control File & Register File xusint, usint*/// Register File --> Control File
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, REG_BANK_C, dh->GetDest());
                dec_instr_eu = new C62xMVCInstr(dec_instr, dest_reg, src2_opr);
            }
            break;

            case 0x1b :
            {
                /*Bitwise OR usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1a :
            {
                /*Bitwise OR scst5, xusint, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x3b :
            {
                /*SET Set Bit Fields of m_src2 whose bounds are given by 5-9 & 0-4 of m_src1*/
                /*usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSETInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x33 :
            {
                /*SHL Shift Left by amount given in m_src1 usint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x31 :
            {
                /*SHL Shift Left by amount given in m_src1 usint, slong, slong*/
                C62xOperand * src1_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x13 :
            {
                /*SHL Shift Left by amount given in m_src1 usint, xusint, uslong*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x32 :
            {
                /*SHL Shift Left by amount given in m_src1 ucst5, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x30 :
            {
                /*SHL Shift Left by amount given in m_src1 ucst5, slong, slong*/
                C62xOperand * src1_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x12 :
            {
                /*SHL Shift Left by amount given in m_src1 ucst5, xusint, uslong*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x37 :
            {
                /*SHR Shift Right  by amount given in m_src1 usint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSHRInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x35 :
            {
                /*SHR Shift Right by amount given in m_src1 usint, slong, slong*/
                C62xOperand * src1_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHRInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x36 :
            {
                /*SHR Shift Right by amount given in m_src1 ucst5, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSHRInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x34 :
            {
                /*SHR Shift Right by amount given in m_src1 ucst5, slong, slong*/
                C62xOperand * src1_opr = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xMultiRegister(true, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHRInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x27 :
            {
                /*SHRU Logical Shift Right by amount given in m_src1 usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSHRUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x25 :
            {
                /*SHRU Logical Shift Right by amount given in m_src1 usint, uslong, uslong*/
                C62xOperand * src1_opr = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHRUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x26 :
            {
                /*SHRU Logical Shift Right by amount given in m_src1 ucst5, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSHRUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x24 :
            {
                /*SHRU Logical Shift Right by amount given in m_src1 ucst5, uslong, uslong*/
                C62xOperand * src1_opr = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetSrc2() + 1, dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xMultiRegister(false, 40, dh->GetDestBankId(), dh->GetDest() + 1, dh->GetDest());
                dec_instr_eu = new C62xSHRUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x23 :
            {
                /*SSHL Shift Left with Saturation usint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x22 :
            {
                /*SSHL Shift Left with Saturation ucst5, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSSHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x17 :
            {
                if((dh->GetSrc1() == dh->GetSrc2()) && (dh->GetDestBankId() == dh->GetCrossBankId()))
                {
                    /*ZERO without saturation sint*/
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xZEROInstr(dec_instr, dest_reg);
                }
                else
                {
                    /*SUB without saturation sint, xsint, sint*/
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            case 0x16 :
            {
                /*SUB without saturation scst5, xsint, sint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x11 :
            {
                /*SUB2 Subtractions of lower and upper halfs sint, xsint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUB2Instr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0b :
            {
                /*XOR usint, xusint, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xXORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0a :
            {
                if(dh->GetSIntConst1() == -1)
                {
                    /*NOT xusint, usint*/
                    C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xNOTInstr(dec_instr, dest_reg, src2_opr);
                }
                else
                {
                    /*XOR scst5, xusint, usint*/
                    C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                    C62xOperand * src2_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xXORInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction for SUNIT" << endl;
            break;

        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitS_MVK(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;
        uint32_t instr = dec_instr->GetParentInstruction()->GetValue();

        uint8_t  mvk_type = (instr >> 6 & 0x01);
        uint32_t uscst16  = (((instr >> 7) & 0x0000ffff) << 16);
        int32_t  scst16   = ((((int32_t) instr >> 7) & 0x0000ffff) << 16) >> 16;

        switch(dec_instr->GetOpcode())
        {
            case 0x00:
                if(mvk_type == 0x00){
                    /*MVK Move a 16bit signed constant into a Register & Sign Extend scst16, sint*/
                    // Constant length is 16 but is printed as 32 signed hex integer
                    C62xOperand * src1_opr = new C62xConstant(true, 16, scst16);
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xMVKInstr(dec_instr, dest_reg, src1_opr);
                }
                else if (mvk_type == 0x01){
                    /*MVKH Move 16bit constant into the Upper Bits of a Register uscst16, sint*/
                    // Constant length is 16 but is encoded in the 16 MSBs
                    C62xOperand * src1_opr = new C62xConstant(false, 16, uscst16);
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xMVKHInstr(dec_instr, dest_reg, src1_opr);
                }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitS_ADDK(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;
        uint32_t instr = dec_instr->GetParentInstruction()->GetValue();
        int16_t  scst16   = (((instr >> 7) & 0x0000ffff) << 16) >> 16;

        switch(dec_instr->GetOpcode())
        {
            case 0x00 :
            {
                /*ADDK cst16, usint*/
                C62xOperand * src1_opr = new C62xConstant(true, 16, scst16);
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDKInstr(dec_instr, dest_reg, src1_opr);
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitS_IMMED(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;
        uint32_t instr = dec_instr->GetParentInstruction()->GetValue();

        int8_t csta = ((instr >> 13) & 0x1f);
        int8_t cstb = ((instr >> 8) & 0x1f);

        switch(dec_instr->GetOpcode())
        {
            case 0x00 :
            {
                /*EXTU Extract & Zero Extend a bit field*/
                /*ucst5, ucst5, usint, usint*/ /* src2, csta, cstb, dst */
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, csta);
                C62xOperand * src3_opr = new C62xConstant(false, 5, cstb);
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xEXTUInstr(dec_instr, dest_reg, src1_opr, src2_opr, src3_opr);
            }
            break;

            case 0x01 :
            {
                /*EXT Extract & Sign Extend a bit field*/
                /*ucst5, ucst5, sint, sint*/ /* src2, csta, cstb, dst */
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, csta);
                C62xOperand * src3_opr = new C62xConstant(false, 5, cstb);
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xEXTInstr(dec_instr, dest_reg, src1_opr, src2_opr, src3_opr);
            }
            break;

            case 0x02 :
            {
                /*SET Set Bit Fields of m_src2 whose bounds are given by csta & cstb*/
                /*ucst5, ucst5, usint, usint*/ /* src2, csta, cstb, dst */
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, csta);
                C62xOperand * src3_opr = new C62xConstant(false, 5, cstb);
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSETInstr(dec_instr, dest_reg, src1_opr, src2_opr, src3_opr);
            }
            break;

            case 0x03 :
            {
                /*CLR Clear Bit Fields of m_src2 whose bounds are given by csta & cstb*/
                /*ucst5, ucst5, usint, usint*/ /* src2, csta, cstb, dst */
                C62xOperand * src1_opr = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, csta);
                C62xOperand * src3_opr = new C62xConstant(false, 5, cstb);
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xCLRInstr(dec_instr, dest_reg, src1_opr, src2_opr, src3_opr);
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitS_BCOND(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;
        Instruction * raw_instr           = dec_instr->GetParentInstruction();
        FetchPacket * fetch_packet        = raw_instr->GetParentFetchPacket();
        uint32_t      instr               = raw_instr->GetValue();
        uint32_t      packet_address      = NULL;

        // TODO: Enable this Assertion
        //ASSERT(fetch_packet != NULL, "Fetch Packet Not Found, Please make sure that Fetch Packet List has been created");
        //packet_address = fetch_packet->GetInstrByIndex(0)->GetAddress();
        if(fetch_packet)
        {
            packet_address = fetch_packet->GetInstrByIndex(0)->GetAddress();
        }

        int32_t scst21 = (((instr >> 7) & 0x001fffff) << 11) >> 11;
        scst21         = (scst21 << 2) + packet_address;
        scst21         = scst21 & 0x7fffff;              // Discard bits higher than 23rd bit.

        switch(dec_instr->GetOpcode())
        {
            case 0x00:
            {
                /*Branch Using a Displacement; {scst21}*/
                C62xOperand * src1_opr = new C62xConstant(true, 21, scst21);
                dec_instr_eu = new C62xBranchInstr(dec_instr, src1_opr);
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitM(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;

        switch(dec_instr->GetOpcode())
        {
            case 0x19 :
            {
                /*MPY Integer Multiply 16lsb x 16lsb; slsb16, xslsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x18 :
            {
                /*MPY Integer Multiply 16lsb x 16lsb; scst5, xslsb16, sint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x01 :
            {
                /*MPYH Integer Multiply 16msb x 16msb; smsb16, xsmsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x09 :
            {
                /*MPYHL Integer Multiply 16msb x 16lsb; smsb16, xslsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0f :
            {
                /*MPYHLU Integer Multiply 16msb x 16lsb; umsb16, xulsb16, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHLUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0b :
            {
                /*MPYHSLU Integer Multiply 16msb x 16lsb; smsb16, xulsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHSLUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x03 :
            {
                /*MPYHSU Integer Multiply 16msb x 16msb; smsb16, xumsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHSUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x07 :
            {
                /*MPYHU Integer Multiply 16msb x 16msb; umsb16, xumsb16, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0d :
            {
                /*MPYHULS Integer Multiply 16msb x 16lsb; umsb16, xslsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHULSInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x05 :
            {
                /*MPYHUS Integer Multiply 16msb x 16msb; umsb16, xsmsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYHUSInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x11 :
            {
                /*MPYLH Integer Multiply 16lsb x 16msb; slsb16, xsmsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYLHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x17 :
            {
                /*MPYLHU Integer Multiply 16lsb x 16msb; ulsb16, xumsb16, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYLHUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x13 :     /* Consulted Manual to Correct Opcode Here; Old Value 0x11*/
            {
                /*MPYLSHU Integer Multiply 16lsb x 16msb slsb16, xumsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYLSHUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x15 :
            {
                /*MPYLUHS Integer Multiply 16lsb x 16msb ulsb16, xsmsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYLUHSInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1b :
            {
                /*MPYSU Integer Multiply 16lsb x 16lsb; slsb16, xulsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYSUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1e :
            {
                /*MPYSU Integer Multiply 16lsb x 16lsb; scst5, xulsb16, sint*/
                C62xOperand * src1_opr = new C62xConstant(true, 5, dh->GetSIntConst1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYSUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1f :
            {
                /*MPYU Integer Multiply 16lsb x 16lsb; ulsb16, xulsb16, usint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(false, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYUInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1d :
            {
                /*MPYUS Integer Multiply 16lsb x 16lsb; ulsb16, xslsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(false, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xMPYUSInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x1a :
            {
                /*SMPY slsb16, xslsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSMPYInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x02 :
            {
                /*SMPYH smsb16, xsmsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSMPYHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x0a :
            {
                /*SMPYHL smsb16, xslsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSMPYHLInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x12 :
            {
                /*SMPYLH slsb16, xsmsb16, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 16, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * src2_opr = new C62xRegister(true, 16, dh->GetCrossBankId(), dh->GetSrc2());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSMPYLHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction for MUNIT" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitD(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;

        switch(dec_instr->GetOpcode())
        {
            case 0x10 :
            {
                /* ADD sint, sint, sint*//* Source 1 and 2 interchanged !!! */
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x12 :
            {
                if(dh->GetUIntConst1() == 0x0)
                {
                    /* MV sint, sint*/
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xMVInstr(dec_instr, dest_reg, src1_opr);
                }
                else
                {
                    /* ADD ucst5, sint, sint*/
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                    C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xADDInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                    dec_instr_eu->SetPreferredPrintMode(MODE_HEX);
                }
            }
            break;

            case 0x30 :
            {
                /*ADDAB Add Byte with Addressing sint, sint, sint;  {+baseR[offsetR], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDABInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x34 :
            {
                /*ADDAH Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDAHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x38 :
            {
                /*ADDAW Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDAWInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x32 :
            {
                /*ADDAB Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDAHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x36 :
            {
                /*ADDAH Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDAHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x3a :
            {
                /*ADDAW Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xADDAWInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x11 :
            {
                if((dh->GetSrc1() == dh->GetSrc2()) && (dh->GetDestBankId() == dh->GetCrossBankId()))
                {
                    /*ZERO without saturation sint*/
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xZEROInstr(dec_instr, dest_reg);
                }
                else
                {
                    /*SUB without saturation sint, sint, sint*/
                    C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                    C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                    C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                    dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                }
            }
            break;

            case 0x13 :
            {
                /*SUB without saturation ucst5, sint, sint*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBInstr(dec_instr, dest_reg, src1_opr, src2_opr);
                dec_instr_eu->SetPreferredPrintMode(MODE_HEX);
            }
            break;

            case 0x31 :
            {
                /*SUBAB Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBABInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x35 :
            {
                /*SUBAH Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBAHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x39 :
            {
                /*SUBAW Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBAWInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x33 :
            {
                /*SUBAB Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBABInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x37 :
            {
                /*SUBAH Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBAHInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            case 0x3b :
            {
                /*SUBAW Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
                C62xOperand * src1_opr = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetSrc2());
                C62xOperand * src2_opr = new C62xConstant(false, 5, dh->GetUIntConst1());
                C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());
                dec_instr_eu = new C62xSUBAWInstr(dec_instr, dest_reg, src1_opr, src2_opr);
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitD_LDSTOFFSET(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;
        uint8_t              base_reg_id  = (dh->GetDUnitId() == 0 ? 14 : 15);

        C62xOperand * src1_opr = new C62xConstant(false, 15, dh->GetUIntConst15());
        // Base Register Register always belong to SideB in this Mode i.e. UnitD_LDSTOFFSET
        C62xOperand * src2_opr = new C62xRegister(true, 32, REG_BANK_B, base_reg_id);
        C62xOperand * dest_reg = new C62xRegister(true, 32, dh->GetDestBankId(), dh->GetDest());

        // Here we correct the decoding unit id. Its a special case in this unit.
        dec_instr->GetExecutionUnit()->SetUnitId(dh->GetDUnitId() + 1);

        /* NOTE: In this Decoding Function; Addressing is always in 'Constant Positive Offset' mode. We don't use the mode
         * bits present in C62xDecodeHelper object.
         * Secondly the offset needs to be shifted left 0, 1 or 2 bits before actually accessing memory.
         */

        switch(dec_instr->GetOpcode())
        {
            case 0x00 :
            {
                /*LDHU Load HalfByte ucst15, m_dst; {+baseR[offset], dst} */// TODO: 1 bit left shift as well on ucst15
                dec_instr_eu = new C62xLDHUInstr(dec_instr, dest_reg, src1_opr, src2_opr, CST_POSITIVE_OFFSET);
                ((C62xLDSTInstr *) dec_instr_eu)->SetRequiresLeftShift(true);
                ((C62xLDSTInstr *) dec_instr_eu)->SetLeftShiftBits(1);
            }
            break;

            case 0x01 :
            {
                /*LDBU Load Byte ucst15, m_dst; {+baseR[offset], dst} */// No Left Shift on ucst15/offset required
                dec_instr_eu = new C62xLDBUInstr(dec_instr, dest_reg, src1_opr, src2_opr,CST_POSITIVE_OFFSET);
            }
            break;

            case 0x02 :
            {
                /*LDB Load Byte ucst15, m_dst; {+baseR[offset], dst} */// No Left Shift on ucst15/offset required
                dec_instr_eu = new C62xLDBInstr(dec_instr, dest_reg, src1_opr, src2_opr, CST_POSITIVE_OFFSET);
            }
            break;

            case 0x03 :
            {
                /*STB Store Byte; {src, +baseR[offset]}*/// No Left Shift on ucst15/offset required
                dec_instr_eu = new C62xSTBInstr(dec_instr, dest_reg, src1_opr, src2_opr, CST_POSITIVE_OFFSET);
            }
            break;

            case 0x04 :
            {
                /*LDH Load HlafByte ucst15, m_dst; {+baseR[offset], dst}*/// TODO: 1 bit left shift as well on ucst15/offset
                dec_instr_eu = new C62xLDHInstr(dec_instr, dest_reg, src1_opr, src2_opr, CST_POSITIVE_OFFSET);
                ((C62xLDSTInstr *) dec_instr_eu)->SetRequiresLeftShift(true);
                ((C62xLDSTInstr *) dec_instr_eu)->SetLeftShiftBits(1);
            }
            break;

            case 0x05 :
            {
                /*STH Store Halfword; {src, +baseR[offset]}*/// TODO: 1 bit left shift as well on ucst15/offset
                dec_instr_eu = new C62xSTHInstr(dec_instr, dest_reg, src1_opr, src2_opr, CST_POSITIVE_OFFSET);
                ((C62xLDSTInstr *) dec_instr_eu)->SetRequiresLeftShift(true);
                ((C62xLDSTInstr *) dec_instr_eu)->SetLeftShiftBits(1);
            }
            break;

            case 0x06 :
            {
                /*LDW Load Word ucst15, m_dst; {+baseR[offset], dst}*/// TODO: 2 bit left shift as well on ucst15/offset
                //dec_instr_eu = new C62xLDWInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
                dec_instr_eu = new C62xLDWInstr(dec_instr, dest_reg, src1_opr, src2_opr, CST_POSITIVE_OFFSET);
                ((C62xLDSTInstr *) dec_instr_eu)->SetRequiresLeftShift(true);
                ((C62xLDSTInstr *) dec_instr_eu)->SetLeftShiftBits(2);
            }
            break;

            case 0x07 :
            {
                /*STW Store Word; {src, +baseR[offset]}*/// TODO: 2 bit left shift as well on ucst15/offset
                // dec_instr_eu = new C62xSTWInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
                dec_instr_eu = new C62xSTWInstr(dec_instr, dest_reg, src1_opr, src2_opr, CST_POSITIVE_OFFSET);
                ((C62xLDSTInstr *) dec_instr_eu)->SetRequiresLeftShift(true);
                ((C62xLDSTInstr *) dec_instr_eu)->SetLeftShiftBits(2);
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitD_LDSTBASEROFFSET(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;
        dec_instr->GetExecutionUnit()->SetUnitId(dh->GetDUnitId() + 1);
        C62xOperand * src1_opr = NULL;       // Can be a Register or Constant Offset; Depends on Addressing Mode
        C62xOperand * src2_opr = new C62xRegister(false, 32, (C62xRegisterBank_t) dh->GetDUnitId(), dh->GetSrc2());
        C62xOperand * dest_reg = new C62xRegister(false, 32, dh->GetDestBankId(), dh->GetDest());

        switch(dh->GetAddressingMode())
        {
            case CST_NEGATIVE_OFFSET:        // *-R[ucst5]
            case CST_POSITIVE_OFFSET:        // *+R[ucst5]
            case CST_OFFSET_PRE_DECR:        // *--R[ucst5]
            case CST_OFFSET_PRE_INCR:        // *++R[ucst5]
            case CST_OFFSET_POST_DECR:       // *R--[ucst5]
            case CST_OFFSET_POST_INCR:       // *R++[ucst5]
                src1_opr = new C62xConstant(false, 5, dh->GetSrc1());
                break;

            case REG_NEGATIVE_OFFSET:        // *-R[offsetR]
            case REG_POSITIVE_OFFSET:        // *+R[offsetR]
            case REG_OFFSET_PRE_DECR:        // *--R[offsetR]
            case REG_OFFSET_PRE_INCR:        // *++R[offsetR]
            case REG_OFFSET_POST_DECR:       // *R--[offsetR]
            case REG_OFFSET_POST_INCR:       // *R++[offsetR]
                // TODO: Verify that the bank id is correct in the following Register !!!
                // Can we have the offset register from the other Register Bank ?
                src1_opr = new C62xRegister(false, 32, dh->GetCrossBankId(), dh->GetSrc1());
                break;
        }

        ASSERT(src1_opr != NULL, "Source 1 Operand NULL");

        switch(dec_instr->GetOpcode())
        {
            case 0x00 :
            {
                /*LDHU Load Byte*/
                dec_instr_eu = new C62xLDHUInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            case 0x01 :
            {
                /*LDBU Load Byte*/
                dec_instr_eu = new C62xLDBUInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            case 0x02 :
            {
                /*LDB Load Byte*/
                dec_instr_eu = new C62xLDBInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            case 0x03 :
            {
                /*STB Store Byte*/
                dec_instr_eu = new C62xSTBInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            case 0x04 :
            {
                /*LDH Load Byte*/
                dec_instr_eu = new C62xLDHInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            case 0x05 :
            {
                /*STH Store Byte*/
                dec_instr_eu = new C62xSTHInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            case 0x06 :
            {
                /*LDW Load Byte*/
                dec_instr_eu = new C62xLDWInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            case 0x07 :
            {
                /*STW Store Byte*/
                dec_instr_eu = new C62xSTWInstr(dec_instr, dest_reg, src1_opr, src2_opr, dh->GetAddressingMode());
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    DecodedInstruction * C62xInstructionDecoder :: DecInstrByUnitI(C62xDecodedInstruction * dec_instr, C62xDecodeHelper * dh)
    {
        DecodedInstruction * dec_instr_eu = NULL;

        uint32_t instr = dec_instr->GetParentInstruction()->GetValue();
        uint8_t ucst4 = ((instr >> 13) & 0x000f) + 1;

        // Remove the associated excution unit object instance.
        C62xExecutionUnit * execution_unit = dec_instr->GetExecutionUnit();
        if(execution_unit)
        {
            dec_instr->SetExecutionUnit(NULL);
            delete execution_unit;
            execution_unit = NULL;
        }

        switch(dec_instr->GetOpcode())
        {
            case 0x00 :
            {
                if(ucst4 > 1)
                {
                    C62xOperand * src2_opr = new C62xConstant(false, 4, ucst4);
                    dec_instr_eu = new C62xNOPInstr(dec_instr, src2_opr);
                }
                else
                {
                    dec_instr_eu = new C62xNOPInstr(dec_instr);
                }
            }
            break;

            case 0x78 :
            {
                dec_instr_eu = new C62xIDLEInstr(dec_instr);
            }
            break;

            default:
                DOUT << "Error: Unknown Instruction Type" << endl;
            break;
        }

        ASSERT(dec_instr_eu != NULL, "Decoding Failed !!!");
        return (dec_instr_eu);
    }

    C62xRegister * C62xInstructionDecoder :: GetConditionRegister(uint32_t instr)
    {
        C62xRegister * cond_reg = NULL;
        uint8_t        creg     = (instr >> 29) & 0x07;

	if (creg != 0)
        {
            cond_reg = new C62xRegister;

            if (creg == CREG_B0)
            {
                cond_reg->SetBankId(REG_BANK_B);     // B
                cond_reg->SetRegId(0);               // 0
            }
            else if (creg == CREG_B1)
            {
                cond_reg->SetBankId(REG_BANK_B);     // B
                cond_reg->SetRegId(1);               // 1
            }
            else if (creg == CREG_B2)
            {
                cond_reg->SetBankId(REG_BANK_B);     // B
                cond_reg->SetRegId(2);               // 2
            }
            else if (creg == CREG_A1)
            {
                cond_reg->SetBankId(REG_BANK_A);     // A
                cond_reg->SetRegId(1);               // 1
            }
            else if (creg == CREG_A2)
            {
                cond_reg->SetBankId(REG_BANK_A);     // A
                cond_reg->SetRegId(2);               // 2
            }
	}

        return (cond_reg);
    }

    C62xExecutionUnit * C62xInstructionDecoder :: GetExecutionUnit(uint32_t instr)
    {
        C62xExecutionUnit * execution_unit = new C62xExecutionUnit();

        if (getUnit_3bit(instr) == LUNIT)
        {
            // .L unit instruction
            execution_unit->SetUnitType(L_EXUNIT);
            execution_unit->SetUnitSubType(LUNIT);
        }
        else if (getUnit_5bit(instr) == MUNIT)
        {
            // .M unit instruction
            execution_unit->SetUnitType(M_EXUNIT);
            execution_unit->SetUnitSubType(MUNIT);
        }
        else if (getUnit_5bit(instr) == DUNIT)
        {
            // .D unit instruction
            execution_unit->SetUnitType(D_EXUNIT);
            execution_unit->SetUnitSubType(DUNIT);
        }
        else if (getUnit_2bit(instr) == DUNIT_LDSTOFFSET)
        {
            // .D unit load/store
            execution_unit->SetUnitType(D_EXUNIT);
            execution_unit->SetUnitSubType(DUNIT_LDSTOFFSET);
        }
        else if (getUnit_2bit(instr) == DUNIT_LDSTBASEROFFSET)
        {
            // .D unit load/store with baseR/offsetR specified
            execution_unit->SetUnitType(D_EXUNIT);
            execution_unit->SetUnitSubType(DUNIT_LDSTBASEROFFSET);
        }
        else if (getUnit_4bit(instr) == SUNIT)
        {
            // .S unit instruction
            execution_unit->SetUnitType(S_EXUNIT);
            execution_unit->SetUnitSubType(SUNIT);
        }
        else if (getUnit_5bit(instr) == SUNIT_ADDK)
        {
            // .S unit ADDK instruction
            execution_unit->SetUnitType(S_EXUNIT);
            execution_unit->SetUnitSubType(SUNIT_ADDK);
        }
        else if (getUnit_4bit(instr) == SUNIT_IMMED)
        {
            // .S unit Field operations (immediate forms)
            execution_unit->SetUnitType(S_EXUNIT);
            execution_unit->SetUnitSubType(SUNIT_IMMED);
        }
        else if (getUnit_4bit(instr) == SUNIT_MVK)
        {
            // .S unit MVK
            execution_unit->SetUnitType(S_EXUNIT);
            execution_unit->SetUnitSubType(SUNIT_MVK);
        }
        else if (getUnit_5bit(instr) == SUNIT_BCOND)
        {
            // .S unit Bcond disp
            execution_unit->SetUnitType(S_EXUNIT);
            execution_unit->SetUnitSubType(SUNIT_BCOND);
        }
        else if (getUnit_11bit(instr) == NOP)
        {
            // NOP instruction
            execution_unit->SetUnitType(U_EXUNIT);
            execution_unit->SetUnitSubType(NOP);
        }
        else if (getUnit_16bit(instr) == IDLEINST)
        {
            execution_unit->SetUnitType(I_EXUNIT);
            execution_unit->SetUnitSubType(IDLEUNIT);
        }
        else
        {
            delete execution_unit;
            execution_unit = NULL;
            /* unknown instruction kind */
            DOUT << "Error: Unknown Instruction Type" << endl;
            return(NULL);
        }

        return(execution_unit);
    }

    uint16_t C62xInstructionDecoder :: GetOpcode(uint32_t instr)
    {
        uint16_t opcode = 0;

        if (getUnit_3bit(instr) == LUNIT)
        {
            // .L unit instruction
            opcode = (instr >> 5) & 0x7f;
        }
        else if (getUnit_5bit(instr) == MUNIT)
        {
            // .M unit instruction
            opcode = (instr >> 7) & 0x1f;
        }
        else if (getUnit_5bit(instr) == DUNIT)
        {
            // .D unit instruction
            opcode = (instr >> 7) & 0x3f;
        }
        else if (getUnit_2bit(instr) == DUNIT_LDSTOFFSET)
        {
            // .D unit load/store
            opcode = (instr >> 4) & 0x07;
        }
        else if (getUnit_2bit(instr) == DUNIT_LDSTBASEROFFSET)
        {
            // .D unit load/store with baseR/offsetR specified
            opcode = (instr >> 4) & 0x07;
        }
        else if (getUnit_4bit(instr) == SUNIT)
        {
            // .S unit instruction
            opcode = (instr >> 6) & 0x3f;
        }
        else if (getUnit_5bit(instr) == SUNIT_ADDK)
        {
            // .S unit ADDK instruction
            opcode = 0; //nothing specified
        }
        else if (getUnit_4bit(instr) == SUNIT_IMMED)
        {
            // .S unit Field operations (immediate forms)
            opcode = (instr >> 6) & 0x03;
        }
        else if (getUnit_4bit(instr) == SUNIT_MVK)
        {
            // .S unit MVK
            opcode = 0; // nothing specified
        }
        else if (getUnit_5bit(instr) == SUNIT_BCOND)
        {
            // .S unit Bcond disp
            opcode = 0; // nothing specified
        }
        else if (getUnit_11bit(instr) == NOP)
        {
            // NOP instruction
            opcode = 0;
        }
        else if (getUnit_16bit(instr) == IDLEINST)
        {
            opcode = IDLEOP;
        }

        return (opcode);
    }

    bool C62xInstructionDecoder :: GetCrossPathAccessFlag(uint32_t instr)
    {
        bool cross_path_access = false;

        if (getUnit_3bit(instr) == LUNIT)
        {
            // .L unit instruction
            cross_path_access = (instr >> 12) & 0x01;
        }
        else if (getUnit_5bit(instr) == MUNIT)
        {
            // .M unit instruction
            cross_path_access = (instr >> 12) & 0x01;
        }
        else if (getUnit_4bit(instr) == SUNIT)
        {
            // .S unit instruction
            cross_path_access = (instr >> 12) & 0x01;
        }

        return (cross_path_access);
    }
}
