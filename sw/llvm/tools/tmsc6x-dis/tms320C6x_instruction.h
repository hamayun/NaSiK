
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
 * File:   tms320C6x_instruction.h
 * Author: hamayun
 *
 * Created on August 3, 2010, 4:27 PM
 * Adapted from tms320c62_decoding.cpp
 */

#ifndef _TMS320C6X_INSTRUCTION_H
#define	_TMS320C6X_INSTRUCTION_H

#include "coff_reader.h"
#include "unified_instruction.h"
#include "inttypes.h"

// define condition register codes
#define CREG_B0                 0x01
#define CREG_B1                 0x02
#define CREG_B2                 0x03
#define CREG_A1                 0x04
#define CREG_A2                 0x05

// allows choosing between the two register files
#define sideA                   0
#define sideB                   1

// define different execution unit types
#define IDLEUNIT                0x00
#define MUNIT                   0x00
#define DUNIT_LDSTBASEROFFSET   0x01
#define SUNIT_IMMED             0x02
#define DUNIT_LDSTOFFSET        0x03
#define SUNIT_BCOND             0x04
#define LUNIT                   0x06
#define SUNIT                   0x08
#define SUNIT_MVK               0x0a
#define DUNIT                   0x10
#define SUNIT_ADDK              0x14

//#define NOP1                  0x00000000
//#define NOP1_PARALLEL         0x00000001

#define IDLEOP                  0x78
#define IDLEINST                0x7800
#define NOP                     0x000

// serve to extract information from instructions
#define getUnit_2bit(inst)      (inst >> 2 & 0x03)
#define getUnit_3bit(inst)      (inst >> 2 & 0x07)
#define getUnit_4bit(inst)      (inst >> 2 & 0x0f)
#define getUnit_5bit(inst)      (inst >> 2 & 0x1f)
#define getUnit_11bit(inst)     (inst >> 2 & 0x07ff)
#define getUnit_16bit(inst)     (inst >> 2 & 0xffff)

#define OPTYPE_III              0           // int, int, int
#define OPTYPE_IIL              1           // int, int, long
#define OPTYPE_ILI              2           // int, long, int
#define OPTYPE_ILL              3           // int, long, long
#define OPTYPE_LIL              5           // long, int, long
#define OPTYPE_LLL              7           // long, long, long

class execute_packet;

class tms320C6x_instruction
{
private:
        execute_packet *p_parent;
        raw_instr_t    *p_raw_instr;
        
	char            m_creg;
        char            m_z;
        char            m_op;
        char            m_unit;
        char            m_dst;
        char            m_src1; 
        char            m_src2; 
        char            m_x; 
        char            m_s; 
        uint32_t        m_uicst1;
        uint32_t        m_uitmp;
        int32_t         m_sicst1;
        int64_t         m_slcst1;
	uint64_t        m_ulcst1;
	char            m_y;
        char            m_mode;

        // Variables for some specific instructions
        char            m_nop_cnt;
        int16_t         m_addk_const;
        char            m_immed_const1;
        char            m_immed_const2;
        char            m_mvk_type;
        int32_t         m_mvk_unsign_const;
        uint16_t        m_mvk_sign_const;
        int32_t         m_bcond_disp;

        // Variables for Meta Information about this Instruction
        bool            m_is_br_target; 

public: 
    tms320C6x_instruction(){
        m_op = 0;
        m_unit = 0;
        m_is_br_target = false; 
    }
    
    ~tms320C6x_instruction(){}

    void set_parent(execute_packet * parent)
    {
        p_parent = parent;
    }

    execute_packet * get_parent()
    {
        return(p_parent); 
    }
    
    int decode(raw_instr_t *ptr_instr);
    
    ir_instr_t * transcode();
    
    void print(ostream *out);

    bool is_label(); 

    void set_branch_target()
    {
        m_is_br_target = true; 
    }

    bool is_branch_target()
    {
        return(m_is_br_target); 
    }

    bool is_branch_instr()
    {
        if(is_direct_branch() || is_register_branch())
            return (true);
        else
            return(false);
    }

    bool is_direct_branch()
    {
        if(m_unit == SUNIT_BCOND)
            return (true);
        else
            return(false);
    }

    bool is_register_branch()
    {
        if(m_unit == SUNIT && (m_op == 0x03 || m_op == 0x0d))
            return (true);
        else
            return(false);
    }

    int32_t get_branch_address()
    {
        return(m_bcond_disp); 
    }

private:
    // Transcode Helper Functions
    int transcode_opcode_operands(ir_instr_t * pir_instr);
    
