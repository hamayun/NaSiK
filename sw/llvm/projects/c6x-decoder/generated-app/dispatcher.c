
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

#include "C62xISABehavior.h"

extern int Init_DSP_State(C62x_DSPState_t * p_state);
extern int Print_DSP_State(C62x_DSPState_t * p_state);

extern address_entry_t AddressingTable[];

int main(int argc, char **argv, char **environ)
{
//    CPU_PROFILE_VERIFY_MEMORY();
    int (*curr_func)(C62x_DSPState_t * p_state) = NULL;

    C62x_DSPState_t p_state;
    Init_DSP_State(& p_state);

    for(int index = 0; index < 8; index++)
    {
       curr_func = AddressingTable[index].func_address;
       curr_func(& p_state);
    }

    Print_DSP_State(& p_state);

    while (1);
    return 0;
}

