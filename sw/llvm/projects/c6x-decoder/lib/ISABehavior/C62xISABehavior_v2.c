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

#include "C62xISABehavior_v2.h"
#include "stdio.h"

#ifdef ENABLE_STATS
extern uint32_t total_ep_count;
extern uint32_t total_bb_count;
extern uint32_t total_lmap_search;
extern uint32_t total_lmap_found;
extern uint32_t total_lmap_extraloops;

void Inc_EP_Count()             { total_ep_count++;             }
void Inc_BB_Count()             { total_bb_count++;             }
void Inc_LMAP_Search()          { total_lmap_search++;          }
void Inc_LMAP_Found()           { total_lmap_found++;           }
void Inc_LMAP_Extrloops()       { total_lmap_extraloops++;      }
#endif

extern char * BANKC_REGS[];

#ifdef QUEUE_BASED_DREGS

int32_t EnQ_Delay_Reg(C62x_DSPState_t * p_state, uint16_t reg_id, uint32_t value, uint8_t delay_slots)
{
    uint32_t             queue_id = (p_state->m_curr_cycle + delay_slots + 1) % (C62X_MAX_DELAY_SLOTS + 1);
    C62x_Delay_Node_t * tail_node = p_state->m_delay_q[queue_id].m_tail_node;

    tail_node->m_reg_id = reg_id;
    tail_node->m_value = value;
    p_state->m_delay_q[queue_id].m_tail_node = tail_node->m_next_node;

    return 0;
}

int32_t EnQ_Delay_Result(C62x_DSPState_t * p_state, C62x_Result_t * result, uint8_t delay_slots)
{
    uint32_t             queue_id = (p_state->m_curr_cycle + delay_slots + 1) % (C62X_MAX_DELAY_SLOTS + 1);
    C62x_Delay_Node_t * tail_node = p_state->m_delay_q[queue_id].m_tail_node;

    //TRACE_PRINT("%s: Type = 0x%X, Reg-ID[0] = 0x%02X, Value[0] = 0x%08X, Reg-ID[1] = 0x%02X, Value[1] = 0x%08X, Delay = %d\n",
    //        __func__, result->m_type, result->m_reg_id[0], result->m_value[0], result->m_reg_id[1], result->m_value[1], delay_slots);

    switch(result->m_type)
    {
        case C62X_NO_RESULT:
            break;

        case C62X_REGISTER:
            tail_node->m_reg_id = result->m_reg_id[0];
            tail_node->m_value  = result->m_value[0];

            p_state->m_delay_q[queue_id].m_tail_node = tail_node->m_next_node;
            break;

        case C62X_MULTIREGISTER:
            tail_node->m_reg_id = result->m_reg_id[0];
            tail_node->m_value  = result->m_value[0];

            tail_node = tail_node->m_next_node;

            tail_node->m_reg_id = result->m_reg_id[1];
            tail_node->m_value  = result->m_value[1];
            
            p_state->m_delay_q[queue_id].m_tail_node = tail_node->m_next_node;
            break;
    }
    return 0;
}

int32_t Update_Immediate(C62x_DSPState_t * p_state, C62x_Result_t * result)
{
    //TRACE_PRINT("%s: Type = 0x%X, Reg-ID[0] = 0x%02X, Value[0] = 0x%08X, Reg-ID[1] = 0x%02X, Value[1] = 0x%08X\n",
    //        __func__, result->m_type, result->m_reg_id[0], result->m_value[0], result->m_reg_id[1], result->m_value[1]);

    switch(result->m_type)
    {
        case C62X_NO_RESULT:
            break;

        case C62X_REGISTER:
            p_state->m_reg[result->m_reg_id[0]] = result->m_value[0];
            break;

        case C62X_MULTIREGISTER:
            p_state->m_reg[result->m_reg_id[0]] = result->m_value[0];
            p_state->m_reg[result->m_reg_id[1]] = result->m_value[1];
            break;

        default:
            TRACE_PRINT("%s: Unknown Type !!!\n");
    }
    return 0;
}

uint32_t Update_Registers(C62x_DSPState_t * p_state)
{
    uint32_t                queue_id = p_state->m_curr_cycle % (C62X_MAX_DELAY_SLOTS + 1);
    C62x_Delay_Queue_t * delay_queue = & p_state->m_delay_q[queue_id];
    C62x_Delay_Node_t  *   delay_reg = NULL;
    uint32_t                  new_pc = 0;
    uint16_t                  reg_id = 0;
    uint32_t               reg_value = 0;

    do
    {
        if(delay_queue->m_tail_node == delay_queue->m_head_node)
            break;

        delay_reg = delay_queue->m_head_node;
        delay_queue->m_head_node = delay_reg->m_next_node;

        reg_id    = delay_reg->m_reg_id;
        reg_value = delay_reg->m_value;

        // Update Processor State
        p_state->m_reg[reg_id] = reg_value;

        if(reg_id == REG_PC_INDEX)
            new_pc = reg_value;

    } while(1);

    return new_pc;
}

#else
int32_t EnQ_Delay_Reg(C62x_DSPState_t * p_state, uint16_t reg_id, uint32_t value, uint8_t delay_slots)
{
    uint32_t  queue_id = (p_state->m_curr_cycle + delay_slots + 1) % (C62X_MAX_DELAY_SLOTS + 1);
    C62x_Delay_Queue_t * delay_queue = & p_state->m_delay_q[queue_id];

    uint32_t  tail_idx = delay_queue->m_tail_idx;

    delay_queue->m_nodes[tail_idx].m_reg_id = reg_id;
    delay_queue->m_nodes[tail_idx].m_value = value;

    delay_queue->m_tail_idx = (tail_idx + 1) % DELAY_QUEUE_SIZE;

    return 0;
}

uint32_t Update_Registers(C62x_DSPState_t * p_state)
{
    uint32_t  queue_id = p_state->m_curr_cycle % (C62X_MAX_DELAY_SLOTS + 1);
    C62x_Delay_Queue_t * delay_queue = & p_state->m_delay_q[queue_id];

    uint32_t  head_idx = delay_queue->m_head_idx;
    uint32_t  tail_idx = delay_queue->m_tail_idx;
    uint32_t    new_pc = 0;
    uint16_t    reg_id = 0;
    uint32_t reg_value = 0;

    while(head_idx != tail_idx)
    {
        //printf("%s:%d: head_idx = %d, tail_idx = %d\n", __func__, __LINE__, head_idx, tail_idx);
        reg_id    = delay_queue->m_nodes[head_idx].m_reg_id;
        reg_value = delay_queue->m_nodes[head_idx].m_value;

        // Update Processor State
        p_state->m_reg[reg_id] = reg_value;
        if(reg_id == REG_PC_INDEX) new_pc = reg_value;

        head_idx = (head_idx + 1) % DELAY_QUEUE_SIZE;
        delay_queue->m_head_idx = head_idx;
    }

    return new_pc;
}
#endif

#ifdef DELAYED_MWBS
int32_t __EnQ_MWB(C62x_MWB_Queue_t * mwb_queue, C62xMWB_Size_t size, uint32_t addr, uint32_t value)
{
    if(mwb_queue->m_tail_node->m_next_node == mwb_queue->m_head_node)
    {
        printf("Error: Memory Write-Back Queue Full\n");
        return (-1);
    }

    mwb_queue->m_tail_node->m_size = size;
    mwb_queue->m_tail_node->m_addr = addr;
    mwb_queue->m_tail_node->m_value = value;
    mwb_queue->m_is_empty = 0;

    mwb_queue->m_tail_node = mwb_queue->m_tail_node->m_next_node;

    return (0);
}

int32_t EnQ_MWB(C62x_DSPState_t * p_state, C62xMWB_Size_t size, uint32_t addr, uint32_t value, uint8_t delay)
{
    uint32_t           mwb_queue_id = (p_state->m_curr_cycle + delay + 1) % (C62X_MAX_DELAY_SLOTS + 1);
    C62x_MWB_Queue_t * mwb_queue    = & p_state->m_mwback_q[mwb_queue_id];

    if(__EnQ_MWB(mwb_queue, size, addr, value))
    {
        return (-1);
    }
    return (0);
}

C62x_MWBack_Node_t * __DeQ_MWB(C62x_MWB_Queue_t * mwb_queue)
{
    if(mwb_queue->m_is_empty)
    {
        printf("Memory Write Back Queue Empty !!!\n");
        return (NULL);
    }

    C62x_MWBack_Node_t * mwb_node = mwb_queue->m_head_node;
    mwb_queue->m_head_node = mwb_node->m_next_node;

    if(mwb_queue->m_head_node == mwb_queue->m_tail_node)
    {
            mwb_queue->m_is_empty = 1;
    }

    return (mwb_node);
}

