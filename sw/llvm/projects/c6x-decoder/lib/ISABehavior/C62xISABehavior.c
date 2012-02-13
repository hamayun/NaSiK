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

#include "C62xISABehavior.h"
#include "stdio.h"

char BANKS[C62X_REG_BANKS];

#define BANKINDEX(idx) idx / C62X_REGS_PER_BANK
#define BANKNAME(idx)  BANKS[BANKINDEX(idx)]
#define RNUM(idx)      idx % C62X_REGS_PER_BANK

#define MAX_REG_PER_INSTR     10
#define MAX_REG_NAME_LEN      3

#define LDSTB_REG_INC_SIZE 4   /* For Use In Pre/Post Inc/Dec of Base Register */
#define LDSTH_REG_INC_SIZE 4   /* For Use In Pre/Post Inc/Dec of Base Register */
#define LDSTW_REG_INC_SIZE 4   /* For Use In Pre/Post Inc/Dec of Base Register */

char * BANKC_REGS[] = {"AMR", "CSR", "ISR", "ICR", "IER", "ISTP", "IRP", "NRP",
                       "C8", "C9", "C10", "C11", "C12", "C13", "C14", "PCE1"};

void Dump(char * rn, uint16_t sz)
{
    uint16_t idx = 0;
    printf("Dump: ");
    for(; idx < sz; idx++)
        printf("%x ", rn[idx]);
    printf("\n");
}


char * REG(uint16_t idx)
{
    /* In order to support multiple calls to this function
     * And to return a different char pointer for each call we use last_idx
     */
    static char name[MAX_REG_PER_INSTR][MAX_REG_NAME_LEN + 1];
    static uint16_t last_idx = 0;
    char * str = & name[last_idx][0];

    memset(str, 0x0, MAX_REG_NAME_LEN + 1);

    if(BANKINDEX(idx) == REG_BANK_C)
    {
        sprintf(str, "%s", BANKC_REGS[RNUM(idx)]);
    }
    else
    {
        sprintf(str, "%c%d", BANKNAME(idx), RNUM(idx));
    }

    last_idx = (last_idx + 1) % MAX_REG_PER_INSTR;
    return (str);
}

int32_t InitDelayTableQueue(C62x_DelayTable_Queue_t * delay_queue)
{
    C62x_DelayTable_Node_t * curr_node   = NULL;
    C62x_DelayTable_Node_t * prev_node   = NULL;
    uint32_t                 nodes_count = 0;

    delay_queue->m_head_node = NULL;
    delay_queue->m_tail_node = NULL;
    delay_queue->m_num_busy_nodes = 0;
    delay_queue->m_max_busy_nodes = 0;

    // Because Destination can be a Long Register in C62x: So 2 Times
    while(nodes_count < (C62X_MAX_DELAY_SLOTS * FETCH_PACKET_SIZE * 2))
    {
        curr_node = malloc(sizeof(C62x_DelayTable_Node_t));
        if(!curr_node)
        {
            printf("Error: Allocating Memory for Delay Table Node\n");
            return (-1);
        }

        if(!delay_queue->m_head_node)
        {
            delay_queue->m_head_node = curr_node;
            prev_node   = curr_node;
        }

        curr_node->m_next_node = delay_queue->m_head_node;
        prev_node->m_next_node = curr_node;
        prev_node = curr_node;
        curr_node = NULL;
        nodes_count++;
    }

    delay_queue->m_tail_node = delay_queue->m_head_node;
    return(0);
}

int32_t ProduceDelayRegister(C62x_DelayTable_Queue_t * delay_queue, uint16_t reg_id, uint32_t value)
{
    if(delay_queue->m_tail_node->m_next_node == delay_queue->m_head_node)
    {
        printf("Error: Delay Node Queue is Full\n");
        return (-1);
    }

    delay_queue->m_tail_node->m_reg_id = reg_id;
    delay_queue->m_tail_node->m_value = value;

    delay_queue->m_tail_node = delay_queue->m_tail_node->m_next_node;
    delay_queue->m_num_busy_nodes++;

    if(delay_queue->m_num_busy_nodes > delay_queue->m_max_busy_nodes)
        delay_queue->m_max_busy_nodes = delay_queue->m_num_busy_nodes;

    ASSERT(delay_queue->m_max_busy_nodes < (C62X_MAX_DELAY_SLOTS * FETCH_PACKET_SIZE * 2), "Maximum Busy Nodes Limit Crossed !!!");

    return (0);
}

C62x_DelayTable_Node_t * ConsumeDelayRegister(C62x_DelayTable_Queue_t * delay_queue)
{
    if(delay_queue->m_num_busy_nodes > 0)
    {
        if(delay_queue->m_tail_node == delay_queue->m_head_node)
        {
            printf("Delay Node Queue Counter vs Pointers Mismatch !!!\n");
            return (NULL);
        }

        C62x_DelayTable_Node_t * delay_register = delay_queue->m_head_node;
        delay_queue->m_head_node = delay_queue->m_head_node->m_next_node;
        delay_queue->m_num_busy_nodes--;

        return (delay_register);
    }

    return (NULL);
}

int32_t InitProcessorState(C62x_Proc_State_t * proc_state)
{
    uint32_t index;

    proc_state->m_curr_cpu_cycle = 0;
    proc_state->p_pc = & proc_state->m_register[REG_PC_INDEX];

    memset((void *) & proc_state->m_register[0], 0x0, sizeof(proc_state->m_register));

    for(index = 0; index < (C62X_MAX_DELAY_SLOTS + 1); index++)
    {
        if(InitDelayTableQueue(& proc_state->m_delay_table[index]))
        {
            printf("Error: Initializing Delay Table Queue [%d]\n", index);
            return (-1);
        }
    }

    for(index = 0; index < C62X_REG_BANKS; index++)
        BANKS[index] = 'A' + index;

    printf("(Parameterized) Processor State Initialized\n");
    return (0);
}

int32_t IncrementPC(C62x_Proc_State_t * proc_state, int32_t offset)
{
    * proc_state->p_pc += offset;
    return(* proc_state->p_pc);
}

int32_t SetPC(C62x_Proc_State_t * proc_state, int32_t abs_addr)
{
    * proc_state->p_pc = abs_addr;
    return(* proc_state->p_pc);
}

int32_t GetPC(C62x_Proc_State_t * proc_state)
{
    return(* proc_state->p_pc);
}

uint64_t IncrementCycles(C62x_Proc_State_t * proc_state)
{
    proc_state->m_curr_cpu_cycle++;
    return(proc_state->m_curr_cpu_cycle);
}

