/*************************************************************************************
 * File   : transcode_instruction.cpp,
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

#include "tms320C6x_instruction.h"
#include "unified_instruction.h"

//#include "block_packets.h"
#include <iostream>
#include <iomanip>

ir_instr_t * tms320C6x_instruction :: transcode()
{
    ir_instr_t * pir_instr = new ir_instr_t();
    if(!pir_instr)
    {
        DOUT << "Error: Allocating Object for IR Instruction" << endl;
        return (NULL);
    }

    pir_instr->set_address(p_raw_instr->get_vir_addrs());

    if(m_creg)
    {
        pir_instr->set_conditional(true);
        pir_instr->set_condition(m_z);

        if(m_creg == CREG_B0 || m_creg == CREG_B1 || m_creg == CREG_B2)
        {
            pir_instr->set_condition_reg(m_creg + REG_BANK_SIZE -1);
        }
        else if(m_creg == CREG_A1 || m_creg == CREG_A2)
        {
            pir_instr->set_condition_reg(m_creg - 3);
        }
        else
        {
            DOUT << "Error: Unknown Condition Register";
            return (NULL);
        }
    }

    if(transcode_opcode_operands(pir_instr))
    {
        DOUT << "Error: Setting Opcode & Operands" << endl;
        return (NULL);
    }

    return(pir_instr);
}

int tms320C6x_instruction :: transcode_opcode_operands(ir_instr_t * pir_instr)
{
    switch(m_unit)
    {
        case LUNIT:
            transcode_lunit(pir_instr);
            break;

        case MUNIT:
        /*case IDLEUNIT:*/
            if(m_op == 0x00 || m_op == 0x78)
            {
					    ;//print_idle(&cout);
						}
            else
						{            
					    ;//print_munit(out);
						}
            break;

        case DUNIT:
            //print_dunit(out);
            break;

        case DUNIT_LDSTOFFSET:
            //print_dunit_ldstoffset(out);
            //transcode_dunit_ldstoffset(pir_instr);
            break;

        case DUNIT_LDSTBASEROFFSET:
            transcode_dunit_ldstbaseroffset(pir_instr);
            break;

        case SUNIT:
            //print_sunit(out);
            break;

        case SUNIT_ADDK:
            //print_sunit_addk(out);
            break;

        case SUNIT_IMMED:
            //print_sunit_immed(out);
            break;

        case SUNIT_MVK:
            //print_sunit_mvk(out);
            break;

        case SUNIT_BCOND:
            transcode_sunit_bcond(pir_instr);
            break;

        default:
            DOUT << "Error: Unknown Functional Unit Type" << endl;
            break;
    }
    return(0);
}