int32_t Do_Memory_Writebacks(C62x_DSPState_t * p_state)
{
    C62x_MWB_Queue_t    * mwb_queue = & p_state->m_mwback_q[p_state->m_curr_cycle % (C62X_MAX_DELAY_SLOTS + 1)];
    C62x_MWBack_Node_t  * mwb_node  = NULL;

    while(! mwb_queue->m_is_empty)
    {
        mwb_node = __DeQ_MWB(mwb_queue);
        switch(mwb_node->m_size)
        {
            case MWB_BYTE:
                *((uint8_t *) mwb_node->m_addr) = (uint8_t) mwb_node->m_value;
                TRACE_PRINT("Mem[%08X]=%02X\n", mwb_node->m_addr, (uint8_t) mwb_node->m_value);
                break;

            case MWB_HWORD:
                *((uint16_t *) mwb_node->m_addr) = (uint16_t) mwb_node->m_value;
                TRACE_PRINT("Mem[%08X]=%04X\n", mwb_node->m_addr, (uint16_t) mwb_node->m_value);
                break;

            case MWB_WORD:
                *((uint32_t *) mwb_node->m_addr) = (uint32_t) mwb_node->m_value;
                TRACE_PRINT("Mem[%08X]=%08X\n", mwb_node->m_addr, (uint32_t) mwb_node->m_value);
                break;

            default:
                ASSERT(1, "Unknown Memory Write-Back Size");
        }
    }

    return (0);
}
#endif

void Update_PC(C62x_DSPState_t * p_state, int32_t offset)
{
    p_state->m_reg[REG_PC_INDEX] += offset;
    return;
}

void Set_DSP_PC(C62x_DSPState_t * p_state, int32_t abs_addr)
{
    p_state->m_reg[REG_PC_INDEX] = abs_addr;
    return;
}

uint32_t Get_DSP_PC(C62x_DSPState_t * p_state)
{
    return(p_state->m_reg[REG_PC_INDEX]);
}

void Inc_DSP_Cycles(C62x_DSPState_t * p_state)
{
    p_state->m_curr_cycle++;
    return;
}

uint64_t Get_DSP_Cycles(C62x_DSPState_t * p_state)
{
    return(p_state->m_curr_cycle);
}

uint8_t Check_Predicate(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc)
{
    uint8_t do_execute = 1;       // By default; We Will execute this instruction.
    if(is_cond)
    {
        int32_t rc = p_state->m_reg[idx_rc];

        if(be_zero)
            do_execute = (rc == 0 ? 1 : 0);
        else
            do_execute = (rc != 0 ? 1 : 0);
    }
    return (do_execute);
}

/// ABS - Absolute value with saturation
ReturnStatus_t
C62xABS_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rd;

        if(ra >= 0)
            rd = ra;
        else if (ra < 0 && ra != 0x80000000)
            rd = -ra;
        else
            rd = 0x80000000;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tABS       %s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xABS_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rah = (int32_t) p_state->m_reg[idx_rah];
        int32_t ral = (int32_t) p_state->m_reg[idx_ral];
        int64_t ra  = C6X40_TO_S64(rah, ral);
        int64_t rd;

        if(ra >= 0)
            rd = ra;
        else if (ra < 0 && ra != 0x8000000000)
            rd = -ra;
        else
            rd = 0x8000000000;

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%010x\tABS       %s:%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// ADD - Add two signed integers without saturation.
ReturnStatus_t
C62xADD_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];

        int32_t rd = ra + rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADD       %s,%s,%s\n",
               Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SR32_SR32_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];
        int64_t rd = ra + rb;

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tADD       %s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SR32_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];

        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        int64_t rd  = ra + rb;

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tADD       %s,%s:%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SC5_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = C6XSC5_TO_S32(constant);
        int32_t rb  = (int32_t) p_state->m_reg[idx_rb];
        int32_t rd  = ra + rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADD       0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb  = constant & 0x1F;
        int32_t rd  = ra + rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADD       %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADD_SC5_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = C6XSC5_TO_S32(constant);
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];

        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        int64_t rd  = ra + rb;

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tADD       0x%x,%s:%s,%s:%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// ADDAB - Add using byte addressing mode of AMR
ReturnStatus_t
C62xADDAB_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = (int32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra + rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADDAB     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADDAB_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = constant & 0x1F;
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra + rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADDAB     %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

/// ADDAH - Add using halfword addressing mode of AMR
ReturnStatus_t
C62xADDAH_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = (int32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra + (rb << 1);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADDAH     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADDAH_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = constant & 0x1F;
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra + (rb << 1);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADDAH     %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

/// ADDAW - Add using word addressing mode of AMR
ReturnStatus_t
C62xADDAW_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = (int32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra + (rb << 2);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADDAW     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xADDAW_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t  rb  = constant & 0x1F;
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra + (rb << 2);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADDAW     %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

/// ADDK - Add signed 16-bit constant to register.
ReturnStatus_t
C62xADDK_SC16_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra  = GET_LSB16(constant);
        int32_t rd  = ra + (int32_t) p_state->m_reg[idx_rd];

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADDK      0x%x,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rd));
    }
    return OK;
}

/// ADDU - Add two unsigned integers without saturation.
ReturnStatus_t
C62xADDU_UR32_UR32_UR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint64_t rd = ra + rb;

        uint32_t rdh = U64_TO_C6XMSB12(rd);
        uint32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tADDU      %s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xADDU_UR32_UR40_UR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rbh = (uint32_t) p_state->m_reg[idx_rbh];
        uint32_t rbl = (uint32_t) p_state->m_reg[idx_rbl];

        uint64_t rb  = C6X40_TO_U64(rbh, rbl);
        uint64_t rd  = ra + rb;

        uint32_t rdh = U64_TO_C6XMSB12(rd);
        uint32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tADDU      %s,%s:%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// ADD2 - Add two 16-bit integers on upper and lower register halves.
ReturnStatus_t
C62xADD2_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb  = (int32_t) p_state->m_reg[idx_rb];

        int16_t ra_msb16 = GET_MSB16(ra);
        int16_t ra_lsb16 = GET_LSB16(ra);

        int16_t rb_msb16 = GET_MSB16(rb);
        int16_t rb_lsb16 = GET_LSB16(rb);

        int32_t rd  = ((int32_t)(ra_msb16 + rb_msb16) << 16) | (ra_lsb16 + rb_lsb16);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tADD2      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// AND - Bitwise AND
/*
 * Performs a bitwise AND operation between src1 and src2. The result is placed
 * in dst. The scst5 operands are sign extended to 32 bits.
 */
ReturnStatus_t
C62xAND_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = ra & rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tAND       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xAND_SC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = C6XSC5_TO_S32(constant);
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd  = ra & rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tAND       0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// B - Branch using a displacement
/*
 * A 21-bit signed constant, cst21, is shifted left by 2 bits and is added to the
 * address of the first instruction of the fetch packet that contains the branch
 * instruction. The result is placed in the program fetch counter (PFC). The
 * assembler/linker automatically computes the correct value for cst21 by the
 * following formula:
 * cst21 = (label - PCE1) >> 2
 * If two branches are in the same execute packet and both are taken, behavior
 * is undefined.
 * Two conditional branches can be in the same execute packet if one branch
 * uses a displacement and the other uses a register, IRP, or NRP. As long as only
 * one branch has a true condition, the code executes in a well-defined way.
 *
 * Notes:
 * 1) PCE1 (program counter) represents the address of the first instruction
 *    in the fetch packet in the E1 stage of the pipeline. PFC is the program
 *    fetch counter.
 * 2) The execute packets in the delay slots of a branch cannot be interrupted.
 *    This is true regardless of whether the branch is taken.
 * 3) See section 3.4.2 on page 3-9 for information on branching into the
 *    middle of an execute packet.
 */

ReturnStatus_t
C62xB_SC23(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  rd = C6XSCST23_TO_S32(constant);

        SAVE_REGISTER_RESULT(result, REG_PC_INDEX, rd);
        
        TRACE_PRINT("%08x\t[%s] B       0x%x\n", Get_DSP_PC(p_state), REG(idx_rc), rd);
    }
    return OK;
}