    int transcode_lunit(ir_instr_t * pir_instr);
    /*
    int transcode_sunit(ir_instr_t * pir_instr);
    int transcode_munit(ir_instr_t * pir_instr);
    int transcode_dunit(ir_instr_t * pir_instr);
    */
    int transcode_dunit_ldstbaseroffset(ir_instr_t * pir_instr);
    int transcode_dunit_ldstoffset(ir_instr_t * pir_instr);
    /*
    int transcode_sunit_addk(ir_instr_t * pir_instr);
    int transcode_sunit_immed(ir_instr_t * pir_instr);
    int transcode_sunit_mvk(ir_instr_t * pir_instr);
    */
    int transcode_sunit_bcond(ir_instr_t * pir_instr);
    //int transcode_idle(ir_instr_t * pir_instr);

    int  transcode_regops(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                 int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);
    /*
    int  iprint_shift(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                 int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);
    */
    int  transcode_cstregops(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);
    /*
    int  iprintc1_shift(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);
    int  iprintc2(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, char x, char src1or2);
     */
    int  transcode_ld(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);
    /*
    int  iprint_rev(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);
    int  iprint_revc(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);
    */
    int  transcode_ldc(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);
    /*
    int  iprintldc_offset(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);
    int  iprintst(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);
    int  iprintstc(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);
    int  iprintstc_offset(ir_instr_t * pir_instr, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);
    int  iprint4(ir_instr_t * pir_instr, const char *inst, int32_t csta, int32_t cstb,
                  int32_t src2, int32_t dst, const char *unit, uint8_t s, char x,
                  char src1or2);
    int  iprint2(ir_instr_t * pir_instr, const char *inst, int32_t src2, int32_t dst,
                  const char *unit, uint8_t s, char x, char src1or2, char optype);
    int  iprint2mvc(ir_instr_t * pir_instr, const char *inst, int32_t src2, int32_t dst,
                  const char *unit, uint8_t s, char x, char src1or2);
    int  iprint2c(ir_instr_t * pir_instr, const char *inst, int32_t src2, int32_t dst,
                   const char *unit, uint8_t s, char x, char src1or2);
    int  iprint_breg(ir_instr_t * pir_instr, const char *inst, int32_t dst, const char *unit,
                  uint8_t s, char x, char src1or2);
    */
    int  transcode_bcond(ir_instr_t * pir_instr, const char *inst, int32_t bcond_disp);
    /*
    int  iprint0(ir_instr_t * pir_instr, const char *inst);
    int  iprint01(ir_instr_t * pir_instr, const char *inst, uint32_t num);
    */



    // Decode Helper Functions
    int get_unit_op_x(uint32_t instr);

    // Print Helper Functions
    void print_creg(ostream *out);
    void print_lunit(ostream *out);
    void print_sunit(ostream *out); 
    void print_munit(ostream *out);
    void print_dunit(ostream *out);
    void print_dunit_ldstbaseroffset(ostream *out);
    void print_dunit_ldstoffset(ostream *out);
    void print_sunit_addk(ostream *out);
    void print_sunit_immed(ostream *out);
    void print_sunit_mvk(ostream *out);
    void print_sunit_bcond(ostream *out); 
    void print_idle(ostream *out); 

    void  iprint(ostream *out, const char *inst, int32_t src1, int32_t src2,
                 int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);

    void  iprint_shift(ostream *out, const char *inst, int32_t src1, int32_t src2,
                 int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);

    void  iprintc1(ostream *out, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);

    void  iprintc1_shift(ostream *out, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, char x, char src1or2, char optype);

    void  iprintc2(ostream *out, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, char x, char src1or2);

    void  iprintld(ostream *out, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprint_rev(ostream *out, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprint_revc(ostream *out, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprintldc(ostream *out, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprintldc_offset(ostream *out, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprintst(ostream *out, const char *inst, int32_t src1, int32_t src2,
                   int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprintstc(ostream *out, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprintstc_offset(ostream *out, const char *inst, int32_t src1, int32_t src2,
                    int32_t dst, const char *unit, uint8_t s, uint8_t x);

    void  iprint4(ostream *out, const char *inst, int32_t csta, int32_t cstb,
                  int32_t src2, int32_t dst, const char *unit, uint8_t s, char x,
                  char src1or2);

    void  iprint2(ostream *out, const char *inst, int32_t src2, int32_t dst,
                  const char *unit, uint8_t s, char x, char src1or2, char optype);

    void  iprint2mvc(ostream *out, const char *inst, int32_t src2, int32_t dst,
                  const char *unit, uint8_t s, char x, char src1or2);

    void  iprint2c(ostream *out, const char *inst, int32_t src2, int32_t dst,
                   const char *unit, uint8_t s, char x, char src1or2);

    void  iprint_breg(ostream *out, const char *inst, int32_t dst, const char *unit,
                  uint8_t s, char x, char src1or2);

    void  iprint1c(ostream *out, const char *inst, int32_t dst, const char *unit,
                   char s, char x, char src1or2);

    void  iprint0(ostream *out, const char *inst);

    void  iprint01(ostream *out, const char *inst, uint32_t num);
};

typedef tms320C6x_instruction       dec_instr_t;

#endif	/* _TMS320C6X_INSTRUCTION_H */