// Transcode Helper Functions
int tms320C6x_instruction :: transcode_lunit(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
#if 0
        case 0x1a :
            /*ABS xsint, sint*/
            iprint2(out, "ABS", m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x38 :
            /*ABS slong, slong*/
            iprint2(out, "ABS", m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILL);
            break;

        case 0x03 :
            /*ADD sint, xsint, sint*/
            transcode_regops(pir_instr, "ADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x23 :
            /*ADD sint, xsint, slong*/
            transcode_regops(pir_instr, "ADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x2b :
            /*ADDU usint, xusint, uslong*/
            transcode_regops(pir_instr, "ADDU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x21 :
            /*ADD xsint, slong, slong*/
            transcode_regops(pir_instr, "ADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILL);
            break;

        case 0x29 :
            /*ADDU xusint, uslong, uslong*/
            transcode_regops(pir_instr, "ADDU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILL);
            break;

        case 0x02 :
            /*ADD scst5, xsint, sint*/
            transcode_cstregops(pir_instr, "ADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x20 :
            /*ADD scst5, slong, slong*/
            transcode_cstregops(pir_instr, "ADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILL);
            break;

        case 0x7b :
            /*AND usint, xusint, usint*/
            transcode_regops(pir_instr, "AND", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x7a :
            /*AND scst5, xusint, usint*/
            transcode_cstregops(pir_instr, "AND", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;
#endif
        case 0x53 :
            /*CMPEQ sint, xsint, usint*/
            transcode_regops(pir_instr, "CMPEQ", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x52 :
            /*CMPEQ scst5, xsint, usint*/
            transcode_cstregops(pir_instr, "CMPEQ", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x51 :
            /*CMPEQ xsint, slong, usint*/
            transcode_regops(pir_instr, "CMPEQ", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x50 :
            /*CMPEQ scst5, slong, usint*/
            transcode_cstregops(pir_instr, "CMPEQ", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;
#if 0
        case 0x47 :
            /*CMPGT sint, xsint, usint*/
            transcode_regops(pir_instr, "CMPGT", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x46 :
            /*CMPGT scst5, xsint, usint*/
            transcode_cstregops(pir_instr, "CMPGT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x45 :
            /*CMPGT xsint, slong, usint*/
            transcode_regops(pir_instr, "CMPGT", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x44 :
            /*CMPGT scst5, slong, usint*/
            transcode_cstregops(pir_instr, "CMPGT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x4f :
            /*CMPGTU usint, xusint, usint*/
            transcode_regops(pir_instr, "CMPGTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x4e :
            /*CMPGTU ucst5, xusint, usint*/
            transcode_cstregops(pir_instr, "CMPGTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x4d :
            /*CMPGTU xusint, uslong, usint*/
            transcode_regops(pir_instr, "CMPGTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x4c :
            /*CMPGTU ucst5, uslong, usint*/
            transcode_cstregops(pir_instr, "CMPGTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x57 :
            /*CMPLT sint, xsint, usint*/
            transcode_regops(pir_instr, "CMPLT", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x56 :
            /*CMPLT scst5, xsint, usint*/
            transcode_cstregops(pir_instr, "CMPLT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x55 :
            /*CMPLT xsint, slong, usint*/
            transcode_regops(pir_instr, "CMPLT", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x54 :
            /*CMPLT scst5, slong, usint*/
            transcode_cstregops(pir_instr, "CMPLT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x5f :
            /*CMPLTU usint, xusint, usint*/
            transcode_regops(pir_instr, "CMPLTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x5e :
            /*CMPLTU ucst5, xusint, usint*/
            transcode_cstregops(pir_instr, "CMPLTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x5d :
            /*CMPLTU xusint, uslong, usint*/
            transcode_regops(pir_instr, "CMPLTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x5c :
            /*CMPLTU ucst5, uslong, usint*/
            transcode_cstregops(pir_instr, "CMPLTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x6b :
            /*LMBD - Left Most Bit Detection usint, xusint, usint*/
            transcode_regops(pir_instr, "LMBD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x6a :
            /*LMBD - Left Most Bit Detection ucst5, xusint, usint*/
            transcode_cstregops(pir_instr, "LMBD", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x63 :
            /*NORM Normalize Integer, # of redundant sign bits are found xsint, usint*/
            iprint2(out, "NORM", m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x60 :
            /*NORM Normalize Integer, # of redundant sign bits are found slong, usint*/
            iprint2(out, "NORM", m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;
#endif
        case 0x7f :
            /*Bitwise OR usint, xusint, usint*/
            transcode_regops(pir_instr, "OR", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x7e :
            /*Bitwise OR scst5, xusint, usint*/
            transcode_cstregops(pir_instr, "OR", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;
#if 0
        case 0x13 :
            /*SADD Integer addition with saturation to result size sint, xsint, sint*/
            transcode_regops(pir_instr, "SADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x31 :
            /*SADD Integer addition with saturation to result size xsint, slong, slong*/
            transcode_regops(pir_instr, "SADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILL);
            break;

        case 0x12 :
            /*SADD Integer addition with saturation to result size scst5, xsint, sint*/
            transcode_cstregops(pir_instr, "SADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x30 :
            /*SADD Integer addition with saturation to result size scst5, slong, slong*/
            transcode_cstregops(pir_instr, "SADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILL);
            break;

        case 0x40 :
            if(m_src1 == 0x00){
                /*SAT saturate a 40bit integer to a 32bit integer slong, sint*/
                iprint2(out, "SAT", m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            }
            else
            {
                DOUT << "Error: Invalid Instruction. src1 = 0x" << hex << m_src1 << endl;
            }
            break;

        case 0x0f :
            /*SSUB Integer addition with saturation to result size sint, xsint, sint*/
            transcode_regops(pir_instr, "SSUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x1f :
            /*SSUB Integer addition with saturation to result size xsint, sint, sint*/
            transcode_regops(pir_instr, "SSUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_III);
            break;

        case 0x0e :
            /*SSUB Integer addition with saturation to result size scst5, xsint, sint*/
            transcode_cstregops(pir_instr, "SSUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x2c :
            /*SSUB Integer addition with saturation to result size scst5, slong, slong*/
            transcode_cstregops(pir_instr, "SSUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILL);
            break;

        case 0x07 :
            /*SUB without saturation sint, xsint, sint*/
            transcode_regops(pir_instr, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x17 :
            /*SUB without saturation xsint, sint, sint*/
            transcode_regops(pir_instr, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_III);
            break;

        case 0x27 :
            /*SUB without saturation sint, xsint, slong*/
            transcode_regops(pir_instr, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x37 :
            /*SUB without saturation xsint, sint, slong*/
            transcode_regops(pir_instr, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_IIL);
            break;

        case 0x2f :
            /*SUBU without saturation usint, xusint, uslong*/
            transcode_regops(pir_instr, "SUBU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x3f :
            /*SUBU without saturation xusint, usint, uslong*/
            transcode_regops(pir_instr, "SUBU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_IIL);
            break;

        case 0x06 :
            /*SUB without saturation scst5, xsint, sint*/
            transcode_cstregops(pir_instr, "SUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x24 :
            /*SUB without saturation scst5, slong, slong*/
            transcode_cstregops(pir_instr, "SUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_ILL);
            break;

        case 0x4b :
            /*SUBC Conditional Integer Subtract and Shift - used for division usint, xusint, usint*/
            transcode_regops(pir_instr, "SUBC", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x6f :
            /*XOR usint, xusint, usint*/
            transcode_regops(pir_instr, "XOR", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x6e :
            /*XOR scst5, xusint, usint*/
            transcode_cstregops(pir_instr, "XOR", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;
#endif
        default:
            DOUT << "Error: Unknown Instruction for LUNIT" << endl;
            break;
    }
    return (0);
}

#if 0
int tms320C6x_instruction :: transcode_sunit(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
 	case 0x07 :
            /*ADD sint, xsint, sint*/
            transcode_regops(pir_instr, "ADD", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x06 :
            /*ADD scst5, xsint, sint*/
            transcode_cstregops(pir_instr, "ADD", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x01 :
            /*ADD2 sint, xsint, sint*/
            transcode_regops(pir_instr, "ADD2", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1f :
            /*AND usint, xusint, usint*/
            transcode_regops(pir_instr, "AND", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1e :
            /*AND scst5, xusint, usint*/
            transcode_cstregops(pir_instr, "AND", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0d :
            /*Branch using a Register, xusint*/
            iprint_breg(out, "B", m_src2, "S", m_s, m_x, 2);
            break;

        case 0x03 :
            if(m_src2 == 0x06){
                /*Branch Using an IRP - Interrupt Return Pointer*/
                iprint0(out, "B IRP ");
            }
            else if(m_src2 == 0x07){
                /*Branch using NMI Return Pointer & set NMIE in crf[IER]*/
                iprint0(out, "B NRP ");
            }
            else{
                DOUT << "Error: Unknown Instruction" << endl;
            }
            break;

	case 0x3f :
            /*CLR Clear Bit Fields of m_src2 whose bounds are given by 5-9 & 0-4 of m_src1*/
            /*usint, xusint, usint*/
            transcode_regops(pir_instr, "CLR", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x2f :
            /*EXT Extract & Sign Extend a bit field*/
            /*usint, xsint, sint*/
            transcode_regops(pir_instr, "EXT", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x2b :
            /*EXTU Extract & Zero Extend a bit field*/
            /*usint, xusint, usint*/
            transcode_regops(pir_instr, "EXTU", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0f :
            /*MVC Move between Control File & Register File usint, usint*/
            // Control File --> Register File
            iprint2mvc(out, "MVC", m_src2, m_dst, "S", m_s, m_x, 0);
            break;

	case 0x0e :
            /*MVC Move between Control File & Register File xusint, usint*/
            // Register File --> Control File
            iprint2mvc(out, "MVC", m_src2, m_dst, "S", m_s, m_x, 2);
            break;

	case 0x1b :
            /*Bitwise OR usint, xusint, usint*/
            transcode_regops(pir_instr, "OR", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1a :
            /*Bitwise OR scst5, xusint, usint*/
            transcode_cstregops(pir_instr, "OR", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x3b :
            /*SET Set Bit Fields of m_src2 whose bounds are given by 5-9 & 0-4 of m_src1*/
            /*usint, xusint, usint*/
            transcode_regops(pir_instr, "SET", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x33 :
            /*SHL Shift Left by amount given in m_src1 usint, xsint, sint*/
            iprint_shift(out, "SHL", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x31 :
            /*SHL Shift Left by amount given in m_src1 usint, slong, slong*/
            iprint_shift(out, "SHL", m_src1, m_src2, m_dst, "S", m_s, m_x, 0, OPTYPE_ILL);
            break;

	case 0x13 :
            /*SHL Shift Left by amount given in m_src1 usint, xusint, uslong*/
            iprint_shift(out, "SHL", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_IIL);
            break;

	case 0x32 :
            /*SHL Shift Left by amount given in m_src1 ucst5, xsint, sint*/
            iprintc1_shift(out, "SHL", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x30 :
            /*SHL Shift Left by amount given in m_src1 ucst5, slong, slong*/
            iprintc1_shift(out, "SHL", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 0, OPTYPE_ILL);
            break;

	case 0x12 :
            /*SHL Shift Left by amount given in m_src1 ucst5, xusint, uslong*/
            iprintc1_shift(out, "SHL", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_IIL);
            break;

	case 0x37 :
            /*SHR Shift Right  by amount given in m_src1 usint, xsint, sint*/
            iprint_shift(out, "SHR", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x35 :
            /*SHR Shift Right by amount given in m_src1 usint, slong, slong*/
            iprint_shift(out, "SHR", m_src1, m_src2, m_dst, "S", m_s, m_x, 0, OPTYPE_ILL);
            break;

	case 0x36 :
            /*SHR Shift Right by amount given in m_src1 ucst5, xsint, sint*/
            iprintc1_shift(out, "SHR", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x34 :
            /*SHR Shift Right by amount given in m_src1 ucst5, slong, slong*/
            iprintc1_shift(out, "SHR", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 0, OPTYPE_ILL);
            break;

	case 0x27 :
            /*SHRU Logical Shift Right by amount given in m_src1 usint, xusint, usint*/
            iprint_shift(out, "SHRU", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x25 :
            /*SHRU Logical Shift Right by amount given in m_src1 usint, uslong, uslong*/
            iprint_shift(out, "SHRU", m_src1, m_src2, m_dst, "S", m_s, m_x, 0, OPTYPE_ILL);
            break;

	case 0x26 :
            /*SHRU Logical Shift Right by amount given in m_src1 ucst5, xusint, usint*/
            iprintc1_shift(out, "SHRU", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x24 :
            /*SHRU Logical Shift Right by amount given in m_src1 ucst5, uslong, uslong*/
            iprintc1_shift(out, "SHRU", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 0, OPTYPE_ILL);
            break;

	case 0x23 :
            /*SSHL Shift Left with Saturation usint, xsint, sint*/
            transcode_regops(pir_instr, "SSHL", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x22 :
            /*SSHL Shift Left with Saturation ucst5, xsint, sint*/
            transcode_cstregops(pir_instr, "SSHL", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x17 :
            /*SUB without saturation sint, xsint, sint*/
            transcode_regops(pir_instr, "SUB", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x16 :
            /*SUB without saturation scst5, xsint, sint*/
            transcode_cstregops(pir_instr, "SUB", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x11 :
            /*SUB2 Subtractions of lower and upper halfs sint, xsint, sint*/
            transcode_regops(pir_instr, "SUB2", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0b :
            /*XOR usint, xusint, usint*/
            transcode_regops(pir_instr, "XOR", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0a :
            /*XOR scst5, xusint, usint*/
            transcode_cstregops(pir_instr, "XOR", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

        default:
            DOUT << "Error: Unknown Instruction for SUNIT" << endl;
            break;
    }
}

int tms320C6x_instruction :: transcode_munit(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
	case 0x19 :
            /*MPY Integer Multiply 16lsb x 16lsb; slsb16, xslsb16, sint*/
            transcode_regops(pir_instr, "MPY", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1f :
            /*MPYU Integer Multiply 16lsb x 16lsb; ulsb16, xulsb16, usint*/
            transcode_regops(pir_instr, "MPYU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1d :
            /*MPYUS Integer Multiply 16lsb x 16lsb; ulsb16, xslsb16, sint*/
            transcode_regops(pir_instr, "MPYUS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1b :
            /*MPYSU Integer Multiply 16lsb x 16lsb; slsb16, xulsb16, sint*/
            transcode_regops(pir_instr, "MPYSU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x18 :
            /*MPY Integer Multiply 16lsb x 16lsb; scst5, xslsb16, sint*/
            transcode_cstregops(pir_instr, "MPY", m_sicst1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1e :
            /*MPYSU Integer Multiply 16lsb x 16lsb; scst5, xulsb16, sint*/
            transcode_cstregops(pir_instr, "MPYSU", m_sicst1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x01 :
            /*MPYH Integer Multiply 16msb x 16msb; smsb16, xsmsb16, sint*/
            transcode_regops(pir_instr, "MPYH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x07 :
            /*MPYHU Integer Multiply 16msb x 16msb; umsb16, xumsb16, usint*/
            transcode_regops(pir_instr, "MPYHU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x05 :
            /*MPYHUS Integer Multiply 16msb x 16msb; umsb16, xsmsb16, sint*/
            transcode_regops(pir_instr, "MPYHUS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x03 :
            /*MPYHSU Integer Multiply 16msb x 16msb; smsb16, xumsb16, sint*/
            transcode_regops(pir_instr, "MPYHSU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x09 :
            /*MPYHL Integer Multiply 16msb x 16lsb; smsb16, xslsb16, sint*/
            transcode_regops(pir_instr, "MPYHL", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0f :
            /*MPYHLU Integer Multiply 16msb x 16lsb; umsb16, xulsb16, usint*/
            transcode_regops(pir_instr, "MPYHLU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0d :
            /*MPYHULS Integer Multiply 16msb x 16lsb; umsb16, xslsb16, sint*/
            transcode_regops(pir_instr, "MPYHULS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0b :
            /*MPYHSLU Integer Multiply 16msb x 16lsb; smsb16, xulsb16, sint*/
            transcode_regops(pir_instr, "MPYHSLU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x11 :
            /*MPYLH Integer Multiply 16lsb x 16msb; slsb16, xsmsb16, sint*/
            transcode_regops(pir_instr, "MPYLH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x17 :
            /*MPYLHU Integer Multiply 16lsb x 16msb; ulsb16, xumsb16, usint*/
            transcode_regops(pir_instr, "MPYLHU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x15 :
            /*MPYLUHS Integer Multiply 16lsb x 16msb ulsb16, xsmsb16, sint*/
            transcode_regops(pir_instr, "MPYLUHS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

#if 0
        //TODO: How to differentiate this following case from similar one above?
	case 0x11 :
            /*MPYLSHU Integer Multiply 16lsb x 16msb slsb16, xumsb16, sint*/
            transcode_regops(pir_instr, "MPYLSHU {slsb16, xumsb16, sint}", m_src1, m_src2, m_dst, "M", m_s, m_x, 2);
            break;
#endif
        case 0x1a :
            /*SMPY slsb16, xslsb16, sint*/
            transcode_regops(pir_instr, "SMPY", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0a :
            /*SMPYHL smsb16, xslsb16, sint*/
            transcode_regops(pir_instr, "SMPYHL", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x12 :
            /*SMPYLH slsb16, xsmsb16, sint*/
            transcode_regops(pir_instr, "SMPYLH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x02 :
            /*SMPYH smsb16, xsmsb16, sint*/
            transcode_regops(pir_instr, "SMPYH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

        default:
            DOUT << "Error: Unknown Instruction for MUNIT" << endl;
            break;
    }
}

int tms320C6x_instruction :: transcode_dunit(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
	case 0x10 :
            /* ADD sint, sint, sint*/
            iprint_rev(out, "ADD", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x12 :
            /* ADD ucst5, sint, sint*/
            iprint_revc(out, "ADD", m_uicst1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x30 :
            /*ADDAB Add Byte with Addressing sint, sint, sint;  {+baseR[offsetR], dst}*/
            iprint_rev(out, "ADDAB", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x34 :
            /*ADDAH Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
            iprint_rev(out, "ADDAH", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x38 :
            /*ADDAW Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
            iprint_rev(out, "ADDAW", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x32 :
            /*ADDAB Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
            iprint_revc(out, "ADDAB", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x36 :
            /*ADDAH Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
            iprint_revc(out, "ADDAH", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x3a :
            /*ADDAW Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
            iprint_revc(out, "ADDAW", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x11 :
            /*SUB without saturation sint, sint, sint*/
            iprint_rev(out, "SUB", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x13 :
            /*SUB without saturation ucst5, sint, sint*/
            iprint_revc(out, "SUB", m_uicst1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x31 :
            /*SUBAB Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
            iprint_rev(out, "SUBAB", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x35 :
            /*SUBAH Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
            iprint_rev(out, "SUBAH", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x39 :
            /*SUBAW Add Byte with Addressing sint, sint, sint; {+baseR[offsetR], dst}*/
            iprint_rev(out, "SUBAW", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x33 :
            /*SUBAB Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
            iprint_revc(out, "SUBAB", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x37 :
            /*SUBAH Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
            iprint_revc(out, "SUBAH", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

	case 0x3b :
            /*SUBAW Add Byte with Addressing ucsut5, sint, sint; {+baseR[offset], dst}*/
            iprint_revc(out, "SUBAW", m_src1, m_src2, m_dst, "D", m_s, m_x);
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }
}
#endif

int tms320C6x_instruction :: transcode_dunit_ldstbaseroffset(ir_instr_t * pir_instr)
{
    switch(m_op){
	case 0x02 :
            /*LDB Load Byte*/
            switch (m_mode) {
                case 0x4:   case 0x5:   case 0xc:   case 0xd:   case 0xe:   case 0xf:
                    transcode_ld(pir_instr, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

                case 0x0:   case 0x1:   case 0x8:   case 0x9:   case 0xa:   case 0xb:
                    transcode_ldc(pir_instr, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;
            }
            break;

	case 0x01 :
            /*LDBU Load Byte*/
            switch (m_mode) {
                case 0x4:   case 0x5:   case 0xc:   case 0xd:   case 0xe:   case 0xf:
                    transcode_ld(pir_instr, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

                case 0x0:   case 0x1:   case 0x8:   case 0x9:   case 0xa:   case 0xb:
                    transcode_ldc(pir_instr, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;
            }
            break;

	case 0x04 :
            /*LDH Load Byte*/
            switch (m_mode) {
                case 0x4:   case 0x5:   case 0xc:   case 0xd:   case 0xe:   case 0xf:
                    transcode_ld(pir_instr, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

                case 0x0:   case 0x1:   case 0x8:   case 0x9:   case 0xa:   case 0xb:
                    transcode_ldc(pir_instr, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;
            }
            break;

	case 0x00 :
            /*LDHU Load Byte*/
            switch (m_mode) {
                case 0x4:   case 0x5:   case 0xc:   case 0xd:   case 0xe:   case 0xf:
                    transcode_ld(pir_instr, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

                case 0x0:   case 0x1:   case 0x8:   case 0x9:   case 0xa:   case 0xb:
                    transcode_ldc(pir_instr, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;
            }
            break;

	case 0x06 :
            /*LDW Load Byte*/
            switch (m_mode) {
                case 0x4:   case 0x5:   case 0xc:   case 0xd:   case 0xe:   case 0xf:
                    transcode_ld(pir_instr, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

                case 0x0:   case 0x1:   case 0x8:   case 0x9:   case 0xa:   case 0xb:
                    transcode_ldc(pir_instr, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;
            }
            break;
#if 0
	case 0x03 :
            /*STB Store Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintst(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintst(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintst(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintst(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintst(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintst(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintstc(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintstc(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintstc(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintstc(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintstc(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintstc(pir_instr, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintst(pir_instr, "STB **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;

	case 0x05 :
            /*STH Store Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintst(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintst(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintst(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintst(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintst(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintst(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintstc(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintstc(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintstc(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintstc(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintstc(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintstc(pir_instr, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintst(pir_instr, "STH **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;

	case 0x07 :
            /*STW Store Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintst(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintst(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintst(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintst(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintst(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintst(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintstc(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintstc(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintstc(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintstc(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintstc(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintstc(pir_instr, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintst(pir_instr, "STW **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;
#endif
        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }

    return (0); 
}

int tms320C6x_instruction :: transcode_dunit_ldstoffset(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
	case 0x00 :
            /*LDHU Load HalfByte ucst15, m_dst; {+baseR[offset], dst} */
            //transcode_ldc(out, "LDHU", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x01 :
            /*LDBU Load Byte ucst15, m_dst; {+baseR[offset], dst} */
            //transcode_ldc(out, "LDBU", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x02 :
            /*LDB Load Byte ucst15, m_dst; {+baseR[offset], dst} */
            transcode_ldc(pir_instr, "LDB", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x03 :
            /*STB Store Byte; {src, +baseR[offset]}*/
            //iprintstc(out, "STB", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x04 :
            /*LDH Load HlafByte ucst15, m_dst; {+baseR[offset], dst}*/
            //transcode_ldc(out, "LDH", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x05 :
            /*STH Store Halfword; {src, +baseR[offset]}*/
            //iprintstc(out, "STH", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x06 :
            /*LDW Load Word ucst15, m_dst; {+baseR[offset], dst}*/
            //iprintldc_offset(out, "LDW", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x07 :
            /*STW Store Word; {src, +baseR[offset]}*/
            //iprintstc_offset(out, "STW", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }
    return(0);
}

#if 0
int tms320C6x_instruction :: transcode_sunit_addk(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
	case 0x00 :
            /*ADDK cst16, usint*/
            iprint2c(out, "ADDK", m_addk_const & 0x0000FFFF, m_dst, "S", m_s, m_x, 0);
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }

}

int tms320C6x_instruction :: transcode_sunit_immed(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
	case 0x00 :
            /*EXTU Extract & Zero Extend a bit field*/
            /*ucst5, ucst5, usint, usint*/
            iprint4(out, "EXTU", m_immed_const1,
                   m_immed_const2, m_src2, m_dst, "S", m_s, m_x, 0);
            break;

	case 0x01 :
            /*EXT Extract & Sign Extend a bit field*/
            /*ucst5, ucst5, sint, sint*/
            iprint4(out, "EXT", m_immed_const1,
                   m_immed_const2, m_src2, m_dst, "S", m_s, m_x, 0);
            break;

        case 0x02 :
            /*SET Set Bit Fields of m_src2 whose bounds are given by csta & cstb*/
            /*ucst5, ucst5, usint, usint*/
            iprint4(out, "SET", m_immed_const1,
                   m_immed_const2, m_src2, m_dst, "S", m_s, m_x, 0);
            break;

	case 0x03 :
            /*CLR Clear Bit Fields of m_src2 whose bounds are given by csta & cstb*/
            /*ucst5, ucst5, usint, usint*/
            iprint4(out, "CLR", m_immed_const1,
                   m_immed_const2, m_src2, m_dst, "S", m_s, m_x, 0);
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }
}

int tms320C6x_instruction :: transcode_sunit_mvk(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
        case 0x00:
            if(m_mvk_type == 0x00){
		/*MVK Move a 16bit signed constant into a Register & Sign Extend scst16, sint*/
		iprint2c(out, "MVK", m_mvk_sign_const, m_dst, "S", m_s, m_x, 0);
            }
            else if (m_mvk_type == 0x01){
		/*MVKH Move 16bit constant into the Upper Bits of a Register uscst16, sint*/
		iprint2c(out, "MVKH", m_mvk_unsign_const, m_dst, "S", m_s, m_x, 0);
            }
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }
}
#endif

int tms320C6x_instruction :: transcode_sunit_bcond(ir_instr_t * pir_instr)
{
    switch(m_op)
    {
        case 0x00:
            /*Branch Using a Displacement; {scst21}*/
            transcode_bcond(pir_instr, "B", m_bcond_disp);
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }
    return (0); 
}

#if 0
int tms320C6x_instruction :: transcode_idle(ir_instr_t * pir_instr)
{
    if(m_op == 0x78)
        iprint0(out, "IDLE");
    else if(m_op == 0x00)
        iprint01(out, "NOP", (uint32_t)m_nop_cnt);
    else
        DOUT << "Error: Unknown Instruction Type" << endl;
}

#endif

int tms320C6x_instruction :: transcode_regops(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2, char optype)
{
        int opcode_index = ir_instr_t :: get_opcode_index(inst);

        pir_instr->set_opcode(opcode_index);
        pir_instr->set_nm_operands(3);
        pir_instr->set_mode(m_mode);

        if(optype & 0x4)
            pir_instr->set_optype(0, OPTYPE_UREG, 64);
        else 
            pir_instr->set_optype(0, OPTYPE_UREG, 32);

        if(optype & 0x2)
            pir_instr->set_optype(1, OPTYPE_UREG, 64);
        else
            pir_instr->set_optype(1, OPTYPE_UREG, 32);

        if(optype & 0x1)
            pir_instr->set_optype(2, OPTYPE_UREG, 64);
        else
            pir_instr->set_optype(2, OPTYPE_UREG, 32);


        bool src1bank = ((m_x==1 && src1or2==1) ? !m_s : m_s);
        bool src2bank = ((m_x==1 && src1or2==2) ? !m_s : m_s);

        pir_instr->set_operand(0, (REG_BANK_SIZE * src1bank) + m_src1);
        pir_instr->set_operand(1, (REG_BANK_SIZE * src2bank) + m_src2);
        pir_instr->set_operand(2, (REG_BANK_SIZE * m_s) + m_dst);

        return (0);

#if 0
	char ab[3] = "AB";

        if((strcmp(inst, "SUB") == 0) && (m_src1 == m_src2) && !m_x){
            (*out) << "ZERO" << "." << unit << m_s+1 << (m_x?"X":"") <<"\t\t";
            if(optype & 0x1){
                (*out) << ab[m_s] << m_dst+1 << ":";
            }
            (*out) << ab[m_s] << m_dst;
        }
        else
        {
            (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") <<"\t\t";

            if(optype & 0x4){
                (*out) << ((m_x==1 && src1or2==1) ? ab[!m_s] : ab[m_s]) << m_src1+1 << ":";
            }

            (*out) << ((m_x==1 && src1or2==1) ? ab[!m_s] : ab[m_s]) << m_src1 << ",";

            if(optype & 0x2){
                (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
            }

            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

            if(optype & 0x1){
                (*out) << ab[m_s] << m_dst+1 << ":";
            }

            (*out) << ab[m_s] << m_dst;
        }
#endif
}

#if 0
int tms320C6x_instruction :: iprint_shift(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2, char optype)
{
	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") <<"\t\t";

        if(optype & 0x2){
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
        }
        (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

        if(optype & 0x4){
            (*out) << ((m_x==1 && src1or2==1) ? ab[!m_s] : ab[m_s]) << m_src1+1 << ":";
        }
        (*out) << ((m_x==1 && src1or2==1) ? ab[!m_s] : ab[m_s]) << m_src1 << ",";

        if(optype & 0x1){
            (*out) << ab[m_s] << m_dst+1 << ":";
        }
        (*out) << ab[m_s] << m_dst;
}
#endif

int tms320C6x_instruction :: transcode_cstregops(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2, char optype)
{
        int opcode_index = ir_instr_t :: get_opcode_index(inst);

        pir_instr->set_opcode(opcode_index);
        pir_instr->set_nm_operands(3);
        pir_instr->set_mode(m_mode);

        pir_instr->set_optype(0, OPTYPE_UCST, 32);

        if(optype & 0x2)
            pir_instr->set_optype(1, OPTYPE_UREG, 64);
        else
            pir_instr->set_optype(1, OPTYPE_UREG, 32);

        if(optype & 0x1)
            pir_instr->set_optype(2, OPTYPE_UREG, 64);
        else
            pir_instr->set_optype(2, OPTYPE_UREG, 32);

        pir_instr->set_operand(0, m_src1);
        pir_instr->set_operand(1, (REG_BANK_SIZE * m_x) + m_src2);
        pir_instr->set_operand(2, (REG_BANK_SIZE * m_s) + m_dst);

        return (0);

#if 0
	char ab[3] = "AB";

        if((strcmp(inst, "ADD") == 0) && m_src1 == 0)
        {
            (*out) << "MV" << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";

            if(optype & 0x2)
                (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

            if(optype & 0x1)
                (*out) << ab[m_s] << m_dst+1 << ":";
            (*out) << ab[m_s] << m_dst;
        }
        else if((strcmp(inst, "ADD") == 0) && ((m_src1 >> 8) & 0xFFFFFF))
        {
            (*out) << "SUB" << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";

            if(optype & 0x2)
                (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

            (*out) << hex << "0x" << (-m_src1) << dec << ",";

            if(optype & 0x1)
                (*out) << ab[m_s] << m_dst+1 << ":";
            (*out) << ab[m_s] << m_dst;
        }
        else if ((strcmp(inst, "CMPLTU") == 0))
        {
            (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
                   << hex << "0x" << m_src1 << dec << ",";

            if(optype & 0x2)
                (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

            if(optype & 0x1)
                (*out) << ab[m_s] << m_dst+1 << ":";
            (*out) << ab[m_s] << m_dst;
        }
        else if((strcmp(inst, "SUB") == 0) && (m_src1 == 0))
        {
            (*out) << "NEG" << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";

            if(optype & 0x2)
                (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

            if(optype & 0x1)
                (*out) << ab[m_s] << m_dst+1 << ":";
            (*out) << ab[m_s] << m_dst;
        }
        else if((strcmp(inst, "XOR") == 0) && (m_src1 == -1))
        {
            (*out) << "NOT" << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";

            if(optype & 0x2)
                (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

            if(optype & 0x1)
                (*out) << ab[m_s] << m_dst+1 << ":";
            (*out) << ab[m_s] << m_dst;
        }
        else
        {
            (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";

            (*out) << dec << m_src1 << ",";

            if(optype & 0x2)
                (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

            if(optype & 0x1)
                (*out) << ab[m_s] << m_dst+1 << ":";
            (*out) << ab[m_s] << m_dst;
        }
#endif
}

#if 0
int tms320C6x_instruction :: iprintc1_shift(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2, char optype)
{
	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";

        if(optype & 0x2){
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
        }
        (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

        (*out) << hex << "0x" << m_src1 << dec << ",";

        if(optype & 0x1){
            (*out) << ab[m_s] << m_dst+1 << ":";
        }
        (*out) << ab[m_s] << m_dst;
}

int tms320C6x_instruction :: iprintc2(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2)
{
	char ab[3] = "AB";

	(*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
               << ((m_x==1 && src1or2==1) ? ab[!m_s] : ab[m_s])
               << m_src1 << ", " << m_src2 << ", " << ab[m_s] << m_dst;
}
#endif

int tms320C6x_instruction :: transcode_ld(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
        int opcode_index = ir_instr_t :: get_opcode_index(inst);

        pir_instr->set_opcode(opcode_index);
        pir_instr->set_nm_operands(3);
        pir_instr->set_mode(m_mode);

        pir_instr->set_optype(0, OPTYPE_UREG, 32);
        pir_instr->set_optype(1, OPTYPE_UREG, 32);
        pir_instr->set_optype(2, OPTYPE_UREG, 32);

        pir_instr->set_operand(0, (REG_BANK_SIZE * m_x) + m_src1);
        pir_instr->set_operand(1, (REG_BANK_SIZE * m_x) + m_src2);
        pir_instr->set_operand(2, (REG_BANK_SIZE * m_s) + m_dst);

        return (0); 
#if 0
        char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t";

        switch (m_mode){
            case 0x5:
                    /* {+baseR[offsetR], dst} */
                    (*out) << "*+" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0x4:
                    /* {-baseR[offsetR], dst} */
                    (*out) << "*-" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xd:
                    /* {++baseR[offsetR], dst} */
                    (*out) << "*++" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xc:
                    /* {--baseR[offsetR], dst} */
                    (*out) << "*--" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xf:
                    /* {baseR++[offsetR], dst} */
                    (*out) << "*" << ab[m_x] << m_src2 << "++" << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xe:
                    /* {baseR--[offsetR], dst} */
                    (*out) << "*" << ab[m_x] << m_src2 << "--" << "[" << ab[m_x] << m_src1 << "]";
                    break;
        }

        (*out) << ","<< ab[m_s] << m_dst;
#endif
}

#if 0
int tms320C6x_instruction :: iprint_rev(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
	char ab[3] = "AB";

        if((strcmp(inst, "SUB") == 0) && (m_src1 == m_src2)){
            (*out) << "ZERO" << "." << unit << m_s+1 << "\t\t"
                   << ab[m_s] << m_dst;
        }
        else
        {
            (*out) << inst << "." << unit << m_s+1 << "\t\t"
                   << ab[m_s] << m_src2 << ","
                   << ab[m_s] << m_src1 << ","
                   << ab[m_s] << m_dst;
        }
}

int tms320C6x_instruction :: iprint_revc(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
	char ab[3] = "AB";

        if((strcmp(inst, "ADD") == 0) && m_src1 == 0)
        {
            (*out) << "MV" << "." << unit << m_s+1 << "\t\t"
                   << ab[m_s] << m_src2 << ","
                   << ab[m_s] << m_dst;
        }
        else
        {
            (*out) << inst << "." << unit << m_s+1 << "\t\t"
                   << ab[m_s] << m_src2 << ","
                   << hex << "0x" << m_src1 << dec << ","
                   << ab[m_s] << m_dst;
        }
}
#endif

int tms320C6x_instruction :: transcode_ldc(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
        int opcode_index = ir_instr_t :: get_opcode_index(inst);

        pir_instr->set_opcode(opcode_index);
        pir_instr->set_nm_operands(3);
        pir_instr->set_mode(m_mode);
        
        pir_instr->set_optype(0, OPTYPE_UCST, 32);
        pir_instr->set_optype(1, OPTYPE_UREG, 32);
        pir_instr->set_optype(2, OPTYPE_UREG, 32);

        pir_instr->set_operand(0, m_src1);
        pir_instr->set_operand(1, (REG_BANK_SIZE * m_x) + m_src2);
        pir_instr->set_operand(2, (REG_BANK_SIZE * m_s) + m_dst);

#if 0
	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t";
        switch (m_mode){
            case 0x1:
                    /* {+baseR[offset], dst} */
                    (*out) << "*+" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0x0:
                    /* {-baseR[offset], dst} */
                    (*out) << "*-" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0x9:
                    /* {++baseR[offset], dst} */
                    (*out) << "*++" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0x8:
                    /* {--baseR[offset], dst} */
                    (*out) << "*--" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0xb:
                    /* {baseR++[offset], dst} */
                    (*out) << "*" << ab[m_x] << m_src2 << "++" << "[" << m_src1 << "]";
                    break;

            case 0xa:
                    /* {baseR--[offset], dst} */
                    (*out) << "*" << ab[m_x] << m_src2 << "--" << "[" << m_src1 << "]";
                    break;
        }

        (*out) << "," << ab[m_s] << m_dst;
#endif
        
        return (0); 
}

#if 0
int tms320C6x_instruction :: iprintldc_offset(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{

	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t"
               << "*+" << ab[m_x] << m_src2 << "[" << m_src1 << "]"
               << "," << ab[m_s] << m_dst;
}

int tms320C6x_instruction :: iprintst(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t"
               << ab[m_s] << m_dst << ",*";

        switch (m_mode){
            case 0x5:
                    /* {+baseR[offsetR], dst} */
                    (*out) << "+" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0x4:
                    /* {-baseR[offsetR], dst} */
                    (*out) << "-" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xd:
                    /* {++baseR[offsetR], dst} */
                    (*out) << "++" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xc:
                    /* {--baseR[offsetR], dst} */
                    (*out) << "--" << ab[m_x] << m_src2 << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xf:
                    /* {baseR++[offsetR], dst} */
                    (*out) << ab[m_x] << m_src2 << "++" << "[" << ab[m_x] << m_src1 << "]";
                    break;

            case 0xe:
                    /* {baseR--[offsetR], dst} */
                    (*out) << ab[m_x] << m_src2 << "--" << "[" << ab[m_x] << m_src1 << "]";
                    break;
        }
}

int tms320C6x_instruction :: iprintstc(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t"
               << ab[m_s] << m_dst << ",*";

        switch (m_mode){
            case 0x1:
                    /* {+baseR[offset], dst} */
                    (*out) << "+" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0x0:
                    /* {-baseR[offset], dst} */
                    (*out) << "-" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0x9:
                    /* {++baseR[offset], dst} */
                    (*out) << "++" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0x8:
                    /* {--baseR[offset], dst} */
                    (*out) << "--" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
                    break;

            case 0xb:
                    /* {baseR++[offset], dst} */
                    (*out) << ab[m_x] << m_src2 << "++" << "[" << m_src1 << "]";
                    break;

            case 0xa:
                    /* {baseR--[offset], dst} */
                    (*out) << ab[m_x] << m_src2 << "--" << "[" << m_src1 << "]";
                    break;
        }
}

int tms320C6x_instruction :: iprintstc_offset(ir_instr_t * pir_instr, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t"
               << ab[m_s] << m_dst << ",*"
               << "+" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
}

int tms320C6x_instruction :: iprint4(ir_instr_t * pir_instr, const char *inst, int32_t csta, int32_t cstb,
		int32_t m_src2, int32_t m_dst, const char *unit, uint8_t m_s, char m_x,
		char src1or2)
{
	char ab[3] = "AB";

	(*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
               << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ","
               << csta << "," << cstb << ","
               << ab[m_s] << m_dst;
}

int tms320C6x_instruction :: iprint2(ir_instr_t * pir_instr, const char *inst, int32_t m_src2, int32_t m_dst,
		const char *unit, uint8_t m_s, char m_x, char src1or2, char optype) {
	char ab[3] = "AB";

	(*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";
        if(optype & 0x2){
            (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2+1 << ":";
        }
        (*out) << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ",";

        if(optype & 0x1){
            (*out) << ab[m_s] << m_dst+1 << ":";
        }
        (*out) << ab[m_s] << m_dst;
}

int tms320C6x_instruction :: iprint2mvc(ir_instr_t * pir_instr, const char *inst, int32_t m_src2, int32_t m_dst,
		const char *unit, uint8_t m_s, char m_x, char src1or2) {
	char ab[3] = "AB";
        char ctrl_reg[8][5] = {"AMR", "CSR", "ISR", "ICR", "IER", "ISTP", "IRP", "NRP"};
        //PCE1   = 16 GFPGFR = 24       // Ignored for the moment

        if(src1or2 == 0)
        {
            (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
                   << ctrl_reg[m_src2] << ","
                   << ab[m_s] << m_dst;
        }
        else if(src1or2 == 2)
        {
            (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
                   << ab[m_s] << m_src2 << ","
                   << ctrl_reg[m_dst];
        }
}

int tms320C6x_instruction :: iprint2c(ir_instr_t * pir_instr, const char *inst, int32_t m_src2, int32_t m_dst,
		const char *unit, uint8_t m_s, char m_x, char src1or2)
{
	char ab[3] = "AB";

        if(strcmp("MVK", inst) == 0 || strcmp("MVKH", inst) == 0)
        {
            (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t";

            if((m_src2 >> 16) == 0x0000 && ((m_src2 >> 12) & 0x8))
                (*out) << FMT_INT << ((m_src2 << 16) >> 16) << dec << "," << ab[m_s] << m_dst;
            else
                (*out) << FMT_SHORT << m_src2 << dec << "," << ab[m_s] << m_dst;
        }
        else
        {
            int16_t signed_src2 = m_src2;

            (*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
                   << signed_src2 << dec << "," << ab[m_s] << m_dst;
        }
}

int tms320C6x_instruction :: iprint_breg(ir_instr_t * pir_instr, const char *inst, int32_t m_dst, const char *unit,
		uint8_t m_s, char m_x, char src1or2)
{
	char ab[3] = "AB";

	(*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
               << (m_x ? ab[!m_s] : ab[m_s]) << m_dst;
}

#endif

int tms320C6x_instruction :: transcode_bcond(ir_instr_t * pir_instr, const char *inst, int32_t bcond_disp)
{
    int opcode_index = ir_instr_t :: get_opcode_index(inst);

    pir_instr->set_opcode(opcode_index);
    pir_instr->set_nm_operands(1);

    pir_instr->set_optype(0, OPTYPE_UCST, 23);
    pir_instr->set_operand(0, bcond_disp);

    return (0); 
    /*
    (*out) << inst << "." << unit << m_s+1 << "\t\t"
           << "0x" << hex << setw(6) << bcond_disp << dec;
    */
}


#if 0
int tms320C6x_instruction :: iprint0(ir_instr_t * pir_instr, const char *inst)
{
	(*out) << inst;
}

int tms320C6x_instruction :: iprint01(ir_instr_t * pir_instr, const char *inst, uint32_t num)
{
    if(num > 1)
	(*out) << inst << "\t\t" << num;
    else
        (*out) << inst;
}
#endif

