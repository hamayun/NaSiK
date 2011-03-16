
/* -*- c++ -*-
 *
 * SOCLIB_LGPL_HEADER_BEGIN
 *
 * This file is part of SoCLib, GNU LGPLv2.1.
 *
 * SoCLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; version 2.1 of the License.
 *
 * SoCLib is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SoCLib; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * SOCLIB_LGPL_HEADER_END
 *
 * TMS320C6X Instruction Set Simulator for the TMS320C6X processor core
 * developed for the SocLib Projet
 *
 * Copyright (C) IRISA/INRIA, 2008
 *         Francois Charot <charot@irisa.fr>
 *
 *
 * Maintainer: charot
 *
 * Functional description:
 * The following files:
 * 		tms320c62.h
 *              tms320c62.cpp
 *              tms320c62_instructions.cpp
 *              tms320c62_decoding.cpp
 *
 * define the Instruction Set Simulator for the TMS320C62 processor.
 *
 *
 */

/*
 * File:   tms320C6x_instruction.cpp
 * Author: hamayun 
 *
 * Created on August 3, 2010, 4:27 PM
 * Adapted from tms320c62_decoding.cpp
 */

#include "tms320C6x_instruction.h"
#include "block_packets.h"
#include <iostream>
#include <iomanip>

bool tms320C6x_instruction :: is_label()
{
    static coff_symbol_table    * psymtab = NULL;
    char                        * plabel = NULL;

    if(!psymtab)
    {
        psymtab = get_parent()->get_parent()->get_parent()->get_symbol_table();
        if(!psymtab)
        {
            DOUT << "Error: Getting Symbol Table Pointer" << endl;
            return false;
        }
    }

    plabel = psymtab->get_label(p_raw_instr->get_vir_addrs());
    if(plabel)
        return true;
    else
        return false;
}

int tms320C6x_instruction :: decode (raw_instr_t *ptr_instr)
{
    uint32_t instr = ptr_instr->get_value();
    p_raw_instr = ptr_instr; 

    m_creg  = (instr >> 29) & 0x07;
    m_z     = (instr >> 28) & 0x01;
    m_dst   = (instr >> 23) & 0x1f;
    m_src2  = (instr >> 18) & 0x1f;
    m_src1  = (instr >> 13) & 0x1f;
    m_x     = 0;
    m_s     = (instr >> 1) & 0x01;

    if(get_unit_op_x(instr)){
        DOUT << "Error: Decoding Instruction" << endl;
        return(-1);
    }

    //find constants
    m_sicst1 = ((int32_t) m_src1 << 27) >> 27; /*taking care of sign bit extn*/
    m_uicst1 = (uint32_t) m_src1;
    m_slcst1 = ((int64_t) m_src1 << 59) >> 59; /*taking care of sign bit extn*/
    m_ulcst1 = (uint64_t) m_src1;

    if((m_op == 0x00 || m_op == 0x01 || m_op == 0x02 || m_op == 0x03 ||
        m_op == 0x04 || m_op == 0x05 || m_op == 0x06 || m_op == 0x07) &&
        m_unit == DUNIT_LDSTBASEROFFSET)
    {
        m_y     = instr >> 7 & 0x1;
        m_mode  = instr >> 9 & 0xf;
    }
    else if((m_op == 0x01 || m_op == 0x02 || m_op == 0x03) &&
             m_unit == DUNIT_LDSTOFFSET)
    {
        m_y     = instr >> 7 & 0x1;
        m_uitmp = instr >> 8 & 0x7fff;
        m_uitmp = m_uitmp << 0;
    }
    else if((m_op == 0x00 || m_op == 0x04 || m_op == 0x05) &&
             m_unit == DUNIT_LDSTOFFSET)
    {
        m_y     = instr >> 7 & 0x1;
        m_uitmp = instr >> 8 & 0x7fff;
        m_uitmp = m_uitmp << 1;
    }
    else if((m_op == 0x06 || m_op == 0x07) &&
             m_unit == DUNIT_LDSTOFFSET)
    {
        m_y     = instr >> 7 & 0x1;
        m_uitmp = instr >> 8 & 0x7fff;  // The 15-Bit Unsigned Constant Offset
        //m_uitmp = m_uitmp << 2;       // In Reality this offset should be Shifted Left before Memory Access
    }

    if(m_op == 0x00 && m_unit == IDLEUNIT)
    {
        m_nop_cnt = ((instr >> 13) & 0x000f) + 1;
    }

    if(m_op == 0x00 && m_unit == SUNIT_ADDK)
    {
        m_addk_const = ((instr >> 7 & 0x0000ffff) << 16) >> 16;
    }

    if(m_unit == SUNIT_IMMED)
    {
        m_immed_const1 = ((instr >> 13) & 0x1f);
        m_immed_const2 = ((instr >> 8) & 0x1f);
    }

    if(m_unit == SUNIT_MVK)
    {
        m_mvk_type          = (instr >> 6 & 0x01);
        m_mvk_unsign_const  = (((instr >> 7) & 0x0000ffff) << 16);
        m_mvk_sign_const    = (((instr >> 7) & 0x0000ffff) << 16) >> 16;
    }

    if(m_unit == SUNIT_BCOND)
    {
        uint32_t    fetch_pkt_addr = get_parent()->get_parent()->get_fetch_pkt_address();

        m_bcond_disp        = (((instr >> 7) & 0x001fffff) << 11) >> 11;
        m_bcond_disp        = (m_bcond_disp << 2) + fetch_pkt_addr;
        m_bcond_disp        = m_bcond_disp & 0x7fffff;              // Discard bits higher than 23rd bit. 
    }
    
    return (0);
}