/// B - Branch Using a Register / IRP / NRP
/*
 * src2 is placed in the program fetch counter (PFC).
 * If two branches are in the same execute packet and are both taken, behavior
 * is undefined.
 * Two conditional branches can be in the same execute packet if one branch
 * uses a displacement and the other uses a register, IRP, or NRP. As long as only
 * one branch has a true condition, the code executes in a well-defined way.
 * Notes:
 * 1) This instruction executes on .S2 only. PFC is program fetch counter.
 * 2) The execute packets in the delay slots of a branch cannot be interrupted.
 *    This is true regardless of whether the branch is taken.
 * 3) See section 3.4.2 on page 3-9 for information on branching into the
 *    middle of an execute packet.
 *
 * Branch Using a IRP:
 * IRP is placed in the program fetch counter (PFC). This instruction also moves
 * the PGIE bit value to the GIE bit. The PGIE bit is unchanged.
 * If two branches are in the same execute packet and are both taken, behavior
 * is undefined.
 * Two conditional branches can be in the same execute packet if one branch
 * uses a displacement and the other uses a register, IRP, or NRP. As long as only
 * one branch has a true condition, the code executes in a well-defined way.
 *
 * Branch Using a NRP:
 * NRP is placed in the program fetch counter (PFC). This instruction also sets
 * the NMIE bit. The PGIE bit is unchanged.
 * If two branches are in the same execute packet and are both taken, behavior
 * is undefined.
 * Two conditional branches can be in the same execute packet if one branch
 * uses a displacement and the other uses a register, IRP, or NRP. As long as only
 * one branch has a true condition, the code executes in a well-defined way.
 */

ReturnStatus_t
C62xB_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rd = (uint32_t) p_state->m_reg[idx_ra];

        // TODO: Move the PGIE bit value to the GIE bit, if its B IRP Instruction.
        // Set the sets NMIE bit if its B NRP Instruction.

        SAVE_REGISTER_RESULT(result, REG_PC_INDEX, rd);

        TRACE_PRINT("%08x\tB         %s\n", Get_DSP_PC(p_state), REG(idx_ra));
    }
    return OK;
}

/// CLR - Clear a Bit Field
ReturnStatus_t
C62xCLR_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];

        uint16_t csta = GET_BITS_5_TO_9(rb);
        uint16_t cstb = GET_BITS_0_TO_4(rb);

        uint16_t rshift = cstb + 1;
        uint16_t lshift = 32 - csta;

        uint32_t ra_hi  = (ra >> rshift) << rshift;
        uint32_t ra_lo  = (ra << lshift) >> lshift;

        uint32_t rd = ra_hi | ra_lo;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCLR       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCLR_UR32_UC5_UC5_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constanta, uint32_t constantb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];

        uint16_t csta = (uint16_t) constanta;
        uint16_t cstb = (uint16_t) constantb;

        uint16_t rshift = cstb + 1;
        uint16_t lshift = 32 - csta;

        uint32_t ra_hi  = (ra >> rshift) << rshift;
        uint32_t ra_lo  = (ra << lshift) >> lshift;

        uint32_t rd = ra_hi | ra_lo;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCLR       %s,0x%x,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

/// CMPEQ - Compare for Equality; Signed Integer
ReturnStatus_t
C62xCMPEQ_SR32_SR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t  rb = (int32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra == rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPEQ     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPEQ_SC5_SR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra = C6XSC5_TO_S32(constant);
        int32_t  rb = (int32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra == rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPEQ     0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPEQ_SR32_SR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];
        int64_t  rb = C6X40_TO_S64(rbh, rbl);
        uint32_t rd = (uint32_t)(ra == rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPEQ     %s,%s:%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPEQ_SC5_SR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int64_t ra  = C6XSC5_TO_S64(constant);
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];
        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        uint32_t rd = (uint32_t)(ra == rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPEQ     0x%x,%s:%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPGT - Compare for Greater Than; Signed Integers
/*
 * Performs a signed comparison of src1 to src2. If src1 is greater than src2, then
 * a 1 is written to dst; otherwise, a 0 is written to dst.
 *
 * Note:
 * The CMPGT instruction allows using a 5-bit constant as src1. If src2 is a 5-bit
 * constant, as in
 *
 * CMPGT .L1 A4, 5, A0
 *
 * Then to implement this operation, the assembler converts this instruction to
 *
 * CMPLT .L1 5, A4, A0
 *
 * These two instructions are equivalent, with the second instruction using the
 * conventional operand types for src1 and src2.
 *
 * Similarly, the CMPGT instruction allows a cross path operand to be used as
 * src2. If src1 is a cross path operand as in
 *
 * CMPGT .L1x B4, A5, A0
 *
 * Then to implement this operation the assembler converts this instruction to
 *
 * CMPLT .L1x A5, B4, A0
 *
 * In both of these operations the listing file (.lst) will have the first implementa-
 * tion, and the second implementation will appear in the debugger.
 */
ReturnStatus_t
C62xCMPGT_SR32_SR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t  rb = (int32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGT     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGT_SC5_SR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra = C6XSC5_TO_S32(constant);
        int32_t  rb = (int32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGT     0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGT_SR32_SR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];
        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        uint32_t rd = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGT     %s,%s:%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGT_SC5_SR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int64_t ra  = C6XSC5_TO_S64(constant);
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];
        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        uint32_t rd = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGT     0x%x,%s:%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPGTU - Compare for Greater Than; Unsigned Integers
/*
 * Performs an unsigned comparison of src1 to src2. If src1 is greater than src2,
 * then a 1 is written to dst; otherwise, a 0 is written to dst. Only the four LSBs
 * are valid in the 5-bit dst field when the ucst4 operand is used. If the MSB of the
 * dst field is nonzero, the result is invalid.
 */
ReturnStatus_t
C62xCMPGTU_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGTU     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGTU_UC4_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                         uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = constant & 0xF;
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGTU    0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGTU_UR32_UR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rbh = (uint32_t) p_state->m_reg[idx_rbh];
        uint32_t rbl = (uint32_t) p_state->m_reg[idx_rbl];
        uint64_t rb  = C6X40_TO_U64(rbh, rbl);
        uint32_t rd  = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGTU    %s,%s:%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPGTU_UC4_UR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = constant & 0xF;
        uint32_t rbh = (uint32_t) p_state->m_reg[idx_rbh];
        uint32_t rbl = (uint32_t) p_state->m_reg[idx_rbl];
        uint64_t rb  = C6X40_TO_U64(rbh, rbl);
        uint32_t rd  = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPGTU    0x%x,%s:%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPLT - Compare for Less Than; Signed Integers
ReturnStatus_t
C62xCMPLT_SR32_SR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t  rb = (int32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra < rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLT     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLT_SC5_SR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra = C6XSC5_TO_S32(constant);
        int32_t  rb = (int32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra < rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLT     0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLT_SR32_SR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];
        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        uint32_t rd = (uint32_t)(ra < rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLT     %s,%s:%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLT_SC5_SR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int64_t ra  = C6XSC5_TO_S64(constant);
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];
        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        uint32_t rd = (uint32_t)(ra < rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLT     0x%x,%s:%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// CMPLTU - Compare for Less Than; Unsigned Integers
ReturnStatus_t
C62xCMPLTU_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra < rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLTU     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLTU_UC4_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                         uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = constant & 0xF;
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = (uint32_t)(ra < rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLTU    0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLTU_UR32_UR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rbh = (uint32_t) p_state->m_reg[idx_rbh];
        uint32_t rbl = (uint32_t) p_state->m_reg[idx_rbl];
        uint64_t rb  = C6X40_TO_U64(rbh, rbl);
        uint32_t rd  = (uint32_t)(ra < rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLTU    %s,%s:%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xCMPLTU_UC4_UR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant,  uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = constant & 0xF;
        uint32_t rbh = (uint32_t) p_state->m_reg[idx_rbh];
        uint32_t rbl = (uint32_t) p_state->m_reg[idx_rbl];
        uint64_t rb  = C6X40_TO_U64(rbh, rbl);
        uint32_t rd  = (uint32_t)(ra > rb);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tCMPLTU    0x%x,%s:%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rd));
    }
    return OK;
}

/// EXT - Extract and Sign-Extend a Bit Field
ReturnStatus_t
C62xEXT_UR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra    = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb    = (int32_t) p_state->m_reg[idx_rb];

        uint16_t csta = GET_BITS_5_TO_9(rb);
        uint16_t cstb = GET_BITS_0_TO_4(rb);

        int32_t rd    = (ra << csta) >> cstb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tEXT       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xEXT_SR32_UC5_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constanta, uint32_t constantb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra    = (int32_t) p_state->m_reg[idx_ra];

        int16_t csta  = (int16_t) constanta;
        int16_t cstb  = (int16_t) constantb;

        int32_t rd    = (ra << csta) >> cstb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tEXT       %s,0x%x,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

/// EXTU - Extract and Zero-Extend a Bit Field
ReturnStatus_t
C62xEXTU_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];

        uint16_t csta = GET_BITS_5_TO_9(rb);
        uint16_t cstb = GET_BITS_0_TO_4(rb);

        uint32_t rd   = (ra << csta) >> cstb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tEXTU      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xEXTU_UR32_UC5_UC5_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constanta, uint32_t constantb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra    = (uint32_t) p_state->m_reg[idx_ra];

        uint16_t csta  = (uint16_t) constanta;
        uint16_t cstb  = (uint16_t) constantb;

        uint32_t rd    = (ra << csta) >> cstb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tEXTU       %s,0x%x,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

/// IDLE - Multi-cycle NOP with no termination until Interrupt.
/*
 * Performs an infinite multicycle NOP that terminates upon servicing an
 * interrupt, or a branch occurs due to an IDLE instruction being in the delay slots
 * of a branch.
 */
ReturnStatus_t
C62xIDLE(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        TRACE_PRINT("%08x\tIDLE\n", Get_DSP_PC(p_state));
    }
    return WAIT_FOR_INTERRUPT;
}