uint64_t GetCycles(C62x_Proc_State_t * proc_state)
{
    return(proc_state->m_curr_cpu_cycle);
}

int32_t UpdateRegisters(C62x_Proc_State_t * proc_state)
{
    C62x_DelayTable_Queue_t * delay_queue = & proc_state->m_delay_table[proc_state->m_curr_cpu_cycle % (C62X_MAX_DELAY_SLOTS + 1)];
    C62x_DelayTable_Node_t  * delay_reg = NULL;

    while((delay_reg = ConsumeDelayRegister(delay_queue)) != NULL)
    {
        //printf("Updating: [%02d]=%04x\n", delay_reg->m_reg_id, delay_reg->m_value);
        proc_state->m_register[delay_reg->m_reg_id] = delay_reg->m_value;
    }

    // TODO: If PC is Updated; Return Something Special Here
    return (0);
}

int32_t AddDelayedRegister(C62x_Proc_State_t * proc_state, uint16_t reg_id, uint32_t value, uint8_t delay_slots)
{
    uint32_t                  delay_queue_id = (proc_state->m_curr_cpu_cycle + delay_slots + 1) % (C62X_MAX_DELAY_SLOTS + 1);
    C62x_DelayTable_Queue_t * delay_queue    = & proc_state->m_delay_table[delay_queue_id];

    //printf("Add Register: [%4s]=%08x, +%d\n\n", REG(reg_id), value, delay_slots);
    ProduceDelayRegister(delay_queue, reg_id, value);

    return (0);
}

int32_t ShowProcessorState(C62x_Proc_State_t * proc_state)
{
    uint32_t reg_id, index;

    printf("Cycle: %016lld, PC = %08x, Registers:\n", proc_state->m_curr_cpu_cycle, *proc_state->p_pc);
    for(reg_id = 0; reg_id < (C62X_REG_BANKS * C62X_REGS_PER_BANK); reg_id++)
    {
        printf("%4s=%08x ", REG(reg_id), proc_state->m_register[reg_id]);
        if((reg_id + 1) % (C62X_REGS_PER_BANK/2) == 0) printf("\n");
    }

    for(index = 0; index < (C62X_MAX_DELAY_SLOTS + 1); index++)
    {
        C62x_DelayTable_Queue_t * delay_queue = & proc_state->m_delay_table[index];
        C62x_DelayTable_Node_t  * curr_node = delay_queue->m_head_node;

        if(proc_state->m_curr_cpu_cycle % (C62X_MAX_DELAY_SLOTS + 1) == index)
            printf("->");
        else
            printf("  ");

        printf("Queue[%d(Max=%02d)]: { ", index, delay_queue->m_max_busy_nodes);
        while(curr_node != delay_queue->m_tail_node)
        {
            printf("%4s=%08x ", REG(curr_node->m_reg_id), curr_node->m_value);
            curr_node = curr_node->m_next_node;
        }
        printf("}\n");
    }

    printf("\n");
    return (0);
}

uint8_t ExecuteDecision(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc)
{
    uint8_t do_execute = 1;       // By default; We Will execute this instruction.
    if(is_cond)
    {
        int32_t rc = proc_state->m_register[idx_rc];

        if(be_zero)
            do_execute = (rc == 0 ? 1 : 0);
        else
            do_execute = (rc != 0 ? 1 : 0);
    }
    return (do_execute);
}

/// ABS - Absolute value with saturation
ReturnStatus_t
C62xABS_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) proc_state->m_register[idx_ra];
        int32_t rd;

        if(ra >= 0)
            rd = ra;
        else if (ra < 0 && ra != 0x80000000)
            rd = -ra;
        else
            rd = 0x80000000;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tABS       %s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xABS_SR40_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t rah = (int32_t) proc_state->m_register[idx_rah];
        int32_t ral = (int32_t) proc_state->m_register[idx_ral];
        int64_t ra  = ((int64_t) rah) << 32 | ral;

        int64_t rd;

        if(ra >= 0)
            rd = ra;
        else if (ra < 0 && ra != 0x8000000000)
            rd = -ra;
        else
            rd = 0x8000000000;

        int32_t rdh = rd >> 32 & 0xFF;
        int32_t rdl = rd & 0xFFFFFFFF;

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%010x\tABS       %s:%s,%s:%s\n",
                GetPC(proc_state), REG(idx_rah), REG(idx_ral), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// ADD - Add two signed integers without saturation.
ReturnStatus_t
C62xADD_SR32_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) proc_state->m_register[idx_ra];
        int32_t rb = (int32_t) proc_state->m_register[idx_rb];
        int32_t rd = ra + rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tADD       %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SR32_SR32_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) proc_state->m_register[idx_ra];
        int32_t rb = (int32_t) proc_state->m_register[idx_rb];
        int64_t rd = ra + rb;

        int32_t rdh = rd >> 32 & 0xFF;
        int32_t rdl = rd & 0xFFFFFFFF;

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%08x\tADD       %s,%s,%s:%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SR32_SR40_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) proc_state->m_register[idx_ra];
        int32_t rbh = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl = (int32_t) proc_state->m_register[idx_rbl];

        int64_t rb  = ((int64_t) rbh) << 32 | rbl;
        int64_t rd  = ra + rb;

        int32_t rdh = rd >> 32 & 0xFF;
        int32_t rdl = rd & 0xFFFFFFFF;

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%08x\tADD       %s,%s:%s,%s:%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SC5_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int8_t cst5 = constant & 0x1F;
        int32_t rb  = (int32_t) proc_state->m_register[idx_rb];
        int32_t rd  = cst5 + rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tADD       0x%x,%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SC5_SR40_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int8_t cst5 = constant & 0x1F;
        int32_t rbh = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl = (int32_t) proc_state->m_register[idx_rbl];

        int64_t rb  = ((int64_t) rbh) << 32 | rbl;
        int64_t rd  = cst5 + rb;

        int32_t rdh = rd >> 32 & 0xFF;
        int32_t rdl = rd & 0xFFFFFFFF;

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%08x\tADD       0x%x,%s:%s,%s:%s\n",
                GetPC(proc_state), cst5, REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// ADDAB - Add using byte addressing mode of AMR
ReturnStatus_t
C62xADDAB_SR32_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "%s: Not Implemented !!!\n");
    return OK;
}

ReturnStatus_t
C62xADDAB_SC5_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "%s: Not Implemented !!!\n");
    return OK;
}

