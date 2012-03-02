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

#ifdef C62x_ISA_VER2
#include "C62xISABehavior_v2.h"
#else
#include "C62xISABehavior.h"
#endif

#include "stdio.h"

extern uint32_t STARTUP_PC;

char * BANKC_REGS[] = {"AMR", "CSR", "ISR", "ICR", "IER", "ISTP", "IRP", "NRP",
                       "C8", "C9", "C10", "C11", "C12", "C13", "C14", "PCE1"};

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

int32_t Init_DSP_State(C62x_DSPState_t * p_state)
{
    uint32_t index;

    memset((void *) & p_state->m_reg[0], 0x0, sizeof(p_state->m_reg));

    // Setup Start Program Counter & Cycle Counter
    p_state->m_curr_cycle = 0;
    p_state->m_reg[REG_PC_INDEX] = STARTUP_PC;
    p_state->m_reg[REG_CSR_INDEX] = 0x40010100;         // Taken From C6x Simulator

    for(index = 0; index < (C62X_MAX_DELAY_SLOTS + 1); index++)
    {
#ifdef QUEUE_BASED_DREGS
        if(Init_Delay_Reg_Queue(& p_state->m_delay_q[index]))
        {
            printf("Error: Initializing Delay Queue [%d]\n", index);
            return (-1);
        }
#endif
        if(Init_MWB_Queue(& p_state->m_mwback_q[index]))
        {
            printf("Error: Initializing Write Back Queue [%d]\n", index);
            return (-1);
        }
    }

    for(index = 0; index < C62X_REG_BANKS; index++)
        BANKS[index] = 'A' + index;

    return (0);
}

int32_t Print_DSP_State(C62x_DSPState_t * p_state)
{
    uint32_t reg_id, index;

    for(reg_id = 0; reg_id < (C62X_REG_BANKS * C62X_REGS_PER_BANK); reg_id++)
    {
        printf("%5s=0x%08x", REG(reg_id), p_state->m_reg[reg_id]);
        if((reg_id + 1) % 4 == 0) printf("\n");
        if((reg_id + 1) % C62X_REGS_PER_BANK == 0) printf("\n");
    }

#ifdef QUEUE_BASED_DREGS
    for(index = 0; index < (C62X_MAX_DELAY_SLOTS + 1); index++)
    {
        C62x_Delay_Queue_t * delay_queue = & p_state->m_delay_q[index];
        C62x_Delay_Node_t    * curr_node = delay_queue->m_head_node;

        if(p_state->m_curr_cycle % (C62X_MAX_DELAY_SLOTS + 1) == index)
            printf("->");
        else
            printf("  ");

        printf("DRegQ[%d]: { ", index);
        while(curr_node != delay_queue->m_tail_node)
        {
            printf("%4s=%08x ", REG(curr_node->m_reg_id), curr_node->m_value);
            curr_node = curr_node->m_next_node;
        }
        printf("}\n");
    }
#else
    for(index = 0; index < (C62X_MAX_DELAY_SLOTS + 1); index++)
    {
        C62x_Delay_Queue_t * delay_queue = & p_state->m_delay_q[index];
        uint32_t              curr_index = delay_queue->m_head_idx;
        uint32_t              tail_index = delay_queue->m_tail_idx;

        if(p_state->m_curr_cycle % (C62X_MAX_DELAY_SLOTS + 1) == index)
            printf("->");
        else
            printf("  ");

        printf("DRegQ[%d]: { ", index);
        while(curr_index != tail_index)
        {
            printf("%4s=%08x ", REG(delay_queue->m_nodes[curr_index].m_reg_id), delay_queue->m_nodes[curr_index].m_value);
            curr_index = (curr_index + 1) % DELAY_QUEUE_SIZE;
        }
        printf("}\n");
    }
#endif

    for(index = 0; index < (C62X_MAX_DELAY_SLOTS + 1); index++)
    {
        C62x_MWB_Queue_t   * mwb_queue = & p_state->m_mwback_q[index];
        C62x_MWBack_Node_t * curr_node = mwb_queue->m_head_node;

        if(p_state->m_curr_cycle % (C62X_MAX_DELAY_SLOTS + 1) == index)
            printf("->");
        else
            printf("  ");

        printf("MWB-Q[%d]: { ", index);
        while(curr_node != mwb_queue->m_tail_node)
        {
            switch(curr_node->m_size)
            {
                case MWB_BYTE:
                    *((uint8_t *) curr_node->m_addr) = (uint8_t) curr_node->m_value;
                    printf("Mem[%08X]=%02X ", curr_node->m_addr, (uint8_t) curr_node->m_value);
                    break;

                case MWB_HWORD:
                    *((uint16_t *) curr_node->m_addr) = (uint16_t) curr_node->m_value;
                    printf("Mem[%08X]=%04X ", curr_node->m_addr, (uint16_t) curr_node->m_value);
                    break;

                case MWB_WORD:
                    *((uint32_t *) curr_node->m_addr) = (uint32_t) curr_node->m_value;
                    printf("Mem[%08X]=%08X ", curr_node->m_addr, (uint32_t) curr_node->m_value);
                    break;

                default:
                    ASSERT(1, "Unknown Memory Write-Back Size");
            }

            curr_node = curr_node->m_next_node;
        }
        printf("}\n");
    }

    printf("TRACE PCE1 [%08x] ", p_state->m_reg[REG_PC_INDEX]);
    printf("%7s Cycles Completed: %016lld\n\n", "", p_state->m_curr_cycle);

    return (0);
}
#ifdef QUEUE_BASED_DREGS
int32_t Init_Delay_Reg_Queue(C62x_Delay_Queue_t * delay_queue)
{
    C62x_Delay_Node_t * curr_node = NULL;
    C62x_Delay_Node_t * prev_node = NULL;
    uint32_t          nodes_count = 0;

    delay_queue->m_head_node = NULL;
    delay_queue->m_tail_node = NULL;

    // Because Destination can be a Long Register in C62x: So 2 Times
    while(nodes_count < (C62X_MAX_DELAY_SLOTS * FETCH_PACKET_SIZE * 2))
    {
        curr_node = malloc(sizeof(C62x_Delay_Node_t));
        if(!curr_node)
        {
            printf("Error: Allocating Memory for Delay Node\n");
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
#endif

int32_t Init_MWB_Queue(C62x_MWB_Queue_t * mwb_queue)
{
    C62x_MWBack_Node_t * curr_node   = NULL;
    C62x_MWBack_Node_t * prev_node   = NULL;
    uint32_t             nodes_count = 0;

    mwb_queue->m_head_node = NULL;
    mwb_queue->m_tail_node = NULL;

    // Supposing that We can have a max of FETCH_PACKET_SIZE Memory Writes in each cycle
    while(nodes_count < FETCH_PACKET_SIZE)
    {
        curr_node = malloc(sizeof(C62x_MWBack_Node_t));
        if(!curr_node)
        {
            printf("Error: Allocating Memory for Write Back Node\n");
            return (-1);
        }

        if(!mwb_queue->m_head_node)
        {
            mwb_queue->m_head_node = curr_node;
            prev_node   = curr_node;
        }

        curr_node->m_next_node = mwb_queue->m_head_node;
        prev_node->m_next_node = curr_node;
        prev_node = curr_node;
        curr_node = NULL;
        nodes_count++;
    }

    mwb_queue->m_tail_node = mwb_queue->m_head_node;
    mwb_queue->m_is_empty = 1;
    return(0);
}
