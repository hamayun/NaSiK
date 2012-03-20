
#include <stdio.h>

#include <Processor/IO.h>
#include <Processor/Profile.h>

#include "C62xISABehavior_v2.h"

extern address_entry_t AddressTable[];
extern uint32_t AddressTableSize;

extern uint32_t EXIT_POINT_PC;
extern uint32_t CIOFLUSH_POINT_PC;
extern uint32_t CIOBUFF_ADDR;

extern void Print_DSP_Stack(C62x_DSPState_t * p_state, uint32_t stack_start);
extern void DSP_Dump_Memory(uint32_t start_addr, uint32_t size);
extern void DSP_Flush_CIO();

typedef int (*sim_func_t)(C62x_DSPState_t * p_state);

#ifdef ENABLE_STATS
    uint32_t sim_func_calls     = 0;
    uint64_t total_loop_count   = 0;
    uint32_t search_loop_min    = 0;
    uint32_t search_loop_max    = 0;
    uint32_t total_mem_access   = 0;
    extern uint32_t total_ep_count;
    extern uint32_t total_bb_count;
#endif

sim_func_t find_sim_func(uint32_t target_pc)
{
    sim_func_t sim_func = NULL;
    uint32_t low = 0, high = AddressTableSize - 1;

#ifdef ENABLE_STATS
    uint32_t curr_count = 0;
    sim_func_calls++;
#endif

    //printf("Searching for Target PC = 0x%08x\n", target_pc);

    while(low <= high)
    {
        uint32_t mid_index = (low + high) / 2;
        uint32_t mid_addr  = AddressTable[mid_index].m_address;

#ifdef ENABLE_STATS
        total_mem_access++;
#endif

        if(mid_addr == target_pc)
        {
#ifdef ENABLE_STATS
            if(curr_count < search_loop_min) search_loop_min = curr_count;
            if(curr_count > search_loop_max) search_loop_max = curr_count;
#endif
            sim_func = AddressTable[mid_index].func_address;
            break;
        }
        else if(mid_addr < target_pc)
            low = mid_index + 1;
        else
            high = mid_index - 1;

#ifdef ENABLE_STATS
        curr_count++;
        total_loop_count++;
#endif
    }

    return (sim_func);
}

int main(int argc, char **argv, char **environ)
{
    sim_func_t curr_func = NULL;
    sim_func_t next_func_ptr = NULL;

    C62x_DSPState_t p_state;
    Init_DSP_State(& p_state);

    CPU_PROFILE_COMP_START();
    while(1)
    {
        if(next_func_ptr == NULL)
            curr_func = find_sim_func(p_state.m_reg[REG_PC_INDEX]);
        else
            curr_func = next_func_ptr;

        if(curr_func)
        {
            next_func_ptr = (sim_func_t) curr_func(& p_state);
            //Print_DSP_State(& p_state);
            //printf("Next Function Pointer: 0x%08X\n", next_func_ptr);
        }
#if 0
        else
        {
            printf("Target Program Counter = 0x%X\n", p_state.m_reg[REG_PC_INDEX]);
            ASSERT(0, "Native Simulation Function Not Found");
        }
#endif

        if(p_state.m_reg[REG_PC_INDEX] == CIOFLUSH_POINT_PC)
        {
            DSP_Flush_CIO();
        }

        if(p_state.m_reg[REG_PC_INDEX] == EXIT_POINT_PC || !curr_func)
        {
            CPU_PROFILE_COMP_END();
            CPU_PROFILE_FLUSH_DATA();
            printf("EXIT_POINT_PC (0x%08X) Reached in %lld Cycles\n", EXIT_POINT_PC, p_state.m_curr_cycle);

#ifdef ENABLE_STATS
            printf("*----- Simulation Driver Statistics -----*\n");
            printf("Find Function Calls           : %d times \n", sim_func_calls);
            printf("Total Loop Count              : %ld times \n", total_loop_count);
            printf("Search Loop Min/Max/Avg       : %d/%d/%.3f times/call\n",
                    search_loop_min, search_loop_max, ((float) total_loop_count / sim_func_calls));
            printf("Total Mem Access Count        : %ld times \n", total_mem_access);
            printf("Avg Mem Access Count          : %.3f times/call \n", ((float) total_mem_access/sim_func_calls));
            printf("Total EPs Executed            : %ld (%.5f%%)\n", total_ep_count, (100.0 * total_ep_count/(total_ep_count+total_bb_count)));
            printf("Total BBs Executed            : %ld (%.5f%%)\n", total_bb_count, (100.0 * total_bb_count/(total_ep_count+total_bb_count)));
            printf("Find Function Calls == EP+BB ?: %s\n", ((total_bb_count+total_ep_count) == sim_func_calls) ? "YES" : "NO");
            printf("*-----------------------------------------*\n");
#endif
            // Halt the KVM CPU
            __asm__ volatile("hlt");
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