/// ADDAH - Add using halfword addressing mode of AMR
ReturnStatus_t
C62xADDAH_SR32_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "%s: Not Implemented !!!\n");
    return OK;
}

ReturnStatus_t
C62xADDAH_SC5_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "%s: Not Implemented !!!\n");
    return OK;
}

/// ADDAW - Add using word addressing mode of AMR
ReturnStatus_t
C62xADDAW_SR32_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "%s: Not Implemented !!!\n");
    return OK;
}

ReturnStatus_t
C62xADDAW_SC5_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "%s: Not Implemented !!!\n");
    return OK;
}

/// ADDK - Add signed 16-bit constant to register.
ReturnStatus_t
C62xADDK_SC16_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst16 = constant & 0xFFFF;
        int32_t rd    = cst16 + (int32_t) proc_state->m_register[idx_rd];

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tADDK      0x%x,%s\n",
                GetPC(proc_state), cst16, REG(idx_rd));
    }
    return OK;
}

/// ADDU - Add two unsinged integers without saturation.
ReturnStatus_t
C62xADDU_UR32_UR32_UR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb = (uint32_t) proc_state->m_register[idx_rb];
        uint64_t rd = ra + rb;

        uint32_t rdh = (rd >> 32) & 0x000000FF;
        uint32_t rdl = rd & 0xFFFFFFFF;

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%08x\tADDU      %s,%s,%s:%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xADDU_UR32_UR40_UR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rbh = (uint32_t) proc_state->m_register[idx_rbh];
        uint32_t rbl = (uint32_t) proc_state->m_register[idx_rbl];

        uint64_t rb  = ((int64_t) rbh) << 32 | rbl;
        uint64_t rd  = ra + rb;

        uint32_t rdh = (rd >> 32) & 0x000000FF;
        uint32_t rdl = rd & 0xFFFFFFFF;

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%08x\tADDU      %s,%s:%s,%s:%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// ADD2 - Add two 16-bit integers on upper and lower register halves.
ReturnStatus_t
C62xADD2_SR32_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) proc_state->m_register[idx_ra];
        int32_t rb  = (int32_t) proc_state->m_register[idx_rb];

        int16_t ra_msb16 = (ra >> 16) & 0xFFFF;
        int16_t ra_lsb16 = ra & 0xFFFF;

        int16_t rb_msb16 = (rb >> 16) & 0xFFFF;
        int16_t rb_lsb16 = rb & 0xFFFF;

        int32_t rd  = ((int32_t)(ra_msb16 + rb_msb16) << 16) | (ra_lsb16 + rb_lsb16);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tADD2      %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// AND - Bitwise AND
ReturnStatus_t
C62xAND_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd = ra & rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tAND       %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xAND_SC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst5 = (int16_t) constant & 0x1F;
        uint32_t rb  = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd  = cst5 & rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tAND       0x%x,%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// B - Branch using a displacement / Register / IRP / NRP
ReturnStatus_t
C62xB_SC21(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd      = constant & 0x1FFFFF;
        uint16_t idx_rd = REG_PC_INDEX;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tB         0x%x\n", GetPC(proc_state), rd);
    }
    return OK;
}

ReturnStatus_t
C62xB_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rd     = (uint32_t) proc_state->m_register[idx_ra];
        uint16_t idx_rd = REG_PC_INDEX;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tB         %s\n", GetPC(proc_state), REG(idx_ra));
    }
    return OK;
}

