
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

//#include "C62xISABehavior.h"

#define C62X_REG_BANKS          3
#define C62X_REGS_PER_BANK      16
#define C62X_BRANCH_DELAY       0x5
#define C62X_MAX_DELAY_SLOTS    C62X_BRANCH_DELAY

typedef struct C62x_Delay_Node
{
    uint16_t                 m_reg_id;
    uint32_t                 m_value;
    struct C62x_Delay_Node * m_next_node;
} C62x_Delay_Node_t;
/* LLVM Type ... { i16, i32, \2 } */

typedef struct C62x_Delay_Queue
{
    C62x_Delay_Node_t      * m_head_node;
    C62x_Delay_Node_t      * m_tail_node;
    uint32_t                 m_num_busy_nodes;
    uint32_t                 m_max_busy_nodes;
} C62x_Delay_Queue_t;
/* LLVM Type ... { { i16, i32, \2 }*, { i16, i32, \2 }*, i32, i32 } */

typedef enum C62xAlignment
{
    BYTE_ALIGN  = 0,
    HWORD_ALIGN = 1,
    WORD_ALIGN  = 2
} C62xAlignment_t;

typedef enum C62xMWB_Size
{
    MWB_BYTE  = 1,
    MWB_HWORD = 2,
    MWB_WORD  = 4
} C62xMWB_Size_t;

/* Memory Write-Back Node */
typedef struct C62x_MWBack_Node
{
    C62xMWB_Size_t                m_size;     /* MWB_BYTE, MWB_HWORD or MWB_WORD */
    uint32_t                      m_addr;
    uint32_t                      m_value;
    struct C62x_MWBack_Node     * m_next_node;
} C62x_MWBack_Node_t;

typedef struct C62x_MWB_Queue
{
    C62x_MWBack_Node_t          * m_head_node;
    C62x_MWBack_Node_t          * m_tail_node;
    uint32_t                      m_is_empty;
} C62x_MWB_Queue_t;

typedef struct C62x_DSPState
{
    uint64_t                      m_curr_cycle;
    uint32_t                    * p_pc;
    uint32_t                      m_register[C62X_REG_BANKS * C62X_REGS_PER_BANK];
    C62x_Delay_Queue_t            m_delay_q[C62X_MAX_DELAY_SLOTS + 1];
    C62x_MWB_Queue_t              m_mwback_q[C62X_MAX_DELAY_SLOTS + 1];
} C62x_DSPState_t;
/* LLVM Type ... { i64, i32*, [48 x i32], [6 x { { i16, i32, \2 }*, { i16, i32, \2 }*, i32, i32 } ... ] } */


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

typedef struct func_table_entry
{
    uint32_t m_address;
    int (*func_address)(C62x_DSPState_t * p_state);
} func_table_entry_t;

func_table_entry_t func_table [8] =
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
       curr_func = func_table[index].func_address;
       curr_func(& p_state);
    }

    Print_DSP_State(& p_state);

    while (1);
    return 0;
}

