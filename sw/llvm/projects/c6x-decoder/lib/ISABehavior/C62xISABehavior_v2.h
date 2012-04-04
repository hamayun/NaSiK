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

#ifndef C62X_PROCESSOR_H
#define	C62X_PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "C62xCommon.h"

#define QUEUE_BASED_DREGS
//#define DELAYED_MWBS
#define PRINT_CYCLES

#define MEMIO_LEVEL 1
#define INFO_LEVEL 2
#define VERBOSE_LEVEL 3

//#define ENABLE_TRACE VERBOSE_LEVEL

#define LOG(format, ...)                                      \
    fprintf (stderr, format, ## __VA_ARGS__);

#ifndef ENABLE_TRACE
#define ENABLE_TRACE 0
#define TRACE(level, format, ...) do {} while(0)
#else
#if ENABLE_TRACE < MEMIO_LEVEL || ENABLE_TRACE > VERBOSE_LEVEL
#error __FUNCTION ", " __LINE__ ": Invalid Trace Level"
#endif

#if (ENABLE_TRACE >= MEMIO_LEVEL)
#define LOG_MEMIO_LEVEL(format, ...) LOG(format, ## __VA_ARGS__)
#else
#define LOG_MEMIO_LEVEL(format, ...)
#endif

#if (ENABLE_TRACE >= INFO_LEVEL)
#define LOG_INFO_LEVEL(format, ...) LOG(format, ## __VA_ARGS__)
#else
#define LOG_INFO_LEVEL(format, ...)
#endif

#if (ENABLE_TRACE == VERBOSE_LEVEL)
#define LOG_VERBOSE_LEVEL(format, ...) LOG(format, ## __VA_ARGS__)
#else
#define LOG_VERBOSE_LEVEL(format, ...)
#endif

#define TRACE(level, format, ...) LOG_##level (format, ## __VA_ARGS__)
#endif

#ifdef QUEUE_BASED_DREGS
typedef struct C62x_Delay_Node
{
    uint16_t                 m_reg_id;
    uint32_t                 m_value;
    struct C62x_Delay_Node * m_next_node;
} C62x_Delay_Node_t;
/* LLVM Type ... { i16, i32, \2 } */

typedef struct C62x_Delay_Queue
{
    C62x_Delay_Node_t      * m_head_node;
    C62x_Delay_Node_t      * m_tail_node;
} C62x_Delay_Queue_t;
/* LLVM Type ... { { i16, i32, \2 }*, { i16, i32, \2 }*} */
#else
#define DELAY_QUEUE_SIZE (C62X_MAX_DELAY_SLOTS * FETCH_PACKET_SIZE * 2)
typedef struct C62x_Delay_Node
{
    uint16_t m_reg_id;
    uint32_t m_value;
} C62x_Delay_Node_t;

typedef struct C62x_Delay_Queue
{
    uint32_t m_head_idx;
    uint32_t m_tail_idx;
    C62x_Delay_Node_t m_nodes[DELAY_QUEUE_SIZE];
} C62x_Delay_Queue_t;
#endif

#define MAX_RESULT_SIZE         3
#define RESULT_LREG_IDX 0
#define RESULT_HREG_IDX 1
#define RESULT_SREG_IDX 2

typedef enum C62x_Result_Type
{
    C62X_NO_RESULT = 0,
    C62X_REGISTER = 1,    /* Single Register Result */
    C62X_MULTIREG = 2,    /* Multiple Registers Result */
    C62X_SREG_UPDATE = 4, /* Register Update as Side Effect to Normal Instruction Result */
} C62x_Result_Type_t;

typedef struct C62x_Result
{
    C62x_Result_Type_t       m_type;
    uint16_t                 m_reg_id[MAX_RESULT_SIZE];
    uint32_t                 m_value [MAX_RESULT_SIZE];
} C62x_Result_t;

#define SAVE_REGISTER_RESULT(result, idx_rd, val_rd)                           \
        result->m_type |= C62X_REGISTER;                                       \
        result->m_reg_id[RESULT_LREG_IDX] = idx_rd;                            \
        result->m_value [RESULT_LREG_IDX] = val_rd;

#define SAVE_MULTIREGISTER_RESULT(result, idx_rdh, val_rdh, idx_rdl, val_rdl)  \
        result->m_type |= C62X_MULTIREG;                                       \
        result->m_reg_id[RESULT_LREG_IDX] = idx_rdl;                           \
        result->m_value [RESULT_LREG_IDX] = val_rdl;                           \
        result->m_reg_id[RESULT_HREG_IDX] = idx_rdh;                           \
        result->m_value [RESULT_HREG_IDX] = val_rdh;

#define SAVE_SREG_UPDATE(result, idx_rd, val_rd)                               \
        result->m_type |= C62X_SREG_UPDATE;                                    \
        result->m_reg_id[RESULT_SREG_IDX] = idx_rd;                            \
        result->m_value [RESULT_SREG_IDX] = val_rd;

typedef enum C62xAlignment
{
    BYTE_ALIGN  = 0,
    HWORD_ALIGN = 1,
    WORD_ALIGN  = 2
} C62xAlignment_t;

typedef enum C62xMWB_Size
{
    MWB_BYTE  = 1,
    MWB_HWORD = 2,
    MWB_WORD  = 4
} C62xMWB_Size_t;

/* Memory Write-Back Node */
typedef struct C62x_MWBack_Node
{
    C62xMWB_Size_t                m_size;     /* MWB_BYTE, MWB_HWORD or MWB_WORD */
    uint32_t                      m_addr;
    uint32_t                      m_value;
    struct C62x_MWBack_Node     * m_next_node;
} C62x_MWBack_Node_t;

typedef struct C62x_MWB_Queue
{
    C62x_MWBack_Node_t          * m_head_node;
    C62x_MWBack_Node_t          * m_tail_node;
    uint32_t                      m_is_empty;
} C62x_MWB_Queue_t;

typedef struct C62x_DSPState
{
    uint64_t                      m_curr_cycle;
    uint32_t                      m_reg[C62X_REG_BANKS * C62X_REGS_PER_BANK];
    C62x_Delay_Queue_t            m_delay_q[C62X_MAX_DELAY_SLOTS + 1];
#ifdef DELAYED_MWBS
    C62x_MWB_Queue_t              m_mwback_q[C62X_MAX_DELAY_SLOTS + 1];
#endif
} C62x_DSPState_t;
/* LLVM Type ... { i64, i32*, [48 x i32], [6 x { { i16, i32, \2 }*, { i16, i32, \2 }*, i32, i32 } ... ] } */

typedef struct address_entry
{
    uint32_t m_address;
    int (*func_address)(C62x_DSPState_t * p_state);
} address_entry_t;

#if 0
typedef enum ReturnStatus
{
    OK = 0,
    ERROR = 1,
    WAIT_FOR_INTERRUPT = 2,
    PC_UPDATED = 4,
} ReturnStatus_t;
#endif

// Return Type and Codes for ISA Functions
typedef uint32_t ReturnStatus_t;

#define OK 0
#define ERROR 1
#define WAIT_FOR_INTERRUPT 2
#define PC_UPDATED 4

#define REG_SP_INDEX   (REG_BANK_B * C62X_REGS_PER_BANK) + 15
#define REG_AMR_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_AMR
#define REG_CSR_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_CSR
#define REG_ISR_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_ISR
#define REG_IFR_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_ISR  /* REG_IFR ???? */
#define REG_ICR_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_ICR
#define REG_IER_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_IER
#define REG_ISTP_INDEX (REG_BANK_C * C62X_REGS_PER_BANK) + REG_ISTP
#define REG_IRP_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_IRP
#define REG_NRP_INDEX  (REG_BANK_C * C62X_REGS_PER_BANK) + REG_NRP
#define REG_PC_INDEX   (REG_BANK_C * C62X_REGS_PER_BANK) + REG_PCE1

#define C6X40_TO_U64(msb32,lsb32) ((((uint64_t) msb32) << 32) | lsb32)

#define C6X40_TO_S64(msb32,lsb32)                                              \
    (((int64_t)((msb32 & 0x80) ? (msb32 | 0xFFFFFF00) : (msb32 & 0x000000FF)) << 32) | lsb32)

#define C6XSC5_TO_S64(scst5)                                                   \
    ((int64_t)((scst5 & 0x10) ? (scst5 | 0xFFFFFFFFFFFFFFE0) : (scst5 & 0x1F)))

#define C6XSC5_TO_S32(scst5)                                                   \
    ((int32_t)((scst5 & 0x10) ? (scst5 | 0xFFFFFFE0) : (scst5 & 0x1F)))

#define C6XSC5_TO_S16(scst5)                                                   \
    ((int16_t)((scst5 & 0x10) ? (scst5 | 0xFFE0) : (scst5 & 0x1F)))

#define C6XSCST23_TO_S32(scst23)                                               \
    ((int32_t)((scst23 & 0x400000) ? (scst23 | 0xFF800000) : (scst23 & 0x7FFFFF)))

#define GET_BITS_5_TO_9(u32) ((u32 & 0x3E0) >> 5)
#define GET_BITS_0_TO_3(u32) (u32 & 0xF)
#define GET_BITS_0_TO_4(u32) (u32 & 0x1F)
#define GET_BITS_0_TO_5(u32) (u32 & 0x3F)

#define U64_TO_C6XMSB12(u64) (u64 >> 32 & 0xFF)
#define U64_TO_C6XLSB32(u64) (u64 & 0xFFFFFFFF)

#define GET_MSB16(u32) ((u32 >> 16) & 0xFFFF)
#define GET_LSB16(u32) (u32 & 0xFFFF)

#define S8_TO_S32(s8) ((int32_t) (s8 & 0x80) ? (s8 | 0xFFFFFF00) : s8)
#define S16_TO_S32(s16) ((int32_t) (s16 & 0x8000) ? (s16 | 0xFFFF0000) : s16)

char BANKS[C62X_REG_BANKS];
char * REG(uint16_t idx);

#define BANKINDEX(idx) idx / C62X_REGS_PER_BANK
#define BANKNAME(idx)  BANKS[BANKINDEX(idx)]
#define RNUM(idx)      idx % C62X_REGS_PER_BANK

#define MAX_REG_PER_INSTR     10
#define MAX_REG_NAME_LEN      3

#define TEST_AGAIN() ASSERT(0, "Test This Instruction Again\n")

int32_t Init_DSP_State(C62x_DSPState_t * p_state);
int32_t Print_DSP_State(C62x_DSPState_t * p_state);
int32_t Init_Delay_Reg_Queue(C62x_Delay_Queue_t * delay_queue);
int32_t Init_MWB_Queue(C62x_MWB_Queue_t * mwb_queue);

#endif	/* C62X_PROCESSOR_H */