/// CLR - Clear a Bit Field
ReturnStatus_t
C62xCLR_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "Test This Instruction !!!");

    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];

        uint16_t csta = (uint16_t) (rb & 0x3E0) >> 5;   /* bits 5 to 9 */
        uint16_t cstb = (uint16_t) rb & 0x1F;           /* bits 0 to 4 */

        uint16_t rshift = cstb + 1;
        uint16_t lshift = 32 - csta;

        uint32_t ra_hi  = (ra >> rshift) << rshift;
        uint32_t ra_lo  = (ra << lshift) >> lshift;

        uint32_t rd = ra_hi | ra_lo;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCLR       %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCLR_UR32_UC5_UC5_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, int32_t constanta, int32_t constantb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "Test This Instruction !!!");
    
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) proc_state->m_register[idx_ra];

        uint16_t csta = (uint16_t) constanta;
        uint16_t cstb = (uint16_t) constantb;

        uint16_t rshift = cstb + 1;
        uint16_t lshift = 32 - csta;

        uint32_t ra_hi  = (ra >> rshift) << rshift;
        uint32_t ra_lo  = (ra << lshift) >> lshift;

        uint32_t rd = ra_hi | ra_lo;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCLR       %s,0x%x,0x%x,%s\n",
                GetPC(proc_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

/// CMPEQ - Compare for Equality; Signed Integer
ReturnStatus_t
C62xCMPEQ_SR32_SR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) proc_state->m_register[idx_ra];
        int32_t rb  = (int32_t) proc_state->m_register[idx_rb];
        uint32_t rd = (uint32_t)(ra == rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPEQ     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPEQ_SC5_SR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst5  = (int16_t) constant & 0x1F;
        int32_t rb    = (int32_t) proc_state->m_register[idx_rb];
        uint32_t rd   = (uint32_t)(cst5 == rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPEQ     0x%x,%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPEQ_SR32_SR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) proc_state->m_register[idx_ra];
        int32_t rbh  = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl  = (int32_t) proc_state->m_register[idx_rbl];
        int64_t rb   = ((int64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(ra == rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPEQ     %s,%s:%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPEQ_SC5_SR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst5 = (int16_t) constant & 0x1F;
        int32_t rbh  = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl  = (int32_t) proc_state->m_register[idx_rbl];
        int64_t rb   = ((int64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(cst5 == rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPEQ     0x%x,%s:%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPGT - Compare for Greater Than; Signed Integers
ReturnStatus_t
C62xCMPGT_SR32_SR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) proc_state->m_register[idx_ra];
        int32_t rb  = (int32_t) proc_state->m_register[idx_rb];
        uint32_t rd = (uint32_t)(ra > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGT     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGT_SC5_SR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst5  = (int16_t) constant & 0x1F;
        int32_t rb    = (int32_t) proc_state->m_register[idx_rb];
        uint32_t rd   = (uint32_t)(cst5 > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGT     0x%x,%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGT_SR32_SR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) proc_state->m_register[idx_ra];
        int32_t rbh  = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl  = (int32_t) proc_state->m_register[idx_rbl];
        int64_t rb   = ((int64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(ra > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGT     %s,%s:%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGT_SC5_SR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst5 = (int16_t) constant & 0x1F;
        int32_t rbh  = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl  = (int32_t) proc_state->m_register[idx_rbl];
        int64_t rb   = ((int64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(cst5 > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGT     0x%x,%s:%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPGTU - Compare for Greater Than; Unsigned Integers
ReturnStatus_t
C62xCMPGTU_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd = (uint32_t)(ra > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGTU     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGTU_UC4_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ucst4 = (uint16_t) constant & 0xF;
        uint32_t rb    = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd    = (uint32_t)(ucst4 > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGTU    0x%x,%s,%s\n",
                GetPC(proc_state), ucst4, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGTU_UR32_UR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rbh = (uint32_t) proc_state->m_register[idx_rbh];
        uint32_t rbl = (uint32_t) proc_state->m_register[idx_rbl];
        uint64_t rb  = ((uint64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(ra > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGTU    %s,%s:%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGTU_UC4_UR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ucst4 = (uint16_t) constant & 0xF;
        uint32_t rbh   = (uint32_t) proc_state->m_register[idx_rbh];
        uint32_t rbl   = (uint32_t) proc_state->m_register[idx_rbl];
        uint64_t rb    = ((uint64_t) rbh) << 32 | rbl;
        uint32_t rd    = (uint32_t)(ucst4 > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPGTU    0x%x,%s:%s,%s\n",
                GetPC(proc_state), ucst4, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPLT - Compare for Less Than; Signed Integers
ReturnStatus_t
C62xCMPLT_SR32_SR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) proc_state->m_register[idx_ra];
        int32_t rb  = (int32_t) proc_state->m_register[idx_rb];
        uint32_t rd = (uint32_t)(ra < rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLT     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLT_SC5_SR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst5  = (int16_t) constant & 0x1F;
        int32_t rb    = (int32_t) proc_state->m_register[idx_rb];
        uint32_t rd   = (uint32_t)(cst5 < rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLT     0x%x,%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLT_SR32_SR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) proc_state->m_register[idx_ra];
        int32_t rbh  = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl  = (int32_t) proc_state->m_register[idx_rbl];
        int64_t rb   = ((int64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(ra < rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLT     %s,%s:%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLT_SC5_SR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t cst5 = (int16_t) constant & 0x1F;
        int32_t rbh  = (int32_t) proc_state->m_register[idx_rbh];
        int32_t rbl  = (int32_t) proc_state->m_register[idx_rbl];
        int64_t rb   = ((int64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(cst5 < rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLT     0x%x,%s:%s,%s\n",
                GetPC(proc_state), cst5, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPLTU - Compare for Less Than; Unsigned Integers
ReturnStatus_t
C62xCMPLTU_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd = (uint32_t)(ra < rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLTU     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLTU_UC4_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ucst4 = (uint16_t) constant & 0xF;
        uint32_t rb    = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd    = (uint32_t)(ucst4 < rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLTU    0x%x,%s,%s\n",
                GetPC(proc_state), ucst4, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLTU_UR32_UR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rbh = (uint32_t) proc_state->m_register[idx_rbh];
        uint32_t rbl = (uint32_t) proc_state->m_register[idx_rbl];
        uint64_t rb  = ((uint64_t) rbh) << 32 | rbl;
        uint32_t rd  = (uint32_t)(ra < rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLTU    %s,%s:%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLTU_UC4_UR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        int32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ucst4 = (uint16_t) constant & 0xF;
        uint32_t rbh   = (uint32_t) proc_state->m_register[idx_rbh];
        uint32_t rbl   = (uint32_t) proc_state->m_register[idx_rbl];
        uint64_t rb    = ((uint64_t) rbh) << 32 | rbl;
        uint32_t rd    = (uint32_t)(ucst4 > rb);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tCMPLTU    0x%x,%s:%s,%s\n",
                GetPC(proc_state), ucst4, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// EXT - Extract and Sign-Extend a Bit Field
ReturnStatus_t
C62xEXT_UR32_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "Test This Instruction !!!");

    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) proc_state->m_register[idx_ra];
        int32_t  rb   = (int32_t) proc_state->m_register[idx_rb];

        uint16_t csta = (uint16_t) (rb & 0x3E0) >> 5;   /* bits 5 to 9 */
        uint16_t cstb = (uint16_t) rb & 0x1F;           /* bits 0 to 4 */

        int32_t rd    = (ra << csta) >> cstb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tEXT       %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xEXT_SR32_UC5_UC5_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, int32_t constanta, int32_t constantb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "Test This Instruction !!!");

    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra    = (int32_t) proc_state->m_register[idx_ra];

        int16_t csta  = (int16_t) constanta;
        int16_t cstb  = (int16_t) constantb;

        int32_t rd    = (ra << csta) >> cstb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tEXT       %s,0x%x,0x%x,%s\n",
                GetPC(proc_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

/// EXTU - Extract and Zero-Extend a Bit Field
ReturnStatus_t
C62xEXTU_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "Test This Instruction !!!");

    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];

        uint16_t csta = (uint16_t) (rb & 0x3E0) >> 5;   /* bits 5 to 9 */
        uint16_t cstb = (uint16_t) rb & 0x1F;           /* bits 0 to 4 */
        
        uint32_t rd   = (ra << csta) >> cstb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tEXTU      %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xEXTU_UR32_UC5_UC5_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, int32_t constanta, int32_t constantb, uint16_t idx_rd, uint8_t delay)
{
    ASSERT(1, "Test This Instruction !!!");

    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra    = (uint32_t) proc_state->m_register[idx_ra];

        uint16_t csta  = (uint16_t) constanta;
        uint16_t cstb  = (uint16_t) constantb;

        uint32_t rd    = (ra << csta) >> cstb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tEXTU       %s,0x%x,0x%x,%s\n",
                GetPC(proc_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

/// IDLE - Multi-cycle NOP with no termination until Interrupt.
ReturnStatus_t
C62xIDLE(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        TRACE_PRINT("%08x\tIDLE\n", GetPC(proc_state));
    }
    return WAIT_FOR_INTERRUPT;
}

int8_t * CalcMemAddrByMode(C62x_Proc_State_t * proc_state, uint32_t base_reg, uint16_t base_idx,
                     uint32_t ucst5, uint8_t mode, uint8_t shift)
{
    int8_t * ptr = NULL; /* NOTE: We use int8_t * type so pointer arithmetic is ALWAYS in byte multiples  */

    switch(mode)
    {
        case CST_NEGATIVE_OFFSET:        // *-R[ucst5]
            ptr = (int8_t *) base_reg - (ucst5 << shift);
            break;

        case CST_POSITIVE_OFFSET:        // *+R[ucst5]
            ptr = (int8_t *) base_reg + (ucst5 << shift);
            break;

        case CST_OFFSET_PRE_DECR:        // *--R[ucst5]
            base_reg -= ucst5 << shift;
            ptr = (int8_t *) base_reg;
            AddDelayedRegister(proc_state, base_idx, (uint32_t) base_reg, 0);
            break;

        case CST_OFFSET_PRE_INCR:        // *++R[ucst5]
            base_reg += ucst5 << shift;
            ptr = (int8_t *) base_reg;
            AddDelayedRegister(proc_state, base_idx, (uint32_t) base_reg, 0);
            break;

        case CST_OFFSET_POST_DECR:       // *R--[ucst5]
            ptr = (int8_t *) base_reg;
            base_reg -= ucst5 << shift;
            AddDelayedRegister(proc_state, base_idx, (uint32_t) base_reg, 0);
            break;

        case CST_OFFSET_POST_INCR:       // *R++[ucst5]
            ptr = (int8_t *) base_reg;
            base_reg += ucst5 << shift;
            AddDelayedRegister(proc_state, base_idx, (uint32_t) base_reg, 0);
            break;

        default:
            ASSERT(0, "Unknown Addressing Mode");
    }

    return (ptr);
}

/// LDB - Load Byte from Memory, Sign Extended
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDB_UC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint8_t ucst5 = constant & 0x1F;
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t amr  = (uint32_t) proc_state->m_register[REG_AMR_INDEX];
        uint32_t rd   = 0x0;
        int8_t * ptr  = NULL; /* NOTE: We use int8_t * type so pointer arithmetic is ALWAYS in byte multiples  */

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        ptr = CalcMemAddrByMode(proc_state, rb, idx_rb, ucst5, mode, 0);
        // Now we should have a valid address (mode-wise).
        rd = * ((int8_t *) ptr);

        if(0x80 & rd) // Need Sign Extension ?
            rd = rd | 0xFFFFFF00;
        else
            rd = rd & 0x000000FF;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tLDB       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%02X\n",
                GetPC(proc_state), ucst5, REG(idx_rb), REG(idx_rd), ptr, *ptr);
    }
    return OK;
}

/// LDBU - Load Byte from Memory, Zero Extended
ReturnStatus_t
C62xLDBU_UC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint8_t ucst5 = constant & 0x1F;
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t amr  = (uint32_t) proc_state->m_register[REG_AMR_INDEX];
        uint32_t rd   = 0x0;
        int8_t * ptr  = NULL; /* NOTE: We use int8_t * type so pointer arithmetic is ALWAYS in byte multiples  */

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        ptr = CalcMemAddrByMode(proc_state, rb, idx_rb, ucst5, mode, 0);
        // Now we should have a valid address (mode-wise).
        rd = * ((int8_t *) ptr);
        rd = rd & 0x000000FF;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tLDBU      0x%x,%s,%s\t\tMEM[0x%08X] = 0x%02X\n",
                GetPC(proc_state), ucst5, REG(idx_rb), REG(idx_rd), ptr, *ptr);
    }
    return OK;
}

/// LDH - Load Halfword from Memory, Sign Extended
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDH_UC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint8_t ucst5 = constant & 0x1F;
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t amr  = (uint32_t) proc_state->m_register[REG_AMR_INDEX];
        uint32_t rd   = 0x0;
        int8_t * ptr  = NULL; /* NOTE: We use int8_t * type so pointer arithmetic is ALWAYS in byte multiples  */

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        ptr = CalcMemAddrByMode(proc_state, rb, idx_rb, ucst5, mode, 1);
        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);

        if(0x8000 & rd) // Need Sign Extension ?
            rd = rd | 0xFFFF0000;
        else
            rd = rd & 0x0000FFFF;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tLDH       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%04X\n",
                GetPC(proc_state), ucst5, REG(idx_rb), REG(idx_rd), ptr, *ptr);
    }
    return OK;
}

/// LDHU - Load Halfword from Memory, Zero Extended
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDHU_UC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint8_t ucst5 = constant & 0x1F;
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t amr  = (uint32_t) proc_state->m_register[REG_AMR_INDEX];
        uint32_t rd   = 0x0;
        int8_t * ptr  = NULL; /* NOTE: We use int8_t * type so pointer arithmetic is ALWAYS in byte multiples  */

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        ptr = CalcMemAddrByMode(proc_state, rb, idx_rb, ucst5, mode, 1);
        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);
        rd = rd & 0x0000FFFF;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tLDHU      0x%x,%s,%s\t\tMEM[0x%08X] = 0x%04X\n",
                GetPC(proc_state), ucst5, REG(idx_rb), REG(idx_rd), ptr, *ptr);
    }
    return OK;
}

/// LDW - Load Word from Memory
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDW_UC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint8_t ucst5 = constant & 0x1F;
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t amr  = (uint32_t) proc_state->m_register[REG_AMR_INDEX];
        uint32_t rd   = 0x0;
        int8_t * ptr  = NULL; /* NOTE: We use int8_t * type so pointer arithmetic is ALWAYS in byte multiples  */

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        ptr = CalcMemAddrByMode(proc_state, rb, idx_rb, ucst5, mode, 2);
        // Now we should have a valid address (mode-wise).
        rd = * ((int32_t *) ptr);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tLDW       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X\n",
                GetPC(proc_state), ucst5, REG(idx_rb), REG(idx_rd), ptr, *ptr);
    }
    return OK;
}

/// LMBD - Left Most Bit Detection
ReturnStatus_t
C62xLMBD_UC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd   = 0x0;
        uint32_t detect_one = constant & 0x1; /* What are we searching for 0 or 1 */
        uint32_t mask = 0x80000000;

        if(!detect_one)
            rb = ~rb;  /* Invert bits; So we always look for the left most 1 bit */

        TRACE_PRINT("detect_one = %d\n", detect_one);

        while(!(mask & rb) && mask)
        {
            rd++; mask >>= 1;
            TRACE_PRINT("rd = %2d, mask = 0x%08X\n", rd, mask);
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);
        TRACE_PRINT("%08x\tLMBD      0x%x,%s,%s\n",
                GetPC(proc_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPY - Multiply Signed 16 LSB x Signed 16 LSB.
ReturnStatus_t
C62xMPY_SR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = (int16_t) proc_state->m_register[idx_ra];
        int16_t rb = (int16_t) proc_state->m_register[idx_rb];
        int32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPY       %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPY - Multiply Signed 16 LSB x Signed 16 LSB.
ReturnStatus_t
C62xMPY_SC5_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = (int16_t) 0x1F & constant;
        int16_t rb = (int16_t) proc_state->m_register[idx_rb];
        int32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPY       0x%x,%s,%s\n",
                GetPC(proc_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYH - Multiply Signed 16 MSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYH_SR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = (int16_t) (proc_state->m_register[idx_ra] >> 16);
        int16_t rb = (int16_t) (proc_state->m_register[idx_rb] >> 16);
        int32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYH      %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHL - Multiply Signed 16 MSB x Signed 16 LSB.
ReturnStatus_t
C62xMPYHL_SR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                         uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = (int16_t) (proc_state->m_register[idx_ra] >> 16);
        int16_t rb = (int16_t)  proc_state->m_register[idx_rb];
        int32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYHL     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHLU - Multiply Unsigned 16 MSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYHLU_UR16_UR16_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t) (proc_state->m_register[idx_ra] >> 16);
        uint16_t rb = (uint16_t)  proc_state->m_register[idx_rb];
        uint32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYHLU    %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHSLU - Multiply Signed 16 MSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYHSLU_SR16_UR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                           uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = (int16_t) (proc_state->m_register[idx_ra] >> 16);
        uint16_t rb = (uint16_t) proc_state->m_register[idx_rb];
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYHSLU   %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHSU - Multiply Signed 16 MSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYHSU_SR16_UR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = (int16_t)  (proc_state->m_register[idx_ra] >> 16);
        uint16_t rb = (uint16_t) (proc_state->m_register[idx_rb] >> 16);
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYHSU    %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHU - Multiply Unsigned 16 MSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYHU_UR16_UR16_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t) (proc_state->m_register[idx_ra] >> 16);
        uint16_t rb = (uint16_t) (proc_state->m_register[idx_rb] >> 16);
        uint32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYHU     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHULS - Multiply Unsigned 16 MSB x Signed 16 LSB.
ReturnStatus_t
C62xMPYHULS_UR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t) (proc_state->m_register[idx_ra] >> 16);
        int16_t  rb = (int16_t)   proc_state->m_register[idx_rb];
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYHULS   %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHUS - Multiply Unsigned 16 MSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYHUS_UR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t) (proc_state->m_register[idx_ra] >> 16);
        int16_t  rb = (int16_t)  (proc_state->m_register[idx_rb] >> 16);
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYHUS   %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLH - Multiply Signed 16 LSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYLH_SR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = (int16_t)  proc_state->m_register[idx_ra];
        int16_t  rb = (int16_t) (proc_state->m_register[idx_rb] >> 16);
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYLH    %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLHU - Multiply Unsigned 16 LSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYLHU_UR16_UR16_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t)  proc_state->m_register[idx_ra];
        uint16_t rb = (uint16_t) (proc_state->m_register[idx_rb] >> 16);
        uint32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYLHU   %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLSHU - Multiply Signed 16 LSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYLSHU_SR16_UR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = (int16_t)   proc_state->m_register[idx_ra];
        uint16_t rb = (uint16_t) (proc_state->m_register[idx_rb] >> 16);
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYLSHU   %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLUHS - Multiply Unsigned 16 LSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYLUHS_UR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t) proc_state->m_register[idx_ra];
        int16_t  rb = (int16_t) (proc_state->m_register[idx_rb] >> 16);
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYLUHS   %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYSU - Multiply Signed 16 LSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYSU_SR16_UR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = (int16_t)  proc_state->m_register[idx_ra];
        uint16_t rb = (uint16_t) proc_state->m_register[idx_rb];
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYSU     %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYSU - Multiply Signed 16 LSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYSU_SC5_UR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int16_t scst5 = (int16_t) 0x1F & constant;
        uint16_t rb = (uint16_t) proc_state->m_register[idx_rb];
        int32_t  rd = scst5 * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYSU     0x%x,%s,%s\n",
                GetPC(proc_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYU - Multiply Unsigned 16 LSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYU_UR16_UR16_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t) proc_state->m_register[idx_ra];
        uint16_t rb = (uint16_t) proc_state->m_register[idx_rb];
        uint32_t rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYU      %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYUS - Multiply Unsigned 16 LSB x Signed 16 LSB.
ReturnStatus_t
C62xMPYUS_UR16_SR16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = (uint16_t) proc_state->m_register[idx_ra];
        int16_t  rb = (int16_t)  proc_state->m_register[idx_rb];
        int32_t  rd = ra * rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMPYUS      %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMV_SR40_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                 uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t rdh = (int32_t) proc_state->m_register[idx_rah];
        int32_t rdl = (int32_t) proc_state->m_register[idx_ral];

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%08x\tMV        %s:%s,%s:%s\n",
                GetPC(proc_state), REG(idx_rah), REG(idx_ral), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xMV_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) proc_state->m_register[idx_ra];

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMV        %s,%s\n", GetPC(proc_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMV_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                 uint16_t idx_ra, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rd = (uint32_t) proc_state->m_register[idx_ra];

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMV        %s,%s\n", GetPC(proc_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

/// MVC - Move between control file and register file.
/*
 * Any write to the ISR or ICR (by the MVC instruction) effectively has one delay
 * slot because the results cannot be read (by the MVC instruction) in the IFR until
 * two cycles after the write to the ISR or ICR.
 * Note: The six MSBs of the AMR are reserved and therefore are not written to.
 * Refer to Table 3-16. Register Addresses for Accessing the Control Registers,
 * for R/W permissions on Control Registers
 * Any write to the interrupt clear register (ICR) is ignored by a simultaneous write to the same bit in ISR.
 */
ReturnStatus_t
C62xMVC_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                 uint16_t idx_ra, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        if(idx_ra != REG_ICR_INDEX && idx_ra != REG_ISR_INDEX &&
           idx_rd != REG_IFR_INDEX && idx_rd != REG_PC_INDEX)
        {
            uint32_t rd = (uint32_t) proc_state->m_register[idx_ra];

            if(idx_rd == REG_ISR_INDEX || idx_rd == REG_ICR_INDEX) delay++;

            AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

            TRACE_PRINT("%08x\tMVC        %s,%s\n", GetPC(proc_state), REG(idx_ra), REG(idx_rd));
        }
        else
        {
            ASSERT(1, "Invalid MVC Instruction Found !!!");
        }
    }
    return OK;
}

ReturnStatus_t
C62xMVK_SC16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  int32_t constant, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        if(constant & 0x8000)
            constant |= 0xFFFF0000;
        else
            constant &= 0x0000FFFF;

        int32_t rd = constant;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMVK       0x%x,%s\n",
                GetPC(proc_state), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMVKH_UC16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                   int32_t constant, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) proc_state->m_register[idx_rd];

        rd = (rd & 0x0000FFFF) | (constant << 16);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMVKH      0x%x,%s\n",
                GetPC(proc_state), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMVKLH_UC16_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                   int32_t constant, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) proc_state->m_register[idx_rd];

        rd = (rd & 0x0000FFFF) | (constant << 16);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tMVKLH     0x%x,%s\n",
                GetPC(proc_state), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xNEG_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_ra, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) 0 - (proc_state->m_register[idx_ra]);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tNEG       %s,%s\n", GetPC(proc_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xNEG_SR40_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int64_t rah = (int64_t) proc_state->m_register[idx_rah];
        int64_t ral = (int64_t) proc_state->m_register[idx_ral];

        int64_t  rd  = 0 - ((rah << 32) | ral);
        uint32_t rdh = (rd >> 32) & 0x000000FF;
        uint32_t rdl = rd & 0xFFFFFFFF;

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        TRACE_PRINT("%08x\tNEG       %s:%s,%s:%s\n", GetPC(proc_state), REG(idx_rah), REG(idx_ral), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// NORM - The number of redundant sign bits of src2 is placed in dst
ReturnStatus_t
C62xNORM_SR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_ra, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) (proc_state->m_register[idx_ra]);
        uint32_t rd = 0;
        uint32_t signbit = (ra & 0x80000000) >> 1;       /* Get the sign bit and shift it right by 1 step */
        uint32_t mask = 0x40000000;                      /* To start from bit # 30 */

        while(((ra & mask) == signbit) && mask)
        {
            rd++; mask >>= 1; signbit >>= 1;
            TRACE_PRINT("rd = %2d, mask = 0x%08X, signbit = 0x%08X\n", rd, mask, signbit);
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tNORM      %s,%s\n", GetPC(proc_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

/// NORM - The number of redundant sign bits of src2 is placed in dst
ReturnStatus_t
C62xNORM_SR40_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                   uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint64_t rah = (uint64_t) proc_state->m_register[idx_rah];
        uint64_t ral = (uint64_t) proc_state->m_register[idx_ral];
        uint64_t ra  = (rah << 32) | ral;
        uint32_t rd  = 0;
        uint64_t signbit = (ra & 0x8000000000) >> 1;       /* Get the sign bit and shift it right by 1 step */
        uint64_t mask = 0x4000000000;                      /* To start from bit # 39 */

        while(((ra & mask) == signbit) && mask)
        {
            rd++; mask >>= 1; signbit >>= 1;
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tNORM      %s:%s,%s\n", GetPC(proc_state), REG(idx_rah), REG(idx_ral), REG(idx_rd));
    }
    return OK;
}