// We use this function to calculate memory address using base + (constant or register) offsets.
int8_t * FindMemoryAddress(C62x_DSPState_t * p_state, uint32_t base_reg, uint16_t base_idx,
                           uint32_t offset, uint8_t mode, C62xAlignment_t shift)
{
    int8_t * ptr = NULL; /* NOTE: We use int8_t * type so pointer arithmetic is ALWAYS in byte multiples  */

    switch(mode)
    {
        case CST_NEGATIVE_OFFSET:        // *-R[constant]
        case REG_NEGATIVE_OFFSET:        // *-R[offsetR]
            ptr = (int8_t *) base_reg - (offset << shift);
            break;

        case CST_POSITIVE_OFFSET:        // *+R[constant]
        case REG_POSITIVE_OFFSET:        // *+R[offsetR]
            ptr = (int8_t *) base_reg + (offset << shift);
            break;

        case CST_OFFSET_PRE_DECR:        // *--R[constant]
        case REG_OFFSET_PRE_DECR:        // *--R[offsetR]
            base_reg -= offset << shift;
            ptr = (int8_t *) base_reg;
            EnQ_Delay_Reg(p_state, base_idx, (uint32_t) base_reg, 0);
            break;

        case CST_OFFSET_PRE_INCR:        // *++R[constant]
        case REG_OFFSET_PRE_INCR:        // *++R[offsetR]
            base_reg += offset << shift;
            ptr = (int8_t *) base_reg;
            EnQ_Delay_Reg(p_state, base_idx, (uint32_t) base_reg, 0);
            break;

        case CST_OFFSET_POST_DECR:       // *R--[constant]
        case REG_OFFSET_POST_DECR:       // *R--[offsetR]
            ptr = (int8_t *) base_reg;
            base_reg -= offset << shift;
            EnQ_Delay_Reg(p_state, base_idx, (uint32_t) base_reg, 0);
            break;

        case CST_OFFSET_POST_INCR:       // *R++[constant]
        case REG_OFFSET_POST_INCR:       // *R++[offsetR]
            ptr = (int8_t *) base_reg;
            base_reg += offset << shift;
            EnQ_Delay_Reg(p_state, base_idx, (uint32_t) base_reg, 0);
            break;

        default:
            ASSERT(1, "Unknown Addressing Mode");
    }

    return (ptr);
}

/// LDB - Load Byte from Memory, Sign Extended
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDB_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, ra, mode, BYTE_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int8_t *) ptr);
        rd = S8_TO_S32(rd); // Do Sign Extension

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDB       %s,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int8_t) rd, (int8_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDB_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, BYTE_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int8_t *) ptr);
        rd = S8_TO_S32(rd); // Do Sign Extension

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDB       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int8_t) rd, (int8_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDB_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, BYTE_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int8_t *) ptr);
        rd = S8_TO_S32(rd); // Do Sign Extension

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDB       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int8_t) rd, (int8_t) rd);
    }
    return OK;
}

/// LDBU - Load Byte from Memory, Zero Extended
ReturnStatus_t
C62xLDBU_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint8_t * ptr  = (uint8_t *) FindMemoryAddress(p_state, rb, idx_rb, ra, mode, BYTE_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((uint8_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDBU      %s,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint8_t) rd, (uint8_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDBU_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint8_t * ptr  = (uint8_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, BYTE_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((uint8_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDBU      0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint8_t) rd, (uint8_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDBU_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint8_t * ptr  = (uint8_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, BYTE_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((uint8_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDBU      0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint8_t) rd, (uint8_t) rd);
    }
    return OK;
}

/// LDH - Load Halfword from Memory, Sign Extended
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDH_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, ra, mode, HWORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);
        rd = S16_TO_S32(rd); // Do Sign Extension

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDH       %s,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%04X\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int16_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDH_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, HWORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);
        rd = S16_TO_S32(rd); // Do Sign Extension

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDH       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%04X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int16_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDH_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, HWORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);
        rd = S16_TO_S32(rd); // Do Sign Extension

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDH       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%04X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int16_t) rd);
    }
    return OK;
}

/// LDHU - Load Halfword from Memory, Zero Extended
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDHU_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, ra, mode, HWORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDHU      %s,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%04X\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int16_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDHU_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, HWORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDHU      0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%04X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int16_t) rd);
    }
    return OK;
}

ReturnStatus_t
C62xLDHU_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, HWORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int16_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDHU      0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X => 0x%04X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (int16_t) rd);
    }
    return OK;
}

/// LDW - Load Word from Memory
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xLDW_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra  = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, ra, mode, WORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int32_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDW       %s,%s,%s\t\tMEM[0x%08X] = 0x%08X\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr));
    }
    return OK;
}

ReturnStatus_t
C62xLDW_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, WORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int32_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDW       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr));
    }
    return OK;
}

ReturnStatus_t
C62xLDW_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb  = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];
        uint32_t rd  = 0x0;

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int8_t * ptr  = FindMemoryAddress(p_state, rb, idx_rb, constant, mode, WORD_ALIGN);

        // Now we should have a valid address (mode-wise).
        rd = * ((int32_t *) ptr);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLDW       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr));
    }
    return OK;
}

/// LMBD - Left Most Bit Detection
/*
 * The LSB of the src1 operand determines whether to search for a leftmost 1 or 0
 * in src2. The number of bits to the left of the first 1 or 0 when searching for a 1
 * or 0, respectively, is placed in dst.
 */
