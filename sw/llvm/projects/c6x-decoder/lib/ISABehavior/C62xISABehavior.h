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

typedef struct C62x_DelayTable_Queue
{
    C62x_DelayTable_Node_t      * m_head_node;
    C62x_DelayTable_Node_t      * m_tail_node;
    uint32_t                      m_num_busy_nodes;
    uint32_t                      m_max_busy_nodes;
} C62x_DelayTable_Queue_t;
/* LLVM Type ... { { i16, i32, \2 }*, { i16, i32, \2 }*, i32, i32 } */

typedef struct C62x_Processor_State
{
    uint64_t                      m_curr_cpu_cycle;
    uint32_t                    * p_pc;
    uint32_t                      m_register[C62X_REG_BANKS * C62X_REGS_PER_BANK];
    C62x_DelayTable_Queue_t       m_delay_table[C62X_MAX_DELAY_SLOTS + 1];
} C62x_Proc_State_t;
/* LLVM Type ... { i64, i32*, [48 x i32], [6 x { { i16, i32, \2 }*, { i16, i32, \2 }*, i32, i32 }] } */

typedef enum ReturnStatus
{
    OK = 0,
    ERROR = 1,
    WAIT_FOR_INTERRUPT = 2
} ReturnStatus_t;

#define REG_PC_INDEX (REG_BANK_C * C62X_REGS_PER_BANK) + REG_PCE1
#define REG_AMR_INDEX (REG_BANK_C * C62X_REGS_PER_BANK) + REG_AMR

#endif	/* C62X_PROCESSOR_H */
