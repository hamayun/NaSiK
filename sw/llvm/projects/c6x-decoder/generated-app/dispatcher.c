
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

    for(int index = 0; index < AddressingTableSize; index++)
    {
        if(AddressingTable[index].m_address == target_pc)
        {
            sim_func = AddressingTable[index].func_address;
            break;
        }
    }

    return (sim_func);
}

int main(int argc, char **argv, char **environ)
{
//    CPU_PROFILE_VERIFY_MEMORY();
    sim_func_t curr_func = NULL;

    C62x_DSPState_t p_state;
    Init_DSP_State(& p_state);

    while(1)
    {
        curr_func = find_sim_func(*p_state.p_pc);
        if(curr_func) 
            curr_func(& p_state);
        else
            ASSERT(0, "Native Simulation Functin Not Found");
    }

    // Print_DSP_State(& p_state);
    
    return 0;
}