ReturnStatus_t
C62xLMBD_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd   = 0x0;
        uint32_t detect_one = ra & 0x1; /* What are we searching for 0 or 1 */
        uint32_t mask = 0x80000000;

        if(!detect_one)
            rb = ~rb;  /* Invert bits; So we always look for the left most 1 bit */

        TRACE_PRINT("detect_one = %d\n", detect_one);

        while(!(mask & rb) && mask)
        {
            rd++; mask >>= 1;
            TRACE_PRINT("rd = %2d, mask = 0x%08X\n", rd, mask);
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tLMBD      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xLMBD_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
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

        SAVE_REGISTER_RESULT(result, idx_rd, rd);
        TRACE_PRINT("%08x\tLMBD      0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPY - Multiply Signed 16 LSB x Signed 16 LSB.
ReturnStatus_t
C62xMPY_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = GET_LSB16(p_state->m_reg[idx_ra]);
        int16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPY       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPY - Multiply Signed 16 LSB x Signed 16 LSB.
ReturnStatus_t
C62xMPY_SC5_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = C6XSC5_TO_S16(constant);
        int16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPY       0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYH - Multiply Signed 16 MSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYH_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        int16_t rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYH      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHL - Multiply Signed 16 MSB x Signed 16 LSB.
ReturnStatus_t
C62xMPYHL_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                         uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        int16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYHL     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHLU - Multiply Unsigned 16 MSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYHLU_UR16_UR16_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        uint32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYHLU    %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHSLU - Multiply Signed 16 MSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYHSLU_SR16_UR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                           uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = GET_MSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYHSLU   %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHSU - Multiply Signed 16 MSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYHSU_SR16_UR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = GET_MSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYHSU    %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHU - Multiply Unsigned 16 MSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYHU_UR16_UR16_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_MSB16(p_state->m_reg[idx_rb]);
        uint32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYHU     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHULS - Multiply Unsigned 16 MSB x Signed 16 LSB.
ReturnStatus_t
C62xMPYHULS_UR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        int16_t  rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYHULS   %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYHUS - Multiply Unsigned 16 MSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYHUS_UR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        int16_t  rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYHUS   %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLH - Multiply Signed 16 LSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYLH_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = GET_LSB16(p_state->m_reg[idx_ra]);
        int16_t  rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYLH    %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLHU - Multiply Unsigned 16 LSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYLHU_UR16_UR16_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_LSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_MSB16(p_state->m_reg[idx_rb]);
        uint32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYLHU   %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLSHU - Multiply Signed 16 LSB x Unsigned 16 MSB.
ReturnStatus_t
C62xMPYLSHU_SR16_UR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = GET_LSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYLSHU   %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYLUHS - Multiply Unsigned 16 LSB x Signed 16 MSB.
ReturnStatus_t
C62xMPYLUHS_UR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_LSB16(p_state->m_reg[idx_ra]);
        int16_t  rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYLUHS   %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYSU - Multiply Signed 16 LSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYSU_SR16_UR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = GET_LSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYSU     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYSU - Multiply Signed 16 LSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYSU_SC5_UR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t  ra = C6XSC5_TO_S16(constant);
        uint16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYSU     0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYU - Multiply Unsigned 16 LSB x Unsigned 16 LSB.
ReturnStatus_t
C62xMPYU_UR16_UR16_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_LSB16(p_state->m_reg[idx_ra]);
        uint16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        uint32_t rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYU      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// MPYUS - Multiply Unsigned 16 LSB x Signed 16 LSB.
ReturnStatus_t
C62xMPYUS_UR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint16_t ra = GET_LSB16(p_state->m_reg[idx_ra]);
        int16_t  rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t  rd = ra * rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMPYUS      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMV_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                 uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rdh = (int32_t) p_state->m_reg[idx_rah];
        int32_t rdl = (int32_t) p_state->m_reg[idx_ral];

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tMV        %s:%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xMV_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) p_state->m_reg[idx_ra];

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMV        %s,%s\n", Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMV_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                 uint16_t idx_ra, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rd = (uint32_t) p_state->m_reg[idx_ra];

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMV        %s,%s\n", Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rd));
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
C62xMVC_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                 uint16_t idx_ra, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        if(idx_ra != REG_ICR_INDEX && idx_ra != REG_ISR_INDEX &&
           idx_rd != REG_IFR_INDEX && idx_rd != REG_PC_INDEX)
        {
            uint32_t rd = (uint32_t) p_state->m_reg[idx_ra];

            if(idx_rd == REG_ISR_INDEX || idx_rd == REG_ICR_INDEX) delay++;

            SAVE_REGISTER_RESULT(result, idx_rd, rd);

            TRACE_PRINT("%08x\tMVC        %s,%s\n", Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rd));
        }
        else
        {
            ASSERT(1, "Invalid MVC Instruction Found !!!");
        }
    }
    return OK;
}

ReturnStatus_t
C62xMVK_SC16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = S16_TO_S32(constant);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMVK       0x%x,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMVKH_UC16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                   uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) p_state->m_reg[idx_rd];

        rd = (rd & 0xFFFF) | (constant << 16);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMVKH      0x%x,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xMVKLH_UC16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                   uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) p_state->m_reg[idx_rd];

        rd = (rd & 0x0000FFFF) | (constant << 16);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tMVKLH     0x%x,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xNEG_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_ra, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rd = (int32_t) 0 - (p_state->m_reg[idx_ra]);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tNEG       %s,%s\n", Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xNEG_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int64_t rah = (int64_t) p_state->m_reg[idx_rah];
        int64_t ral = (int64_t) p_state->m_reg[idx_ral];

        int64_t  rd  = 0 - C6X40_TO_S64(rah, ral);

        uint32_t rdh = U64_TO_C6XMSB12(rd);
        uint32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tNEG       %s:%s,%s:%s\n", Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// NOP - No Operation
/*
 * src is encoded as count - 1. For src + 1 cycles, no operation is performed. The
 * maximum value for count is 9. NOP with no operand is treated like NOP 1 with
 * src encoded as 0000.
 * A multicycle NOP will not finish if a branch is completed first. For example, if
 * a branch is initiated on cycle n and a NOP 5 instruction is initiated on cycle
 * n + 3, the branch is complete on cycle n + 6 and the NOP is executed only from
 * cycle n + 3 to cycle n + 5. A single-cycle NOP in parallel with other instructions
 * does not affect operation.
 */

ReturnStatus_t
C62xNOP(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
        uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        TRACE_PRINT("%08x\tNOP\n", Get_DSP_PC(p_state));
    }
    return OK;
}

ReturnStatus_t
C62xNOP_UC4(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
            uint32_t constant, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t nop_count = GET_BITS_0_TO_3(constant) - 1;
        uint32_t new_pc = 0;

        TRACE_PRINT("%08x\tNOP       0x%x\n", Get_DSP_PC(p_state), constant);
        for(uint32_t i = 0; i < nop_count; i++)
        {
            //printf("Multi-Cycle NOP; i = %d\n", i);
            Inc_DSP_Cycles(p_state);
#ifdef DELAYED_MWBS
            Do_Memory_Writebacks(p_state);
#endif
            new_pc = Update_Registers(p_state);
            if(new_pc)
            {
                TRACE_PRINT("Multi-Cycle NOP Early Termination [i=%d]; PC Updated\n", i);
                return PC_UPDATED;
            }
        }
    }
    return OK;
}

/// NORM - The number of redundant sign bits of src2 is placed in dst
ReturnStatus_t
C62xNORM_SR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_ra, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) (p_state->m_reg[idx_ra]);
        uint32_t rd = 0;
        uint32_t signbit = (ra & 0x80000000) >> 1;       /* Get the sign bit and shift it right by 1 step */
        uint32_t mask = 0x40000000;                      /* To start from bit # 30 */

        while(((ra & mask) == signbit) && mask)
        {
            rd++; mask >>= 1; signbit >>= 1;
            TRACE_PRINT("rd = %2d, mask = 0x%08X, signbit = 0x%08X\n", rd, mask, signbit);
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tNORM      %s,%s\n", Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

/// NORM - The number of redundant sign bits of src2 is placed in dst
ReturnStatus_t
C62xNORM_SR40_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                   uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint64_t rah = (uint64_t) p_state->m_reg[idx_rah];
        uint64_t ral = (uint64_t) p_state->m_reg[idx_ral];
        uint64_t ra  = (rah << 32) | ral;
        uint32_t rd  = 0;
        uint64_t signbit = (ra & 0x8000000000) >> 1;       /* Get the sign bit and shift it right by 1 step */
        uint64_t mask = 0x4000000000;                      /* To start from bit # 39 */

        while(((ra & mask) == signbit) && mask)
        {
            rd++; mask >>= 1; signbit >>= 1;
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tNORM      %s:%s,%s\n", Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rd));
    }
    return OK;
}

/// NOT - Bitwise Not
ReturnStatus_t
C62xNOT_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_ra, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rd = ~(p_state->m_reg[idx_ra]);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tNOT       %s,%s\n", Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xOR_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = ra | rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tOR        %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xOR_SC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                     uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = C6XSC5_TO_S32(constant);
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = ra | rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tOR        0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
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
C62xSADD_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];
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

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      %s,%s,%s\n",
                    Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSADD_SR32_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];
        int32_t sflag = 0;

        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        int64_t rd  = ra + rb;

        if((ra > 0) && (rb > 0) && (rd < 0))
        {
            rd = 0x7FFFFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }
        else if((ra < 0) && (rb < 0) && (rd > 0))
        {
            rd = 0x8000000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      %s,%s:%s,%s:%s\n",
                    Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSADD_SC5_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = C6XSC5_TO_S32(constant);
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];
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

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      0x%x,%s,%s\n",
                    Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSADD_SC5_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int64_t ra    = C6XSC5_TO_S64(constant);
        int64_t rbh   = (int64_t) p_state->m_reg[idx_rbh];
        int64_t rbl   = (int64_t) p_state->m_reg[idx_rbl];
        int32_t sflag = 0;

        int64_t rb    = C6X40_TO_S64(rbh, rbl);
        int64_t rd    = ra + rb;

        if((ra > 0) && (rb > 0) && (rd < 0))
        {
            rd = 0x7FFFFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }
        else if((ra < 0) && (rb < 0) && (rd > 0))
        {
            rd = 0x8000000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSADD      0x%x,%s:%s,%s:%s\n",
                    Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
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
C62xSAT_SR40_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                  uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rah = (int32_t) p_state->m_reg[idx_rah];
        int32_t ral = (int32_t) p_state->m_reg[idx_ral];
        int32_t sflag = 0;

        int64_t  ra = C6X40_TO_S64(rah, ral);
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

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSAT       %s:%s,%s\n", Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rd));
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
C62xSET_UR32_UC5_UC5_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint32_t csta, uint32_t cstb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
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

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSET       %s,%d,%d,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), csta, cstb, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSET_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];

        uint16_t csta = GET_BITS_5_TO_9(rb);
        uint16_t cstb = GET_BITS_0_TO_4(rb);

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

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSET       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// SHL - Arithmetic Shift Left
/*
 * The src2 operand is shifted to the left by the src1 operand. The result is placed in dst.
 * When a register is used, the six LSBs specify the shift amount and valid values are 0-40.
 * When an immediate is used, valid shift amounts are 0-31. If src2 is a register pair, only
 * the bottom 40 bits of the register pair are shifted. The upper 24 bits of the register pair
 * are unused.
 * If 39 < src1 < 64, src2 is shifted to the left by 40. Only the six LSBs of src1 are used by
 * the shifter, so any bits set above bit 5 do not affect execution.
 */

