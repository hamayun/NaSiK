
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

#ifdef C62x_ISA_VER2
#include "C62xISABehavior_v2.h"
#else
#include "C62xISABehavior.h"
#endif

extern address_entry_t AddressingTable[];
extern uint32_t AddressingTableSize;
extern uint32_t EXIT_POINT_PC;
extern uint32_t CIOFLUSH_POINT_PC;
extern uint32_t CIOBUFF_ADDR;

extern void Print_DSP_Stack(C62x_DSPState_t * p_state, uint32_t stack_start);
extern void DSP_Dump_Memory(uint32_t start_addr, uint32_t size);
extern void DSP_Flush_CIO();

typedef int (*sim_func_t)(C62x_DSPState_t * p_state);

sim_func_t find_sim_func(uint32_t target_pc)
{
    sim_func_t sim_func = NULL;
    uint32_t low = 0, high = AddressingTableSize - 1;

    while(low <= high)
    {
        uint32_t mid_index = (low + high) / 2;
        uint32_t mid_addr  = AddressingTable[mid_index].m_address;

        if(mid_addr == target_pc)
        {
            sim_func = AddressingTable[mid_index].func_address;
            break;
        }
        else if(mid_addr < target_pc)
            low = mid_index + 1;
        else
            high = mid_index - 1;
    }

    return (sim_func);
}


int main(int argc, char **argv, char **environ)
{
//    CPU_PROFILE_VERIFY_MEMORY();
    sim_func_t curr_func = NULL;
    uint32_t ret_val = 0;

    C62x_DSPState_t p_state;
    Init_DSP_State(& p_state);
    //DSP_Dump_Memory(0x8EC0, 0x530);

    while(1)
    {
        curr_func = find_sim_func(p_state.m_reg[REG_PC_INDEX]);
        if(curr_func)
        {
            ret_val = curr_func(& p_state);
            //Print_DSP_State(& p_state);
        }
        else
        {
            printf("Target Program Counter = 0x%X\n", p_state.m_reg[REG_PC_INDEX]);
            ASSERT(0, "Native Simulation Function Not Found");
        }

        if(p_state.m_reg[REG_PC_INDEX] == CIOFLUSH_POINT_PC)
        {
            DSP_Flush_CIO();
        }

        if(p_state.m_reg[REG_PC_INDEX] == EXIT_POINT_PC)
        {
            printf("EXIT_POINT_PC (0x%08X) Reached\n", EXIT_POINT_PC);
            ASSERT(0, "Simulation Stopped ... !!!");
        }

#if 0
        if(p_state.m_curr_cycle >= 4100)
        {
            DSP_Dump_Memory(CIOBUFF_ADDR, 36);
        }
#endif
    }
    
    return 0;
}

