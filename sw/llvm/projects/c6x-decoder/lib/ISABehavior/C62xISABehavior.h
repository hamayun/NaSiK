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

#define ENABLE_TRACE

#ifdef ENABLE_TRACE
#define TRACE_PRINT(fmt, args...)                               \
    do { fprintf(stderr, fmt, ##args); } while (0)
#else
#define TRACE_PRINT(fmt, args...) do {} while(0)
#endif

typedef struct C62x_DelayTable_Node
{
    uint16_t                      m_reg_id;
    uint32_t                      m_value;
    struct C62x_DelayTable_Node * m_next_node;
} C62x_DelayTable_Node_t;
/* LLVM Type ... { i16, i32, \2 } */

typedef struct C62x_Delay_Queue
{
    C62x_DelayTable_Node_t      * m_head_node;
    C62x_DelayTable_Node_t      * m_tail_node;
    uint32_t                      m_num_busy_nodes;
    uint32_t                      m_max_busy_nodes;
} C62x_Delay_Queue_t;
/* LLVM Type ... { { i16, i32, \2 }*, { i16, i32, \2 }*, i32, i32 } */

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

typedef struct C62x_Processor_State
{
    uint64_t                      m_curr_cpu_cycle;
    uint32_t                    * p_pc;
    uint32_t                      m_register[C62X_REG_BANKS * C62X_REGS_PER_BANK];
    C62x_Delay_Queue_t            m_delay_q[C62X_MAX_DELAY_SLOTS + 1];
    C62x_MWB_Queue_t              m_mwback_q[C62X_MAX_DELAY_SLOTS + 1];
} C62x_Proc_State_t;
/* LLVM Type ... { i64, i32*, [48 x i32], [6 x { { i16, i32, \2 }*, { i16, i32, \2 }*, i32, i32 }] } */

typedef enum ReturnStatus
{
    OK = 0,
    ERROR = 1,
    WAIT_FOR_INTERRUPT = 2
} ReturnStatus_t;

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

#endif	/* C62X_PROCESSOR_H */
