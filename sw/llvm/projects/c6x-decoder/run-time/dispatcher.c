
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

//#include "C62xISABehavior.h"

#define C62X_REG_BANKS          3
#define C62X_REGS_PER_BANK      16
#define C62X_BRANCH_DELAY       0x5
#define C62X_MAX_DELAY_SLOTS    C62X_BRANCH_DELAY

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


#if 0
extern int simulated_bb();

int main(int argc, char **argv, char **environ)
{
    CPU_PROFILE_VERIFY_MEMORY();

    simulated_bb();

    while (1);
    return 0;
}

#else

extern int InitProcessorState(C62x_Proc_State_t * proc_state);
extern int ShowProcessorState(C62x_Proc_State_t * proc_state);

extern int Simulate_EP00000000 (C62x_Proc_State_t * proc_state);
extern int Simulate_EP00000004 (C62x_Proc_State_t * proc_state);
extern int Simulate_EP0000000c (C62x_Proc_State_t * proc_state);
extern int Simulate_EP00000010 (C62x_Proc_State_t * proc_state);
extern int Simulate_EP00000014 (C62x_Proc_State_t * proc_state);
extern int Simulate_EP00000018 (C62x_Proc_State_t * proc_state);
extern int Simulate_EP0000001c (C62x_Proc_State_t * proc_state);

typedef struct func_table_entry
{
    uint32_t m_address;
    int (*func_address)(C62x_Proc_State_t * proc_state);
} func_table_entry_t;

func_table_entry_t func_table [7] =
{
    { 0x00000000, Simulate_EP00000000 },
    { 0x00000004, Simulate_EP00000004 },
    { 0x0000000c, Simulate_EP0000000c },
    { 0x00000010, Simulate_EP00000010 },
    { 0x00000014, Simulate_EP00000014 },
    { 0x00000018, Simulate_EP00000018 },
    { 0x0000001c, Simulate_EP0000001c },
};

int main(int argc, char **argv, char **environ)
{
    CPU_PROFILE_VERIFY_MEMORY();
    int (*curr_func)(C62x_Proc_State_t * proc_state) = NULL;

    C62x_Proc_State_t proc_state;
    InitProcessorState(& proc_state);

    for(int index = 0; index < 7; index++)
    {
       curr_func = func_table[index].func_address;

       curr_func(& proc_state); 
    }

    ShowProcessorState(& proc_state);

    while (1);
    return 0;
}
#endif


