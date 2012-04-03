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

extern uint32_t ENTRY_POINT_PC;
extern uint32_t CIOBUFF_ADDR;
extern void platform_console_puts_len (char * string, int len);

#ifdef ENABLE_STATS
uint32_t total_ep_count = 0;
uint32_t total_bb_count = 0;
uint32_t total_lmap_search = 0;
uint32_t total_lmap_found = 0;
uint32_t total_lmap_extraloops = 0;
#endif

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
    p_state->m_reg[REG_PC_INDEX] = ENTRY_POINT_PC;
    p_state->m_reg[REG_CSR_INDEX] = 0xC020100;         // Taken From C6x Simulator

    for(index = 0; index < (C62X_MAX_DELAY_SLOTS + 1); index++)
    {
#ifdef QUEUE_BASED_DREGS
        if(Init_Delay_Reg_Queue(& p_state->m_delay_q[index]))
        {
            printf("Error: Initializing Delay Queue [%d]\n", index);
            return (-1);
        }
#endif

#ifdef DELAYED_MWBS
        if(Init_MWB_Queue(& p_state->m_mwback_q[index]))
        {
            printf("Error: Initializing Write Back Queue [%d]\n", index);
            return (-1);
        }
#endif
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

#ifdef DELAYED_MWBS
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
#endif

    printf("TRACE PCE1 [%08x] ", p_state->m_reg[REG_PC_INDEX]);
#ifdef PRINT_CYCLES
    printf("%7s Cycles Completed: %016lld\n\n", "", p_state->m_curr_cycle);
#else
    printf("\n\n");
#endif

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

#ifdef DELAYED_MWBS
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
#endif

void Print_DSP_Stack(C62x_DSPState_t * p_state, uint32_t stack_start)
{
    uint32_t stack_ptr  = p_state->m_reg[REG_SP_INDEX];
    uint32_t stack_size = stack_start - stack_ptr + 4;
    uint32_t stack_val  = 0x0;

    printf("Stack Start: 0x%x Pointer: 0x%x Size: 0x%x\n", stack_start, stack_ptr, stack_size);

    while(stack_ptr <= stack_start)
    {
        stack_val =  *((uint32_t *) stack_ptr);
        printf("[%x] = 0x%x\n", stack_ptr, stack_val);

        stack_ptr += 4;
    }
    return;
}

void DSP_Dump_Memory(uint32_t start_addr, uint32_t size)
{
    uint32_t curr_addr = start_addr;
    uint32_t end_addr = start_addr + size;

    printf("Memory Dump: From 0x%x To: 0x%x\n", start_addr, end_addr);

    while(curr_addr < end_addr)
    {
        printf("[%x] = 0x%x\n", curr_addr, *((uint32_t *) curr_addr));
        curr_addr += 4;
    }
    return;
}

void DSP_Flush_CIO()
{
    uint32_t io_len = *((uint32_t *) CIOBUFF_ADDR); // _CIOBUF_ Address
    char * io_str = ((char *) CIOBUFF_ADDR) + 0xD;

    if(io_len)
    {
        platform_console_puts_len(io_str, io_len);
        *((uint32_t *)(CIOBUFF_ADDR + 0x4)) = io_len;
    }
    else
    {
        *((uint32_t *)(CIOBUFF_ADDR + 0x4)) = 0xFFFFFFFF;
    }

    *((uint32_t *)(CIOBUFF_ADDR + 0x0)) = 0x0;
    *((uint32_t *)(CIOBUFF_ADDR + 0x8)) = 0x0;
    return;
}