void tms320C6x_instruction :: print(ostream *out)
{
    out->setf(ios::basefield, ios::dec);
    print_creg(out);
    
    switch(m_unit)
    {
        case LUNIT:
            print_lunit(out);
            break;

        case MUNIT:
        /*case IDLEUNIT:*/
            if(m_op == 0x00 || m_op == 0x78)
                print_idle(out);
            else
                print_munit(out);
            break;

        case DUNIT:
            print_dunit(out); 
            break;

        case DUNIT_LDSTOFFSET:
            print_dunit_ldstoffset(out); 
            break;

        case DUNIT_LDSTBASEROFFSET:
            print_dunit_ldstbaseroffset(out); 
            break;

        case SUNIT:
            print_sunit(out); 
            break;

        case SUNIT_ADDK:
            print_sunit_addk(out);
            break;

        case SUNIT_IMMED:
            print_sunit_immed(out); 
            break;

        case SUNIT_MVK:
            print_sunit_mvk(out); 
            break;

        case SUNIT_BCOND:
            print_sunit_bcond(out);
            break;

        default:
            DOUT << "Error: Unknown Functional Unit Type" << endl;
            break;
    }
}

// Private Functions
// Decoding Helper Functions
int tms320C6x_instruction :: get_unit_op_x(uint32_t instr)
{
    if (getUnit_3bit(instr) == LUNIT)
    {
            // .L unit instruction
            m_x = (instr >> 12) & 0x01;
            m_op = (instr >> 5) & 0x7f;
            m_unit = LUNIT;
    }
    else if (getUnit_5bit(instr) == MUNIT)
    {
            // .M unit instruction
            m_x = (instr >> 12) & 0x01;
            m_op = (instr >> 7) & 0x1f;
            m_unit = MUNIT;
    }
    else if (getUnit_5bit(instr) == DUNIT)
    {
            // .D unit instruction
            m_op = (instr >> 7) & 0x3f;
            m_unit = DUNIT;
    }
    else if (getUnit_2bit(instr) == DUNIT_LDSTOFFSET)
    {
            // .D unit load/store
            m_op = (instr >> 4) & 0x07;
            m_unit = DUNIT_LDSTOFFSET;
    }
    else if (getUnit_2bit(instr) == DUNIT_LDSTBASEROFFSET)
    {
            // .D unit load/store with baseR/offsetR specified
            m_op = (instr >> 4) & 0x07;
            m_unit = DUNIT_LDSTBASEROFFSET;
    }
    else if (getUnit_4bit(instr) == SUNIT)
    {
            // .S unit instruction
            m_x = (instr >> 12) & 0x01;
            m_op = (instr >> 6) & 0x3f;
            m_unit = SUNIT;
    }
    else if (getUnit_5bit(instr) == SUNIT_ADDK)
    {
            // .S unit ADDK instruction
            m_op = 0; //nothing specified
            m_unit = SUNIT_ADDK;
    }
    else if (getUnit_4bit(instr) == SUNIT_IMMED)
    {
            // .S unit Field operations (immediate forms)
            m_op = (instr >> 6) & 0x03;
            m_unit = SUNIT_IMMED;
    }
    else if (getUnit_4bit(instr) == SUNIT_MVK)
    {
            // .S unit MVK
            m_op = 0; // nothing specified
            m_unit = SUNIT_MVK;
    }
    else if (getUnit_5bit(instr) == SUNIT_BCOND)
    {
            // .S unit Bcond disp
            m_op = 0; // nothing specified
            m_unit = SUNIT_BCOND;
    }
    else if (getUnit_11bit(instr) == NOP)
    {
            // NOP instruction
            m_op = 0;
            m_unit = 0;
    }
    else if (getUnit_16bit(instr) == IDLEINST)
    {
            m_op = IDLEOP;
            m_unit = IDLEUNIT;
    }
    else
    {
            /* unknown instruction kind */
            DOUT << "Error: Unknown Instruction Type" << endl;
            return(-1);
    }

    return(0); 
}

