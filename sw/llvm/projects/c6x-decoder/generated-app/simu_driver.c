
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

#include "C62xISABehavior.h"

extern int Init_DSP_State(C62x_DSPState_t * p_state);
extern int Print_DSP_State(C62x_DSPState_t * p_state);

extern address_entry_t AddressingTable[];
extern uint32_t AddressingTableSize;

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

    while(1)
    {
        curr_func = find_sim_func(*p_state.p_pc);
        if(curr_func)
        {
            ret_val = curr_func(& p_state);
            printf("Return Value = 0x%x\n", ret_val);
            Print_DSP_State(& p_state);
        }
        else
            ASSERT(0, "Native Simulation Functin Not Found");
    }
    
    return 0;
}