/// NOT - Bitwise Not
ReturnStatus_t
C62xNOT_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_ra, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rd = ~(proc_state->m_register[idx_ra]);

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tNOT       %s,%s\n", GetPC(proc_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xOR_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd = ra | rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tOR        %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xOR_SC5_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                     uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t scst5 = ((constant & 0x10) ? (constant | 0xFFFFFFE0) : (constant & 0x0000001F));
        uint32_t rb = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd = scst5 | rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tOR        0x%x,%s,%s\n",
                GetPC(proc_state), scst5, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// SADD - Add two signed integers with saturation.
/*
 * src1 is added to src2 and saturated, if an overflow occurs according to the following rules:
 * 1. If the dst is an int and src1 + src2 > 2^31 - 1, then the result is 2^31 - 1.
 * 2. If the dst is an int and src1 + src2 < -2^31, then the result is -2^31.
 * 3. If the dst is a long and src1 + src2 > 2^39 - 1, then the result is 2^39 - 1.
 * 4. If the dst is a long and src1 + src2 < -2^39, then the result is -2^39.
 * The result is placed in dst. If a saturate occurs, the SAT bit in the control status register
 * (CSR) is set one cycle after dst is written.
 *
 * Saturate bit (Bit # 9). Can be cleared only by the MVC instruction and can be set
 * only by a functional unit. The set by a functional unit has priority over a
 * clear (by the MVC instruction), if they occur on the same cycle. The SAT
 * bit is set one full cycle (one delay slot) after a saturate occurs. The SAT
 * bit will not be modified by a conditional instruction whose condition is false.
 */