ReturnStatus_t
C62xSHL_SR32_UR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra     = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t shift = GET_BITS_0_TO_5(p_state->m_reg[idx_rb]);

        int32_t rd     = 0;
        if(shift < 32)
            rd = ra << shift;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSHL       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSHL_SR40_UR32_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rah   = (uint32_t) p_state->m_reg[idx_rah];
        uint32_t ral   = (uint32_t) p_state->m_reg[idx_ral];
        int32_t shift  = GET_BITS_0_TO_5(p_state->m_reg[idx_rb]);

        uint64_t ra    = C6X40_TO_U64(rah, ral);
        uint64_t rd    = 0;
        if(shift < 40)
            rd = ra << shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHL       %s:%s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSHL_UR32_UR32_UR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint64_t ra    = (uint64_t) p_state->m_reg[idx_ra];
        int32_t shift  = GET_BITS_0_TO_5(p_state->m_reg[idx_rb]);

        //ASSERT(0 <= shift && shift < 40, "Invalid Shift Amount\n");

        uint64_t rd    = 0;
        if(shift < 40)
            rd = ra << shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHL       %s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSHL_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra    = (int32_t) p_state->m_reg[idx_ra];
        int32_t shift = constant & 0x1F;

        int32_t rd    = ra << shift;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSHL       %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), shift, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSHL_SR40_UC5_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_rah, uint16_t idx_ral, uint32_t constant, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rah   = (uint32_t) p_state->m_reg[idx_rah];
        uint32_t ral   = (uint32_t) p_state->m_reg[idx_ral];
        uint8_t shift  = constant & 0x1F;

        int64_t  ra    = C6X40_TO_S64(rah, ral);
        uint64_t rd    = ra << shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHL       %s:%s,0x%x,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), shift, REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSHL_UR32_UC5_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint32_t constant, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint64_t ra    = (uint64_t) p_state->m_reg[idx_ra];
        uint32_t shift = constant & 0x1F;

        uint64_t rd    = ra << shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHL       %s,0x%x,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), shift, REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// SHR - Arithmetic Shift Right
/*
 * The src2 operand is shifted to the right by the src1 operand. The sign-extended
 * result is placed in dst. When a register is used, the six LSBs specify the shift
 * amount and valid values are 0--40. When an immediate value is used, valid
 * shift amounts are 0–31.
 * If 39 < src1 < 64, src2 is shifted to the right by 40. Only the six LSBs of src1 are
 * used by the shifter, so any bits set above bit 5 do not affect execution.
 */
ReturnStatus_t
C62xSHR_SR32_UR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra    = (int32_t) p_state->m_reg[idx_ra];
        int32_t shift = GET_BITS_0_TO_5(p_state->m_reg[idx_rb]);

        ASSERT(0 <= shift && shift < 32, "Invalid Shift Amount\n");

        int32_t rd    = ra >> shift;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSHR       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSHR_SR40_UR32_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rah    = (int32_t) p_state->m_reg[idx_rah];
        int32_t ral    = (int32_t) p_state->m_reg[idx_ral];
        int32_t shift  = GET_BITS_0_TO_5(p_state->m_reg[idx_rb]);

        ASSERT(0 <= shift && shift < 40, "Invalid Shift Amount\n");

        int64_t ra     = C6X40_TO_S64(rah, ral);
        int64_t rd     = ra >> shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHR       %s:%s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSHR_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra    = (int32_t) p_state->m_reg[idx_ra];
        int32_t shift = constant & 0x1F;

        int32_t rd    = ra >> shift;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSHR       %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), shift, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSHR_SR40_UC5_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_rah, uint16_t idx_ral, uint32_t constant, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rah    = (int32_t) p_state->m_reg[idx_rah];
        int32_t ral    = (int32_t) p_state->m_reg[idx_ral];
        uint32_t shift = constant & 0x1F;

        int64_t ra     = C6X40_TO_S64(rah, ral);
        int64_t rd     = ra >> shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHR       %s:%s,0x%x,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), shift, REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// SHRU - Logical Shift Right
/*
 * The src2 operand is shifted to the right by the src1 operand. The zero-extended result is
 * placed in dst. When a register is used, the six LSBs specify the shift amount and valid
 * values are 0-40. When an immediate value is used, valid shift amounts are 0-31. If src2
 * is a register pair, only the bottom 40 bits of the register pair are shifted. The upper 24
 * bits of the register pair are unused.
 * If 39 < src1 < 64, src2 is shifted to the right by 40. Only the six LSBs of src1 are used by
 * the shifter, so any bits set above bit 5 do not affect execution.
 */
ReturnStatus_t
C62xSHRU_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra    = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t shift = GET_BITS_0_TO_5(p_state->m_reg[idx_rb]);

        ASSERT(0 <= shift && shift < 32, "Invalid Shift Amount\n");

        uint32_t rd    = ra >> shift;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSHRU      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSHRU_UR40_UR32_UR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_rah, uint16_t idx_ral, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rah   = (uint32_t) p_state->m_reg[idx_rah];
        uint32_t ral   = (uint32_t) p_state->m_reg[idx_ral];
        uint32_t shift = GET_BITS_0_TO_5(p_state->m_reg[idx_rb]);

        ASSERT(0 <= shift && shift < 40, "Invalid Shift Amount\n");

        uint64_t ra    = C6X40_TO_U64(rah, ral);
        uint64_t rd    = ra >> shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHRU      %s:%s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSHRU_UR32_UC5_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra    = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t shift = constant & 0x1F;

        uint32_t rd    = ra >> shift;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSHRU      %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), shift, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSHRU_UR40_UC5_UR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_rah, uint16_t idx_ral, uint32_t constant, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rah   = (uint32_t) p_state->m_reg[idx_rah];
        uint32_t ral   = (uint32_t) p_state->m_reg[idx_ral];
        uint32_t shift = constant & 0x1F;

        uint64_t ra    = C6X40_TO_U64(rah, ral);
        uint64_t rd    = ra >> shift;

        uint32_t rdh   = U64_TO_C6XMSB12(rd);
        uint32_t rdl   = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSHRU      %s:%s,0x%x,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), shift, REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// SMPY - Multiply Signed 16 LSB × Signed 16 LSB With Left Shift and Saturation
/*
 * The 16 least-significant bits of src1 operand is multiplied by the 16 least-significant bits
 * of the src2 operand. The result is left shifted by 1 and placed in dst. If the left-shifted
 * result is 8000 0000h, then the result is saturated to 7FFF FFFFh. If a saturate occurs,
 * the SAT bit in CSR is set one cycle after dst is written. The source operands are signed
 * by default.
 */