// Print Helper Functions
void tms320C6x_instruction :: print_creg(ostream *out)
{

	if (m_creg == 0 && m_z == 0) {
		(*out) << "       ";
	} else {
		if (m_z == 1)
			(*out) << " [!";
		else
			(*out) << " [ ";
		if (m_creg == CREG_B0)
			(*out) << "B0] ";
		else if (m_creg == CREG_B1)
			(*out) << "B1] ";
		else if (m_creg == CREG_B2)
			(*out) << "B2] ";
		else if (m_creg == CREG_A1)
			(*out) << "A1] ";
		else if (m_creg == CREG_A2)
			(*out) << "A2] ";
	}
}

void tms320C6x_instruction :: print_lunit(ostream *out)
{
    switch(m_op)
    {
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
            iprint(out, "ADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x23 :
            /*ADD sint, xsint, slong*/
            iprint(out, "ADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x2b :
            /*ADDU usint, xusint, uslong*/
            iprint(out, "ADDU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x21 :
            /*ADD xsint, slong, slong*/
            iprint(out, "ADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILL);
            break;

        case 0x29 :
            /*ADDU xusint, uslong, uslong*/
            iprint(out, "ADDU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILL);
            break;

        case 0x02 :
            /*ADD scst5, xsint, sint*/
            iprintc1(out, "ADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x20 :
            /*ADD scst5, slong, slong*/
            iprintc1(out, "ADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILL);
            break;

        case 0x7b :
            /*AND usint, xusint, usint*/
            iprint(out, "AND", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x7a :
            /*AND scst5, xusint, usint*/
            iprintc1(out, "AND", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x53 :
            /*CMPEQ sint, xsint, usint*/
            iprint(out, "CMPEQ", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x52 :
            /*CMPEQ scst5, xsint, usint*/
            iprintc1(out, "CMPEQ", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x51 :
            /*CMPEQ xsint, slong, usint*/
            iprint(out, "CMPEQ", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x50 :
            /*CMPEQ scst5, slong, usint*/
            iprintc1(out, "CMPEQ", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x47 :
            /*CMPGT sint, xsint, usint*/
            iprint(out, "CMPGT", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x46 :
            /*CMPGT scst5, xsint, usint*/
            iprintc1(out, "CMPGT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x45 :
            /*CMPGT xsint, slong, usint*/
            iprint(out, "CMPGT", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x44 :
            /*CMPGT scst5, slong, usint*/
            iprintc1(out, "CMPGT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x4f :
            /*CMPGTU usint, xusint, usint*/
            iprint(out, "CMPGTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x4e :
            /*CMPGTU ucst5, xusint, usint*/
            iprintc1(out, "CMPGTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x4d :
            /*CMPGTU xusint, uslong, usint*/
            iprint(out, "CMPGTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x4c :
            /*CMPGTU ucst5, uslong, usint*/
            iprintc1(out, "CMPGTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x57 :
            /*CMPLT sint, xsint, usint*/
            iprint(out, "CMPLT", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x56 :
            /*CMPLT scst5, xsint, usint*/
            iprintc1(out, "CMPLT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x55 :
            /*CMPLT xsint, slong, usint*/
            iprint(out, "CMPLT", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x54 :
            /*CMPLT scst5, slong, usint*/
            iprintc1(out, "CMPLT", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x5f :
            /*CMPLTU usint, xusint, usint*/
            iprint(out, "CMPLTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x5e :
            /*CMPLTU ucst5, xusint, usint*/
            iprintc1(out, "CMPLTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x5d :
            /*CMPLTU xusint, uslong, usint*/
            iprint(out, "CMPLTU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILI);
            break;

        case 0x5c :
            /*CMPLTU ucst5, uslong, usint*/
            iprintc1(out, "CMPLTU", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x6b :
            /*LMBD - Left Most Bit Detection usint, xusint, usint*/
            iprint(out, "LMBD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x6a :
            /*LMBD - Left Most Bit Detection ucst5, xusint, usint*/
            iprintc1(out, "LMBD", m_uicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x63 :
            /*NORM Normalize Integer, # of redundant sign bits are found xsint, usint*/
            iprint2(out, "NORM", m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x60 :
            /*NORM Normalize Integer, # of redundant sign bits are found slong, usint*/
            iprint2(out, "NORM", m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILI);
            break;

        case 0x7f :
            /*Bitwise OR usint, xusint, usint*/
            iprint(out, "OR", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x7e :
            /*Bitwise OR scst5, xusint, usint*/
            iprintc1(out, "OR", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x13 :
            /*SADD Integer addition with saturation to result size sint, xsint, sint*/
            iprint(out, "SADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x31 :
            /*SADD Integer addition with saturation to result size xsint, slong, slong*/
            iprint(out, "SADD", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_ILL);
            break;

        case 0x12 :
            /*SADD Integer addition with saturation to result size scst5, xsint, sint*/
            iprintc1(out, "SADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x30 :
            /*SADD Integer addition with saturation to result size scst5, slong, slong*/
            iprintc1(out, "SADD", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILL);
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
            iprint(out, "SSUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x1f :
            /*SSUB Integer addition with saturation to result size xsint, sint, sint*/
            iprint(out, "SSUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_III);
            break;

        case 0x0e :
            /*SSUB Integer addition with saturation to result size scst5, xsint, sint*/
            iprintc1(out, "SSUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x2c :
            /*SSUB Integer addition with saturation to result size scst5, slong, slong*/
            iprintc1(out, "SSUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 0, OPTYPE_ILL);
            break;

        case 0x07 :
            /*SUB without saturation sint, xsint, sint*/
            iprint(out, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x17 :
            /*SUB without saturation xsint, sint, sint*/
            iprint(out, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_III);
            break;

        case 0x27 :
            /*SUB without saturation sint, xsint, slong*/
            iprint(out, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x37 :
            /*SUB without saturation xsint, sint, slong*/
            iprint(out, "SUB", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_IIL);
            break;

        case 0x2f :
            /*SUBU without saturation usint, xusint, uslong*/
            iprint(out, "SUBU", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_IIL);
            break;

        case 0x3f :
            /*SUBU without saturation xusint, usint, uslong*/
            iprint(out, "SUBU", m_src1, m_src2, m_dst, "L", m_s, m_x, 1, OPTYPE_IIL);
            break;

        case 0x06 :
            /*SUB without saturation scst5, xsint, sint*/
            iprintc1(out, "SUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x24 :
            /*SUB without saturation scst5, slong, slong*/
            iprintc1(out, "SUB", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_ILL);
            break;

        case 0x4b :
            /*SUBC Conditional Integer Subtract and Shift - used for division usint, xusint, usint*/
            iprint(out, "SUBC", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x6f :
            /*XOR usint, xusint, usint*/
            iprint(out, "XOR", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        case 0x6e :
            /*XOR scst5, xusint, usint*/
            iprintc1(out, "XOR", m_sicst1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
            break;

        default:
            DOUT << "Error: Unknown Instruction for LUNIT" << endl;
            break;
    }
}

void tms320C6x_instruction :: print_sunit(ostream *out)
{
    switch(m_op)
    {
 	case 0x07 :
            /*ADD sint, xsint, sint*/
            iprint(out, "ADD", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x06 :
            /*ADD scst5, xsint, sint*/
            iprintc1(out, "ADD", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x01 :
            /*ADD2 sint, xsint, sint*/
            iprint(out, "ADD2", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1f :
            /*AND usint, xusint, usint*/
            iprint(out, "AND", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1e :
            /*AND scst5, xusint, usint*/
            iprintc1(out, "AND", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
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
            iprint(out, "CLR", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;
            
	case 0x2f :
            /*EXT Extract & Sign Extend a bit field*/
            /*usint, xsint, sint*/
            iprint(out, "EXT", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;
            
	case 0x2b :
            /*EXTU Extract & Zero Extend a bit field*/
            /*usint, xusint, usint*/
            iprint(out, "EXTU", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
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
            iprint(out, "OR", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;
            
	case 0x1a :
            /*Bitwise OR scst5, xusint, usint*/
            iprintc1(out, "OR", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;
            
        case 0x3b :
            /*SET Set Bit Fields of m_src2 whose bounds are given by 5-9 & 0-4 of m_src1*/
            /*usint, xusint, usint*/
            iprint(out, "SET", m_src1, m_src2, m_dst, "L", m_s, m_x, 2, OPTYPE_III);
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
            iprint(out, "SSHL", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x22 :
            /*SSHL Shift Left with Saturation ucst5, xsint, sint*/
            iprintc1(out, "SSHL", m_uicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x17 :
            /*SUB without saturation sint, xsint, sint*/
            iprint(out, "SUB", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x16 :
            /*SUB without saturation scst5, xsint, sint*/
            iprintc1(out, "SUB", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x11 :
            /*SUB2 Subtractions of lower and upper halfs sint, xsint, sint*/
            iprint(out, "SUB2", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0b :
            /*XOR usint, xusint, usint*/
            iprint(out, "XOR", m_src1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0a :
            /*XOR scst5, xusint, usint*/
            iprintc1(out, "XOR", m_sicst1, m_src2, m_dst, "S", m_s, m_x, 2, OPTYPE_III);
            break;
            
        default:
            DOUT << "Error: Unknown Instruction for SUNIT" << endl;
            break;
    }
}

void tms320C6x_instruction :: print_munit(ostream *out)
{
    switch(m_op)
    {
	case 0x19 :
            /*MPY Integer Multiply 16lsb x 16lsb; slsb16, xslsb16, sint*/
            iprint(out, "MPY", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1f :
            /*MPYU Integer Multiply 16lsb x 16lsb; ulsb16, xulsb16, usint*/
            iprint(out, "MPYU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1d :
            /*MPYUS Integer Multiply 16lsb x 16lsb; ulsb16, xslsb16, sint*/
            iprint(out, "MPYUS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1b :
            /*MPYSU Integer Multiply 16lsb x 16lsb; slsb16, xulsb16, sint*/
            iprint(out, "MPYSU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x18 :
            /*MPY Integer Multiply 16lsb x 16lsb; scst5, xslsb16, sint*/
            iprintc1(out, "MPY", m_sicst1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x1e :
            /*MPYSU Integer Multiply 16lsb x 16lsb; scst5, xulsb16, sint*/
            iprintc1(out, "MPYSU", m_sicst1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x01 :
            /*MPYH Integer Multiply 16msb x 16msb; smsb16, xsmsb16, sint*/
            iprint(out, "MPYH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x07 :
            /*MPYHU Integer Multiply 16msb x 16msb; umsb16, xumsb16, usint*/
            iprint(out, "MPYHU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x05 :
            /*MPYHUS Integer Multiply 16msb x 16msb; umsb16, xsmsb16, sint*/
            iprint(out, "MPYHUS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x03 :
            /*MPYHSU Integer Multiply 16msb x 16msb; smsb16, xumsb16, sint*/
            iprint(out, "MPYHSU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x09 :
            /*MPYHL Integer Multiply 16msb x 16lsb; smsb16, xslsb16, sint*/
            iprint(out, "MPYHL", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0f :
            /*MPYHLU Integer Multiply 16msb x 16lsb; umsb16, xulsb16, usint*/
            iprint(out, "MPYHLU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0d :
            /*MPYHULS Integer Multiply 16msb x 16lsb; umsb16, xslsb16, sint*/
            iprint(out, "MPYHULS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0b :
            /*MPYHSLU Integer Multiply 16msb x 16lsb; smsb16, xulsb16, sint*/
            iprint(out, "MPYHSLU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x11 :
            /*MPYLH Integer Multiply 16lsb x 16msb; slsb16, xsmsb16, sint*/
            iprint(out, "MPYLH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x17 :
            /*MPYLHU Integer Multiply 16lsb x 16msb; ulsb16, xumsb16, usint*/
            iprint(out, "MPYLHU", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x15 :
            /*MPYLUHS Integer Multiply 16lsb x 16msb ulsb16, xsmsb16, sint*/
            iprint(out, "MPYLUHS", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

#if 0
        //TODO: How to differentiate this following case from similar one above?
	case 0x11 :
            /*MPYLSHU Integer Multiply 16lsb x 16msb slsb16, xumsb16, sint*/
            iprint(out, "MPYLSHU {slsb16, xumsb16, sint}", m_src1, m_src2, m_dst, "M", m_s, m_x, 2);
            break;
#endif
        case 0x1a :
            /*SMPY slsb16, xslsb16, sint*/
            iprint(out, "SMPY", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x0a :
            /*SMPYHL smsb16, xslsb16, sint*/
            iprint(out, "SMPYHL", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x12 :
            /*SMPYLH slsb16, xsmsb16, sint*/
            iprint(out, "SMPYLH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

	case 0x02 :
            /*SMPYH smsb16, xsmsb16, sint*/
            iprint(out, "SMPYH", m_src1, m_src2, m_dst, "M", m_s, m_x, 2, OPTYPE_III);
            break;

        default:
            DOUT << "Error: Unknown Instruction for MUNIT" << endl;
            break;
    }
}

void tms320C6x_instruction :: print_dunit(ostream *out)
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

void tms320C6x_instruction :: print_dunit_ldstbaseroffset(ostream *out)
{
    switch(m_op){
	case 0x02 :
            /*LDB Load Byte*/
            switch (m_mode) {
            case 0x5:
                    /* {+baseR[offsetR], dst} */
                    iprintld(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    /* {-baseR[offsetR], dst} */
                    iprintld(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    /* {++baseR[offsetR], dst} */
                    iprintld(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    /* {--baseR[offsetR], dst} */
                    iprintld(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    /* {baseR++[offsetR], dst} */
                    iprintld(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    /* {baseR--[offsetR], dst} */
                    iprintld(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    /* {+baseR[offset], dst} */
                    iprintldc(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    /* {-baseR[offset], dst} */
                    iprintldc(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    /* {++baseR[offset], dst} */
                    iprintldc(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    /* {--baseR[offset], dst} */
                    iprintldc(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    /* {baseR++[offset], dst} */
                    iprintldc(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    /* {baseR--[offset], dst} */
                    iprintldc(out, "LDB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintld(out, "LDB **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;

	case 0x01 :
            /*LDBU Load Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintld(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintld(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintld(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintld(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintld(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintld(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintldc(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintldc(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintldc(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintldc(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintldc(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintldc(out, "LDBU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintld(out, "LDBU **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;

            }
            break;

	case 0x04 :
            /*LDH Load Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintld(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintld(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintld(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintld(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintld(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintld(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintldc(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintldc(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintldc(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintldc(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintldc(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintldc(out, "LDH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintld(out, "LDH **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;

            }
            break;

	case 0x00 :
            /*LDHU Load Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintld(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintld(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintld(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintld(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintld(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintld(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintldc(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintldc(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintldc(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintldc(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintldc(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintldc(out, "LDHU", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintld(out, "LDHU **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;

            }
            break;

	case 0x06 :
            /*LDW Load Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintld(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintld(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintld(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintld(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintld(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintld(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintldc(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintldc(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintldc(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintldc(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintldc(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintldc(out, "LDW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintld(out, "LDW **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;

	case 0x03 :
            /*STB Store Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintst(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintst(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintst(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintst(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintst(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintst(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintstc(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintstc(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintstc(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintstc(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintstc(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintstc(out, "STB", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintst(out, "STB **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;

	case 0x05 :
            /*STH Store Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintst(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintst(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintst(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintst(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintst(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintst(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintstc(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintstc(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintstc(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintstc(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintstc(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintstc(out, "STH", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintst(out, "STH **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;

	case 0x07 :
            /*STW Store Byte*/
            switch (m_mode) {
            case 0x5:
                    iprintst(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x4:
                    iprintst(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xd:
                    iprintst(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xc:
                    iprintst(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xf:
                    iprintst(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xe:
                    iprintst(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x1:
                    iprintstc(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x0:
                    iprintstc(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x9:
                    iprintstc(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0x8:
                    iprintstc(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xb:
                    iprintstc(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            case 0xa:
                    iprintstc(out, "STW", m_src1, m_src2, m_dst, "D", m_s, m_y);
                    break;

            default:
                    iprintst(out, "STW **unknown mode**", 0, 0, 0, "D", 0, 0);
                    break;
            }
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl; 
            break;
    }
}

void tms320C6x_instruction :: print_dunit_ldstoffset(ostream *out)
{
    switch(m_op)
    {
	case 0x00 :
            /*LDHU Load HalfByte ucst15, m_dst; {+baseR[offset], dst} */
            iprintldc(out, "LDHU", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x01 :
            /*LDBU Load Byte ucst15, m_dst; {+baseR[offset], dst} */
            iprintldc(out, "LDBU", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x02 :
            /*LDB Load Byte ucst15, m_dst; {+baseR[offset], dst} */
            iprintldc(out, "LDB", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x03 :
            /*STB Store Byte; {src, +baseR[offset]}*/
            iprintstc(out, "STB", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x04 :
            /*LDH Load HlafByte ucst15, m_dst; {+baseR[offset], dst}*/
            iprintldc(out, "LDH", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x05 :
            /*STH Store Halfword; {src, +baseR[offset]}*/
            iprintstc(out, "STH", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x06 :
            /*LDW Load Word ucst15, m_dst; {+baseR[offset], dst}*/
            iprintldc_offset(out, "LDW", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

	case 0x07 :
            /*STW Store Word; {src, +baseR[offset]}*/
            iprintstc_offset(out, "STW", m_uitmp, (m_y==0) ? 14 : 15, m_dst, "D", m_s, sideB);
            break;

        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }
}

void tms320C6x_instruction :: print_sunit_addk(ostream *out)
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

void tms320C6x_instruction :: print_sunit_immed(ostream *out)
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

void tms320C6x_instruction :: print_sunit_mvk(ostream *out)
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

void tms320C6x_instruction :: print_sunit_bcond(ostream *out)
{
    switch(m_op)
    {
        case 0x00:
            /*Branch Using a Displacement; {scst21}*/
            iprint1c(out, "B", m_bcond_disp, "S", m_s, m_x, 0);
            break;
            
        default:
            DOUT << "Error: Unknown Instruction Type" << endl;
            break;
    }
}

void tms320C6x_instruction :: print_idle(ostream *out)
{
    if(m_op == 0x78)
        iprint0(out, "IDLE");
    else if(m_op == 0x00)
        iprint01(out, "NOP", (uint32_t)m_nop_cnt);
    else
        DOUT << "Error: Unknown Instruction Type" << endl; 
}

void tms320C6x_instruction :: iprint(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2, char optype)
{
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
}

void tms320C6x_instruction :: iprint_shift(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
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

void tms320C6x_instruction :: iprintc1(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2, char optype)
{
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
}

void tms320C6x_instruction :: iprintc1_shift(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
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

void tms320C6x_instruction :: iprintc2(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, char m_x, char src1or2)
{
	char ab[3] = "AB";

	(*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
               << ((m_x==1 && src1or2==1) ? ab[!m_s] : ab[m_s])
               << m_src1 << ", " << m_src2 << ", " << ab[m_s] << m_dst;
}

void tms320C6x_instruction :: iprintld(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{

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
}

void tms320C6x_instruction :: iprint_rev(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
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

void tms320C6x_instruction :: iprint_revc(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
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

void tms320C6x_instruction :: iprintldc(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{

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
}

void tms320C6x_instruction :: iprintldc_offset(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{

	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t"
               << "*+" << ab[m_x] << m_src2 << "[" << m_src1 << "]"
               << "," << ab[m_s] << m_dst;
}

void tms320C6x_instruction :: iprintst(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
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

void tms320C6x_instruction :: iprintstc(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
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

void tms320C6x_instruction :: iprintstc_offset(ostream *out, const char *inst, int32_t m_src1, int32_t m_src2,
		int32_t m_dst, const char *unit, uint8_t m_s, uint8_t m_x)
{
	char ab[3] = "AB";

        (*out) << inst << "." << unit << m_x+1 << "T" << m_s+1 << "\t\t"
               << ab[m_s] << m_dst << ",*"
               << "+" << ab[m_x] << m_src2 << "[" << m_src1 << "]";
}

void tms320C6x_instruction :: iprint4(ostream *out, const char *inst, int32_t csta, int32_t cstb,
		int32_t m_src2, int32_t m_dst, const char *unit, uint8_t m_s, char m_x,
		char src1or2)
{
	char ab[3] = "AB";

	(*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
               << ((m_x==1 && src1or2==2) ? ab[!m_s] : ab[m_s]) << m_src2 << ","
               << csta << "," << cstb << ","
               << ab[m_s] << m_dst;
}

void tms320C6x_instruction :: iprint2(ostream *out, const char *inst, int32_t m_src2, int32_t m_dst,
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

void tms320C6x_instruction :: iprint2mvc(ostream *out, const char *inst, int32_t m_src2, int32_t m_dst,
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

void tms320C6x_instruction :: iprint2c(ostream *out, const char *inst, int32_t m_src2, int32_t m_dst,
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

void tms320C6x_instruction :: iprint_breg(ostream *out, const char *inst, int32_t m_dst, const char *unit,
		uint8_t m_s, char m_x, char src1or2)
{
	char ab[3] = "AB";

	(*out) << inst << "." << unit << m_s+1 << (m_x?"X":"") << "\t\t"
               << (m_x ? ab[!m_s] : ab[m_s]) << m_dst; 
}

void tms320C6x_instruction :: iprint1c(ostream *out, const char *inst, int32_t bcond_disp, const char *unit,
		char m_s, char m_x, char src1or2)
{
    fetch_packet      * p_fetpkt       = get_parent()->get_parent();
    if(p_fetpkt == NULL)
    {
        cout << "Fetch Packet Parent NULL";
        return;
    }
    coff_symbol_table * p_symtab       = p_fetpkt->get_parent()->get_symbol_table();
    coff_symtab_entry * p_symtab_entry = p_symtab->get_symtab_entry_byval(bcond_disp);
    char * ptr_str = NULL; 

    if(p_symtab_entry)
    {
        ptr_str = p_symtab_entry->get_name_str();

        if(memcmp(ptr_str, ".text:", 6) == 0){
            ptr_str += 6;
        }

        if(p_symtab->is_hidden_label(ptr_str)){
            ptr_str = NULL;
        }
    }

    if(ptr_str)
    {
        int    cur_pc       = p_fetpkt->get_fetch_pkt_address();
        int    cur_offset   = bcond_disp - cur_pc;
        
        (*out) << inst << "." << unit << m_s+1 << "\t\t"
               << ptr_str << " (PC" << (cur_offset >= 0 ? "+" :"") << dec << cur_offset
               << " = 0x" << hex << setw(8) << bcond_disp << dec << ")";
    }
    else
    {
        (*out) << inst << "." << unit << m_s+1 << "\t\t"
               << "0x" << hex << setw(6) << bcond_disp << dec;
    }
}

void tms320C6x_instruction :: iprint0(ostream *out, const char *inst)
{
	(*out) << inst;
}

void tms320C6x_instruction :: iprint01(ostream *out, const char *inst, uint32_t num)
{
    if(num > 1)
	(*out) << inst << "\t\t" << num;
    else
        (*out) << inst;
}