ReturnStatus_t
C62xSADD_SR32_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) proc_state->m_register[idx_ra];
        int32_t rb = (int32_t) proc_state->m_register[idx_rb];
        int32_t sflag = 0;

        int32_t rd = ra + rb;

        if((ra > 0) && (rb > 0) && (rd < 0))
        {
            rd = 0x7FFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }
        else if((ra < 0) && (rb < 0) && (rd > 0))
        {
            rd = 0x80000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        if(sflag)
        {
            uint32_t csr = (uint32_t) proc_state->m_register[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            AddDelayedRegister(proc_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      %s,%s,%s\n",
                    GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSADD_SR32_SR40_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra =  (int32_t) proc_state->m_register[idx_ra];
        int64_t rbh = (int64_t) proc_state->m_register[idx_rbh];
        int64_t rbl = (int64_t) proc_state->m_register[idx_rbl];
        int32_t sflag = 0;

        int64_t rb = (rbh << 32) | rbl;
        int64_t rd = ra + rb;

        if(!(ra & 0x80000000) && !(rb & 0x8000000000) && (rd & 0x8000000000))
        {
            rd = 0x7FFFFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }
        else if((ra & 0x80000000) && (rb & 0x8000000000) && !(rd & 0x8000000000))
        {
            rd = 0x8000000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }

        int32_t rdh = (int32_t) (rd >> 32) & 0xFF;
        int32_t rdl = (int32_t) (rd & 0xFFFFFFFF);

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        if(sflag)
        {
            uint32_t csr = (uint32_t) proc_state->m_register[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            AddDelayedRegister(proc_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      %s,%s:%s,%s:%s\n",
                    GetPC(proc_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSADD_SC5_SR32_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t scst5 = ((constant & 0x10) ? (constant | 0xFFFFFFE0) : (constant & 0x0000001F));
        int32_t rb = (int32_t) proc_state->m_register[idx_rb];
        int32_t sflag = 0;

        int32_t rd = scst5 + rb;

        if((scst5 > 0) && (rb > 0) && (rd < 0))
        {
            rd = 0x7FFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }
        else if((scst5 < 0) && (rb < 0) && (rd > 0))
        {
            rd = 0x80000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        if(sflag)
        {
            uint32_t csr = (uint32_t) proc_state->m_register[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            AddDelayedRegister(proc_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      0x%x,%s,%s\n",
                    GetPC(proc_state), scst5, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSADD_SC5_SR40_SR40(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t scst5 = ((constant & 0x10) ? (constant | 0xFFFFFFE0) : (constant & 0x0000001F));
        int64_t rbh = (int64_t) proc_state->m_register[idx_rbh];
        int64_t rbl = (int64_t) proc_state->m_register[idx_rbl];
        int32_t sflag = 0;

        int64_t rb = (rbh << 32) | rbl;
        int64_t rd = scst5 + rb;

        if(!(scst5 & 0x80000000) && !(rb & 0x8000000000) && (rd & 0x8000000000)) // 0x80000000 for scst5 is valid here, sign extended above
        {
            rd = 0x7FFFFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }
        else if((scst5 & 0x80000000) && (rb & 0x8000000000) && !(rd & 0x8000000000))
        {
            rd = 0x8000000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }

        int32_t rdh = (int32_t) (rd >> 32) & 0xFF;
        int32_t rdl = (int32_t) (rd & 0xFFFFFFFF);

        AddDelayedRegister(proc_state, idx_rdh, (uint32_t) rdh, delay);
        AddDelayedRegister(proc_state, idx_rdl, (uint32_t) rdl, delay);

        if(sflag)
        {
            uint32_t csr = (uint32_t) proc_state->m_register[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            AddDelayedRegister(proc_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      0x%x,%s:%s,%s:%s\n",
                    GetPC(proc_state), scst5, REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// SAT - Saturate a 40-bit integer to 32-bit integer
/*
 * A 40-bit src2 value is converted to a 32-bit value. If the value in src2 is greater than what
 * can be represented in 32-bits, src2 is saturated. The result is placed in dst. If a saturate
 * occurs, the SAT bit in the control status register (CSR) is set one cycle after dst is
 * written.
 */
ReturnStatus_t
C62xSAT_SR40_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int64_t rah = (int64_t) proc_state->m_register[idx_rah];
        int64_t ral = (int64_t) proc_state->m_register[idx_ral];
        int32_t sflag = 0;

        int64_t ra = (rah << 32) | ral;
        uint32_t rd = ra & 0xFFFFFFFF;

        if(!(ra & 0x8000000000) && ra > 0x7FFFFFFF)
        {
            rd = 0x7FFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }
        else if((ra & 0x8000000000) && ral < 0x80000000)
        {
            rd = 0x80000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        if(sflag)
        {
            uint32_t csr = (uint32_t) proc_state->m_register[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            AddDelayedRegister(proc_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSAT       %s:%s,%s\n", GetPC(proc_state), REG(idx_rah), REG(idx_ral), REG(idx_rd));
    }
    return OK;
}

/// SET - Set a Bitfield
/*
 * For cstb ≥ csta, the field in src2 as specified by csta to cstb is set to all 1s in dst. The
 * csta and cstb operands may be specified as constants or in the 10 LSBs of the src1
 * register, with cstb being bits 0-4 (src1 4..0) and csta being bits 5-9 (src1 9..5). csta is the
 * LSB of the field and cstb is the MSB of the field. In other words, csta and cstb represent
 * the beginning and ending bits, respectively, of the field to be set to all 1s in dst. The LSB
 * location of src2 is bit 0 and the MSB location of src2 is bit 31.
 *
 * For cstb < csta, the src2 register is copied to dst.
 */
ReturnStatus_t
C62xSET_UR32_UC5_UC5_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint32_t csta, uint32_t cstb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rd = 0x0;

        if(cstb < csta)
        {
            rd = ra;
        }
        else
        {
            uint32_t mask = 0xFFFFFFFF;

            uint16_t rshift = (uint16_t) csta;
            uint16_t lshift = (uint16_t) (31 - cstb);

            mask = (mask >> rshift) << rshift;
            mask = (mask << lshift) >> lshift;

            rd = ra | mask;
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tSET       %s,%d,%d,%s\n",
                GetPC(proc_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSET_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    proc_state->m_register[idx_ra] = 0x9ED31A31;
    proc_state->m_register[idx_rb] = 0x0000C197;

    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb   = (uint32_t) proc_state->m_register[idx_rb];

        uint16_t csta = (uint16_t) (rb & 0x3E0) >> 5;   /* bits 5 to 9 */
        uint16_t cstb = (uint16_t) rb & 0x1F;           /* bits 0 to 4 */

        uint32_t rd = 0x0;

        if(cstb < csta)
        {
            TRACE_PRINT("(cstb < csta) ==> (%d < %d), rb = 0x%08X\n", cstb, csta, rb);
            rd = ra;
        }
        else
        {
            uint32_t mask = 0xFFFFFFFF;

            uint16_t rshift = (uint16_t) csta;
            uint16_t lshift = (uint16_t) (31 - cstb);

            mask = (mask >> rshift) << rshift;
            mask = (mask << lshift) >> lshift;

            rd = ra | mask;
        }

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tSET       %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

// Working Here

ReturnStatus_t
C62xSHL_SR32_UC5_SR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra    = (int32_t) proc_state->m_register[idx_ra];
        uint8_t ucst5 = constant & 0x1F;
        int32_t rd    = ra << ucst5;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tSHL       %s,0x%x,%s\n",
                GetPC(proc_state), REG(idx_ra), ucst5, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSHRU_UR32_UC5_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) proc_state->m_register[idx_ra];
        uint8_t ucst5 = constant & 0x1F;
        uint32_t rd   = ra >> ucst5;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tSHRU      %s,0x%x,%s\n", GetPC(proc_state), REG(idx_ra), ucst5, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xXOR_UR32_UR32_UR32(C62x_Proc_State_t * proc_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay)
{
    if(ExecuteDecision(proc_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) proc_state->m_register[idx_ra];
        uint32_t rb = (uint32_t) proc_state->m_register[idx_rb];
        uint32_t rd = ra ^ rb;

        AddDelayedRegister(proc_state, idx_rd, (uint32_t) rd, delay);

        TRACE_PRINT("%08x\tXOR       %s,%s,%s\n",
                GetPC(proc_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