ReturnStatus_t
C62xSMPY_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = GET_LSB16(p_state->m_reg[idx_ra]);
        int16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t sflag = 0;

        int32_t rd = (ra * rb) << 1;

        if(rd == 0x80000000)
        {
            rd = 0x7FFFFFFF; sflag = 1;
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSMPY      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// SMPYH - Multiply Signed 16 MSB × Signed 16 MSB With Left Shift and Saturation
ReturnStatus_t
C62xSMPYH_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        int16_t rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t sflag = 0;

        int32_t rd = (ra * rb) << 1;

        if(rd == 0x80000000)
        {
            rd = 0x7FFFFFFF; sflag = 1;
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSMPYH     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// SMPYHL - Multiply Signed 16 MSB × Signed 16 LSB With Left Shift and Saturation
ReturnStatus_t
C62xSMPYHL_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = GET_MSB16(p_state->m_reg[idx_ra]);
        int16_t rb = GET_LSB16(p_state->m_reg[idx_rb]);
        int32_t sflag = 0;

        int32_t rd = (ra * rb) << 1;

        if(rd == 0x80000000)
        {
            rd = 0x7FFFFFFF; sflag = 1;
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSMPYHL    %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// SMPYLH - Multiply Signed 16 LSB × Signed 16 MSB With Left Shift and Saturation
ReturnStatus_t
C62xSMPYLH_SR16_SR16_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                          uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int16_t ra = GET_LSB16(p_state->m_reg[idx_ra]);
        int16_t rb = GET_MSB16(p_state->m_reg[idx_rb]);
        int32_t sflag = 0;

        int32_t rd = (ra * rb) << 1;

        if(rd == 0x80000000)
        {
            rd = 0x7FFFFFFF; sflag = 1;
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSMPYLH    %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// SSHL - Shift Left With Saturation
/*
 * The src2 operand is shifted to the left by the src1 operand. The result is placed in dst.
 * When a register is used to specify the shift, the 5 least-significant bits specify the shift
 * amount. Valid values are 0 through 31, and the result of the shift is invalid if the shift
 * amount is greater than 31. The result of the shift is saturated to 32 bits. If a saturate
 * occurs, the SAT bit in CSR is set one cycle after dst is written.
 */
ReturnStatus_t
C62xSSHL_SR32_UR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra     = (int32_t) p_state->m_reg[idx_ra];
        int32_t shift  = (int32_t) p_state->m_reg[idx_rb] & 0x1F; // 0x3F for C64x !!!
        int32_t sflag  = 0;
        uint32_t bitmask = 0x80000000;
        uint32_t signbit = ra & bitmask;

        ASSERT(0 <= shift && shift < 32, "Invalid Shift Amount\n");

        for(int i = 30; i >= (31 - shift); i--)
        {
            if(signbit && !(ra & signbit))
            {
                sflag = 1; break;
            }
            else if(bitmask & ra)
            {
                sflag = 1; break;
            }

            signbit >>= 1;
            bitmask >>= 1;
        }

        int32_t rd = ra << shift;

        if(sflag && ra > 0)
            rd = 0x7FFFFFFF;
        else if(sflag && ra < 0)
            rd = 0x80000000;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSSHL       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSSHL_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra     = (int32_t) p_state->m_reg[idx_ra];
        int32_t shift  = constant & 0x1F;
        int32_t sflag  = 0;
        uint32_t bitmask = 0x80000000;
        uint32_t signbit = ra & bitmask;

        ASSERT(0 <= shift && shift < 32, "Invalid Shift Amount\n");

        for(int i = 30; i >= (31 - shift); i--)
        {
            if(signbit && !(ra & signbit))
            {
                sflag = 1; break;
            }
            else if(bitmask & ra)
            {
                sflag = 1; break;
            }

            signbit >>= 1;
            bitmask >>= 1;
        }

        int32_t rd = ra << shift;

        if(sflag && ra > 0)
            rd = 0x7FFFFFFF;
        else if(sflag && ra < 0)
            rd = 0x80000000;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSSHL       %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), shift, REG(idx_rd));
    }
    return OK;
}

/// SSUB - Subtract Two Signed Integers With Saturation
/*
 * src2 is subtracted from src1 and is saturated to the result size according to the following
 * rules:
 * 1. If the result is an int and src1 - src2 > 2^31 - 1, then the result is 2^31 - 1.
 * 2. If the result is an int and src1 - src2 < -2^31, then the result is -2^31.
 * 3. If the result is a long and src1 - src2 > 2^39 - 1, then the result is 2^39 - 1.
 * 4. If the result is a long and src1 - src2 < -2^39, then the result is -2^39.
 * The result is placed in dst. If a saturate occurs, the SAT bit in CSR is set one cycle after
 * dst is written.
 */

ReturnStatus_t
C62xSSUB_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];
        int32_t sflag = 0;

        int32_t rd = ra - rb;

        if((ra < 0) && (rb > 0) && (rd > 0))
        {
            rd = 0x80000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }
        else if((ra > 0) && (rb < 0) && (rd < 0))
        {
            rd = 0x7FFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSSUB      %s,%s,%s\n",
                    Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSSUB_SC5_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = C6XSC5_TO_S32(constant);
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];
        int32_t sflag = 0;

        int32_t rd = ra - rb;

        if((ra < 0) && (rb > 0) && (rd > 0))
        {
            rd = 0x80000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }
        else if((ra > 0) && (rb < 0) && (rd < 0))
        {
            rd = 0x7FFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSSUB      0x%x,%s,%s\n",
                    Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSSUB_SC5_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int64_t ra    = C6XSC5_TO_S64(constant);
        int32_t rbh   = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl   = (int32_t) p_state->m_reg[idx_rbl];
        int32_t sflag = 0;

        int64_t rb = C6X40_TO_S64(rbh, rbl);
        int64_t rd = ra - rb;

        if((ra < 0) && (rb > 0) && (rd > 0))
        {
            rd = 0x8000000000; sflag = 1;
            TRACE_PRINT("-VE Saturation\n");
        }
        else if((ra > 0) && (rb < 0) && (rd < 0))
        {
            rd = 0x7FFFFFFFFF; sflag = 1;
            TRACE_PRINT("+VE Saturation\n");
        }

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        if(sflag)
        {
            uint32_t csr = (uint32_t) p_state->m_reg[REG_CSR_INDEX];
            csr |= 1 << 9;  /* SAT bit is at position 9 in CSR */
            EnQ_Delay_Reg(p_state, REG_CSR_INDEX, (uint32_t) csr, delay + 1);
        }

        TRACE_PRINT("%08x\tSSUB      0x%x,%s:%s,%s:%s\n",
                    Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/*  For SSUB
    constant = 0x1E; // -2; With -1 we will still remain in range
    p_state->m_reg[idx_rbh] = 0x7F;
    p_state->m_reg[idx_rbl] = 0xFFFFFFFF;
 */

/// STB - Store Byte to Memory
/// We will use the same target addresses here as this code will execute in KVM.
ReturnStatus_t
C62xSTB_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint8_t * ptr = (uint8_t *) FindMemoryAddress(p_state, rb, idx_rb, ra, mode, BYTE_ALIGN);

        TRACE_PRINT("%08x\tSTB       %s,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint8_t) src, (uint8_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_BYTE, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint8_t) src;
#endif
    }
    return OK;
}

ReturnStatus_t
C62xSTB_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint8_t * ptr = (uint8_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, BYTE_ALIGN);

        TRACE_PRINT("%08x\tSTB       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint8_t) src, (uint8_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_BYTE, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint8_t) src;
#endif
    }
    return OK;
}

ReturnStatus_t
C62xSTB_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint8_t * ptr = (uint8_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, BYTE_ALIGN);

        TRACE_PRINT("%08x\tSTB       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%02X \'%c\'\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint8_t) src, (uint8_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_BYTE, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint8_t) src;
#endif
    }
    return OK;
}

/// STH - Store Halfword to Memory
ReturnStatus_t
C62xSTH_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint16_t *ptr = (uint16_t *) FindMemoryAddress(p_state, rb, idx_rb, ra, mode, HWORD_ALIGN);

        TRACE_PRINT("%08x\tSTH       %s,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%04X\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint16_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_HWORD, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint16_t) src;
#endif
    }
    return OK;
}

ReturnStatus_t
C62xSTH_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint16_t *ptr = (uint16_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, HWORD_ALIGN);

        TRACE_PRINT("%08x\tSTH       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%04X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint16_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_HWORD, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint16_t) src;
#endif
    }
    return OK;
}

ReturnStatus_t
C62xSTH_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint16_t *ptr = (uint16_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, HWORD_ALIGN);

        TRACE_PRINT("%08x\tSTH       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%04X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint16_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_HWORD, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint16_t) src;
#endif
    }
    return OK;
}

/// STW - Store Word to Memory
ReturnStatus_t
C62xSTW_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra   = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint32_t *ptr = (uint32_t *) FindMemoryAddress(p_state, rb, idx_rb, ra, mode, WORD_ALIGN);

        TRACE_PRINT("%08x\tSTW       %s,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%08X\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint32_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_WORD, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint32_t) src;
#endif
    }
    return OK;
}

