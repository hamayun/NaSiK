
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

#include "C62xISABehavior.h"

extern int Init_DSP_State(C62x_DSPState_t * p_state);
extern int Print_DSP_State(C62x_DSPState_t * p_state);

extern int SimEP_00000000 (C62x_DSPState_t * p_state);
extern int SimEP_00000004 (C62x_DSPState_t * p_state);
extern int SimEP_00000008 (C62x_DSPState_t * p_state);
extern int SimEP_0000000c (C62x_DSPState_t * p_state);
extern int SimEP_00000010 (C62x_DSPState_t * p_state);
extern int SimEP_00000014 (C62x_DSPState_t * p_state);
extern int SimEP_00000018 (C62x_DSPState_t * p_state);
extern int SimEP_0000001c (C62x_DSPState_t * p_state);

address_entry_t addr_table [8] =
{
    { 0x00000000, SimEP_00000000 },
    { 0x00000004, SimEP_00000004 },
    { 0x00000008, SimEP_00000008 },
    { 0x0000000c, SimEP_0000000c },
    { 0x00000010, SimEP_00000010 },
    { 0x00000014, SimEP_00000014 },
    { 0x00000018, SimEP_00000018 },
    { 0x0000001c, SimEP_0000001c },
};

int main(int argc, char **argv, char **environ)
{
//    CPU_PROFILE_VERIFY_MEMORY();
    int (*curr_func)(C62x_DSPState_t * p_state) = NULL;

    C62x_DSPState_t p_state;
    Init_DSP_State(& p_state);

    for(int index = 0; index < 8; index++)
    {
       curr_func = addr_table[index].func_address;
       curr_func(& p_state);
    }

    Print_DSP_State(& p_state);

    while (1);
    return 0;
}