ReturnStatus_t
C62xSTW_UC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint32_t *ptr = (uint32_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, WORD_ALIGN);

        TRACE_PRINT("%08x\tSTW       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%08X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint32_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_WORD, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint32_t) src;
#endif
    }
    return OK;
}

ReturnStatus_t
C62xSTW_UC15_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t mode, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t rb   = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t src  = (uint32_t) p_state->m_reg[idx_rd];
        uint32_t amr  = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        uint32_t *ptr = (uint32_t *) FindMemoryAddress(p_state, rb, idx_rb, constant, mode, WORD_ALIGN);

        TRACE_PRINT("%08x\tSTW       0x%x,%s,%s\t\tMEM[0x%08X] = 0x%08X <= 0x%08X\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd), ptr, *((uint32_t *) ptr), (uint32_t) src);

        // Now we should have a valid address (mode-wise).
#ifdef DELAYED_MWBS
        // Put Value in the Write Back Buffer.
        if(EnQ_MWB(p_state, MWB_WORD, (uint32_t) ptr, src, delay))
        {
           ASSERT(1, "Error in Add Memory Write Back\n");
        }
#else
        * ptr = (uint32_t) src;
#endif
    }
    return OK;
}

/// SUB - Subtract Two Signed Integers Without Saturation
ReturnStatus_t
C62xSUB_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];
        int32_t rd = ra - rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUB       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSUB_SR32_SR32_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb = (int32_t) p_state->m_reg[idx_rb];
        int64_t rd = ra - rb;

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSUB       %s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSUB_SC5_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = C6XSC5_TO_S32(constant);
        int32_t rb  = (int32_t) p_state->m_reg[idx_rb];
        int32_t rd  = ra - rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUB       0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSUB_SC5_SR40_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rbh, uint16_t idx_rbl, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int64_t ra  = C6XSC5_TO_S64(constant);
        int32_t rbh = (int32_t) p_state->m_reg[idx_rbh];
        int32_t rbl = (int32_t) p_state->m_reg[idx_rbl];

        int64_t rb  = C6X40_TO_S64(rbh, rbl);
        int64_t rd  = ra - rb;

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSUB       0x%x,%s:%s,%s:%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rbh), REG(idx_rbl), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

ReturnStatus_t
C62xSUB_SR32_SC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb  = C6XSC5_TO_S32(constant);
        int32_t rd  = ra - rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUB       %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSUB_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra  = (int32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) constant & 0x1F;
        int32_t rd  = ra - rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUB       %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSUB_SR40_SC5_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint16_t idx_rah, uint16_t idx_ral, uint32_t constant, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t rah = (int32_t) p_state->m_reg[idx_rah];
        int32_t ral = (int32_t) p_state->m_reg[idx_ral];
        int64_t ra  = C6X40_TO_S64(rah, ral);
        int64_t rb  = C6XSC5_TO_S64(constant);

        int64_t rd  = ra - rb;

        int32_t rdh = U64_TO_C6XMSB12(rd);
        int32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSUB       %s:%s,0x%x,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_rah), REG(idx_ral), constant, REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// SUBAB - Subtract Using Byte Addressing Mode
ReturnStatus_t
C62xSUBAB_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = (int32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra - rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUBAB     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSUBAB_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = constant & 0x1F;
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra - rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUBAB     %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

/// SUBAH - Subtract Using Halfword Addressing Mode
ReturnStatus_t
C62xSUBAH_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = (int32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra - (rb << 1);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUBAH     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSUBAH_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = constant & 0x1F;
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra - (rb << 1);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUBAH     %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

/// SUBAW - Subtract Using Word Addressing Mode
ReturnStatus_t
C62xSUBAW_SR32_SR32_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                         uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t ra   = (int32_t) p_state->m_reg[idx_ra];
        int32_t rb   = (int32_t) p_state->m_reg[idx_rb];
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra - (rb << 2);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUBAW     %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xSUBAW_SR32_UC5_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint32_t constant, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        int32_t  ra  = (int32_t) p_state->m_reg[idx_ra];
        uint32_t rb  = constant & 0x1F;
        uint32_t amr = (uint32_t) p_state->m_reg[REG_AMR_INDEX];

        ASSERT(amr == 0x0, "AMR Register Indicates Circular Addressing Mode");

        int32_t rd = ra - (rb << 2);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUBAW     %s,0x%x,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), constant, REG(idx_rd));
    }
    return OK;
}

/// SUBC - Subtract Conditionally and Shift—Used for Division
/*
 * Subtract src2 from src1. If result is greater than or equal to 0, left shift result by 1, add 1
 * to it, and place it in dst. If result is less than 0, left shift src1 by 1, and place it in dst. This
 * step is commonly used in division.
 */
ReturnStatus_t
C62xSUBC_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];

        int32_t  tmp = ra - rb;
        uint32_t rd  = 0;

        if(tmp >= 0)
            rd = (tmp << 1) + 1;
        else
            rd = ra << 1;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUBC      %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// SUBU - Subtract Two Unsigned Integers Without Saturation
ReturnStatus_t
C62xSUBU_UR32_UR32_UR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                       uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];

        uint64_t rd = ra - rb;

        uint32_t rdh = U64_TO_C6XMSB12(rd);
        uint32_t rdl = U64_TO_C6XLSB32(rd);

        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, rdh, idx_rdl, rdl);

        TRACE_PRINT("%08x\tSUBU      %s,%s,%s:%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}

/// SUB2 - Subtract Two 16-Bit Integers on Upper and Lower Register Halves
/*
 * The upper and lower halves of src2 are subtracted from the upper and lower halves of
 * src1 and the result is placed in dst. Any borrow from the lower-half subtraction does not
 * affect the upper-half subtraction. Specifically, the upper-half of src2 is subtracted from
 * the upper-half of src1 and placed in the upper-half of dst. The lower-half of src2 is
 * subtracted from the lower-half of src1 and placed in the lower-half of dst.
 */
ReturnStatus_t
C62xSUB2_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (int32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (int32_t) p_state->m_reg[idx_rb];

        uint16_t rah = GET_MSB16(ra);
        uint16_t ral = GET_LSB16(ra);
        uint16_t rbh = GET_MSB16(rb);
        uint16_t rbl = GET_LSB16(rb);

        uint32_t rd = ((rah - rbh) << 16) | (ral - rbl);

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tSUB2       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// XOR - Bitwise Exclusive OR
/*
 * Performs a bitwise exclusive-OR (XOR) operation between src1 and src2. The result is
 * placed in dst. The scst5 operands are sign extended to 32 bits.
 */
ReturnStatus_t
C62xXOR_UR32_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                        uint16_t idx_ra, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = (uint32_t) p_state->m_reg[idx_ra];
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = ra ^ rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tXOR       %s,%s,%s\n",
                Get_DSP_PC(p_state), REG(idx_ra), REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xXOR_SC5_UR32_UR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
                      uint32_t constant, uint16_t idx_rb, uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        uint32_t ra = C6XSC5_TO_S32(constant);
        uint32_t rb = (uint32_t) p_state->m_reg[idx_rb];
        uint32_t rd = ra ^ rb;

        SAVE_REGISTER_RESULT(result, idx_rd, rd);

        TRACE_PRINT("%08x\tXOR       0x%x,%s,%s\n",
                Get_DSP_PC(p_state), constant, REG(idx_rb), REG(idx_rd));
    }
    return OK;
}

/// ZERO - Zero a Register
ReturnStatus_t
C62xZERO_SR32(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
              uint16_t idx_rd, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        SAVE_REGISTER_RESULT(result, idx_rd, 0);

        TRACE_PRINT("%08x\tZERO      %s\n", Get_DSP_PC(p_state), REG(idx_rd));
    }
    return OK;
}

ReturnStatus_t
C62xZERO_SR40(C62x_DSPState_t * p_state, uint8_t is_cond, uint8_t be_zero, uint16_t idx_rc,
              uint16_t idx_rdh, uint16_t idx_rdl, uint8_t delay, C62x_Result_t * result)
{
    if(Check_Predicate(p_state, is_cond, be_zero, idx_rc))
    {
        SAVE_MULTIREGISTER_RESULT(result, idx_rdh, 0x0, idx_rdl, 0x0);

        TRACE_PRINT("%08x\tZERO      %s:%s\n", Get_DSP_PC(p_state), REG(idx_rdh), REG(idx_rdl));
    }
    return OK;
}
