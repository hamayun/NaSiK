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

#include "ExecutePacketList.h"
#include "BasicBlockList.h"

namespace native
{
    BasicBlockList :: BasicBlockList(ExecutePacketList * exec_packets_list)
    {
        ExecutePacketList_Iterator_t BEPI;        // Book Marked Execute Packet Iterator.
        bool         bookmark_flag      = false;  // Tells us whether we have a bookmarked execute packet or not
        BasicBlock * basic_block        = NULL;
        uint32_t     basic_block_id     = 0;
        int32_t      delay_slot_count   = 0;
        int32_t      branch_count       = 0;
        bool         branch_flag        = false;
        DecodedInstruction * dec_instr  = NULL;

        // Traverse through all the Execute Packets
        for(ExecutePacketList_Iterator_t EPLI = exec_packets_list->GetExecutePacketList()->begin(),
            EPLE = exec_packets_list->GetExecutePacketList()->end(); EPLI != EPLE; ++EPLI)
        {
            ExecutePacketList_Iterator_t NEPI = EPLI; NEPI++;       // Next Execute Packet Iterator
            // See if the next execute packet is not the last one and contains a branch target?
            if((NEPI != EPLE) && (*NEPI)->GetBranchTargetInstructionCount() && bookmark_flag == false)
            {
                // We will build the next Basic Block Starting at this Execute Packet
                bookmark_flag = true; BEPI = NEPI;
            }

            if(!basic_block)
            {
                basic_block = new BasicBlock(basic_block_id++);
                ASSERT(basic_block, "Allocating Memory for Basic Block");
            }

            // Add the current Execute Packet to Basic Block
            basic_block->PushBack(*EPLI);

            /* If the current execute packet contains at-least one Branch Instruction;
             * Then start counting Branch Delay Slots to find the end of the current Basic Block.
             * Attention: We can have multiple branch instructions within the Delay Slot Range,
             * of the previous Branch Instruction. This will be handled in LLVM Code Generation,
             * where we will check for Program Counter Modification in the Update_Registers Calls.
             */
            //branch_count = (*EPLI)->GetUnconditionalBranchInstructionsCount();
            branch_count = (*EPLI)->GetBranchInstructionsCount();
            if(branch_count)
            {
                branch_flag = true; delay_slot_count = 0;
            }

            if(branch_flag)
            {
                // Check if this Execute Packet Contains Any Multi-Cycle NOP Instruction.
                dec_instr = (*EPLI)->GetMutiCycleNOPInstr();
                if(dec_instr)
                    delay_slot_count += dec_instr->GetNOPCount();
                else
                    delay_slot_count++;
            }

            if(delay_slot_count > C62X_BRANCH_DELAY)
            {
                m_basic_blocks_list.push_back(basic_block);

                // Reset for processing the Next Basic Block
                basic_block = NULL; branch_flag = false; delay_slot_count = 0;

                if(bookmark_flag)
                {
                    bookmark_flag = false;
                    EPLI = --BEPI;  // Because we did an increment in the loop above
                }
            }
        }

        /*
         * This means that the Last Basic Block hasn't been pushed to the basic block list.
         * Due to the fact that it does not contain any branch instructions. Should be discarded !!!
         * But we push it on the Basic Block List, anyways.
         */
        if(basic_block)
        {
            m_basic_blocks_list.push_back(basic_block);
            basic_block = NULL;
            WARN << "Sink Basic Block Found" << endl;
        }

        //ASSERT(!AddMarkerExecutePackets(), "Failed to Add Marker Execute Packets");
    }

    uint32_t BasicBlockList :: RemoveRedundantEPs(ExecutePacketList * exec_pkts_list)
    {
        for(BasicBlockList_ConstIterator_t BBLCI = m_basic_blocks_list.begin(),
            BBLCE = m_basic_blocks_list.end(); BBLCI != BBLCE; ++BBLCI)
        {
            ExecutePacket * exec_pkt = (*BBLCI)->GetFirstExecPacket();
            exec_pkts_list->Remove(exec_pkt);
            //exec_pkt->Print(&cout);
        }

        return (0);
    }

    int32_t BasicBlockList :: AddMarkerExecutePackets()
    {
        for(BasicBlockList_Iterator_t BBLI = m_basic_blocks_list.begin(),
            BBLE = m_basic_blocks_list.end(); BBLI != BBLE; ++BBLI)
        {
            if((*BBLI)->AddMarkerExecutePackets())
            {
                return (-1);
            }
        }
        return(0);
    }
}

#if 0 /// Basic Block List Creation with Marker Packets
    BasicBlockList :: BasicBlockList(ExecutePacketList * exec_packets_list)
    {
        ExecutePacketList_Iterator_t BEPI;        // Book Marked Execute Packet Iterator.
        bool         bookmark_flag      = false;  // Tells us whether we have a bookmarked execute packet or not
        BasicBlock * basic_block        = NULL;
        uint32_t     basic_block_id     = 0;
        int32_t      delay_slot_count   = 0;
        int32_t      taken_branch_count = 0;
        bool         taken_branch_flag  = false;
        DecodedInstruction * dec_instr  = NULL;

        // Traverse through all instructions in the instruction list
        for(ExecutePacketList_Iterator_t EPLI = exec_packets_list->GetExecutePacketList()->begin(),
            EPLE = exec_packets_list->GetExecutePacketList()->end(); EPLI != EPLE; ++EPLI)
        {
            ExecutePacketList_Iterator_t NEPI = EPLI; NEPI++;       // Next Execute Packet Iterator
            // See if the next execute packet is not the last one and contains a branch target?
            if((NEPI != EPLE) && (*NEPI)->GetBranchTargetInstructionCount() && bookmark_flag == false)
            {
                // We will build the next Basic Block Starting at this Execute Packet
                bookmark_flag = true; BEPI = NEPI;
            }

            if(!basic_block)
            {
                basic_block = new BasicBlock(basic_block_id++);
                ASSERT(basic_block, "Allocating Memory for Basic Block");
            }

            // Add the current Execute Packet to Basic Block
            basic_block->PushBack(*EPLI);

            /* If the current execute packet contains at-least one Unconditional Branch Instruction;
             * Then start counting Branch Delay Slots to find the end of the current Basic Block.
             * Attention: We can have multiple branch instructions within the 'Delay Slot Range'
             * of the previous Branch Instruction. In such case we reset the Delay Slot Counter.
             */
            taken_branch_count = (*EPLI)->GetUnconditionalBranchInstructionsCount();
            if(taken_branch_count)
            {
                taken_branch_flag = true; delay_slot_count = 0;
            }

            if(taken_branch_flag)
            {
                delay_slot_count++;

                // If instruction count in this packet is 1, It could be an Idle 'NOP n' Instruction
                // If yes, then see how many delay slots it occupies.
                if((*EPLI)->GetSize() == 1)
                {
                    dec_instr = (*EPLI)->GetInstrByIndex(0)->GetDecodedInstruction();
                    ASSERT(dec_instr != NULL, "Instructions need to be decoded first.");
                    if(dec_instr->IsNOPInstruction())
                    {
                        uint8_t nop_count = dec_instr->GetNOPCount();
                        if(nop_count > 0)
                        {
                            // We already added one to delay slots above
                            delay_slot_count += nop_count - 1;
                        }
                    }
                }
            }

            if(delay_slot_count > C62X_BRANCH_DELAY)
            {
                m_basic_blocks_list.push_back(basic_block);
                // Reset for processing the Next Basic Block
                basic_block = NULL; taken_branch_flag = false; delay_slot_count = 0;

                if(bookmark_flag)
                {
                    bookmark_flag = false;
                    EPLI = --BEPI;  // Because we do an increment in the loop
                }
            }
        }

        /*
         * This means that the Last Basic Block hasn't been pushed to the basic block list.
         * Due to the fact that it does not contain any branch instructions. Should be discarded !!!
         * But we push it on the Basic Block List; To be on the safe side
         */
        if(basic_block)
        {
            m_basic_blocks_list.push_back(basic_block);
            basic_block = NULL;
            WARN << "Sink Basic Block Found" << endl;
        }

        ASSERT(!AddMarkerExecutePackets(), "Failed to Add Marker Execute Packets");
    }
#endif


#if 0
class basic_block
{
private:
    uint32_t             m_id;
    uint32_t             m_base_addr;
    uint32_t             m_next_bb_addr;        // Address of Next BB; For Fallthrough

public:
    list <execute_packet*>   execpkt_list;
    set <basic_block*>       pred_set;
    set <basic_block*>       succ_set;

    basic_block(uint32_t id){
        m_id = id;
        m_base_addr = 0;
    }
    ~basic_block(){}

    uint32_t get_id()
    {
        return (m_id);
    }

    uint32_t get_base_addr()
    {
        return(m_base_addr);
    }

    void set_next_bb_addr(uint32_t next_bb_addr)
    {
        m_next_bb_addr = next_bb_addr;
    }

    uint32_t get_next_bb_addr()
    {
        return(m_next_bb_addr);
    }

    int32_t get_nm_expkts()
    {
        return(execpkt_list.size());
    }

    int32_t get_nm_instrs()
    {
        int32_t nm_instr = 0;

        for(list<execute_packet*>::iterator pcurexp = execpkt_list.begin(),
            plastexp = execpkt_list.end(); pcurexp != plastexp; pcurexp++)
        {
            nm_instr += (*pcurexp)->get_instr_count();
        }
        return(nm_instr);
    }

    int32_t get_branch_instr_count()
    {
        int32_t nm_br_instr = 0;

        for(list<execute_packet*>::iterator pcurexp = execpkt_list.begin(),
            plastexp = execpkt_list.end(); pcurexp != plastexp; pcurexp++)
        {
            nm_br_instr += (*pcurexp)->get_branch_instr_count();
        }
        return(nm_br_instr);
    }

    execute_packet* get_execute_packet(uint32_t id)
    {
        uint32_t index = 0;
        list<execute_packet*>::iterator EPI = execpkt_list.begin();

        while (index < execpkt_list.size())
        {
            if(index == id)
                return (*EPI);

            ++EPI; ++index;
        }

        return (NULL);
    }

    void push_back(execute_packet * pexpkt)
    {
        if(execpkt_list.size() == 0)
            m_base_addr = pexpkt->get_addr();

        execpkt_list.push_back(pexpkt);
    }

    void print(ostream *out, int32_t mode)
    {
#ifdef PRINT_BB_INFO
        (*out) << "BB" << m_id
               << "\t\tAddress:" << FMT_INT << m_base_addr
               << "\t(" << dec << get_nm_expkts() << " Packets, "
               << get_nm_instrs() << " Instructions)";

        (*out) << "\nPredecessors: ";
        for (set <basic_block*> :: iterator PDI = pred_set.begin(), PDE = pred_set.end(); PDI != PDE; ++PDI)
        {
            (*out) << "BB" << dec <<(*PDI)->get_id() << " (" << FMT_INT << (*PDI)->get_base_addr() << ") ";
        }

        (*out) << "\nSuccessors  : ";
        for (set <basic_block*> :: iterator SCI = succ_set.begin(), SCE = succ_set.end(); SCI != SCE; ++SCI)
        {
            (*out) << "BB" << dec <<(*SCI)->get_id() << " (" << FMT_INT << (*SCI)->get_base_addr() << ") ";
        }

        (*out) << endl;
#endif

        for(list<execute_packet*>::iterator pcurexp = execpkt_list.begin(),
            plastexp = execpkt_list.end(); pcurexp != plastexp; pcurexp++)
        {
            (*pcurexp)->print(out, mode);
#ifdef PRINT_BB_INFO
            (*out) << "--------------------------------------------------------------------------------" << endl;
#endif
        }
    }
};

class basic_block_list
{
private:
    list <basic_block*> bblist;
    int32_t check_branch_targets(execute_packet_list * pexplist);
    int32_t add_pred_succ_cfg_info();
    int32_t find_function_calls();

public:
    basic_block_list(execute_packet_list * explist);
    ~basic_block_list();

    basic_block * find_basic_block(uint32_t base_addr);
    basic_block * get_basic_block(uint32_t id);
    void print(ostream *out, int32_t mode)
    {
        for(list<basic_block*>::iterator pcurbb = bblist.begin(),
            plastbb = bblist.end(); pcurbb != plastbb; pcurbb++)
        {
                (*pcurbb)->print(out, mode);
#ifdef PRINT_BB_INFO
                (*out) << endl;
#endif
        }
    }
};

// class basic_block_list
basic_block_list :: basic_block_list(execute_packet_list * explist)
{
    basic_block *pbb    = NULL;
    uint32_t bbnum  = 0;
    int32_t  branch_instr_count = 0;
    int32_t  branch_instr_flag = 0;     // To indicate that we have found
                                            // atleast one branch instruction in
                                            // the current execute packet
    int32_t  is_br_target = 0;
    int32_t  delay_slot_count = 0;

    for(list<execute_packet*>::iterator pcurexp = explist->get_list()->begin(),
        pnextexp = pcurexp, plastexp = explist->get_list()->end();
        pcurexp != plastexp; pcurexp++)
    {
        if(!pbb)
        {
            pbb = new basic_block(bbnum++);
            if(!pbb)
            {
                DOUT << "Error: Allocating Memory for Basic Block" << endl;
                return;
            }
        }

        pbb->push_back(*pcurexp);
        // Find How many Branch Instructions are in this Execute Packet?
        branch_instr_count = (*pcurexp)->get_branch_instr_count();
        // If the Current Execute Packet contains at-least one branch instruction;
        // Then start counting Branch Delay Slots; Once reached, mark as the BB Boundery.
        if(branch_instr_count)
        {
            branch_instr_flag = 1;
            // Reset the delay slot counter
            delay_slot_count = 0;
        }

        if(branch_instr_flag)
        {
            delay_slot_count++;

            // If instruction count in this packet is 1, It could be an Idle 'NOP n' Instruction
            // If yes, then see how many delay slots it occupies.
            if((*pcurexp)->get_instr_count() == 1)
            {
                dec_instr_t *pdecoded_instr = (*pcurexp)->get_decoded_instr(0);
                if(pdecoded_instr->is_idle_instruction())
                {
                    uint8_t nop_count = pdecoded_instr->get_nop_count();
                    if(nop_count > 0)
                    {
                        // We already added one to delay slots above
                        delay_slot_count += nop_count - 1;
                    }
                }
            }
        }

        // See if the Next Execute Packet is a Branch Target?
        pnextexp++;
        if(pnextexp != plastexp)
            // How to Check the Branch Targets which are not the First Instruction of ExPkt?
            // OR Write a function that checks whether all target addresses do have a corresponding BB
            is_br_target = (*pnextexp)->is_branch_target();
        else
            is_br_target = 0;

        // If the Next Execute Packet is a Branch Target;
        // Then this marks the begining of a New Basic Block
        if(is_br_target || (delay_slot_count > BRANCH_DELAY_SLOTS))
        {
            pbb->set_next_bb_addr((*pnextexp)->get_addr());    // For fallthrough, if required
            bblist.push_back(pbb);

            // Reset for processing the Next Basic Block
            pbb = NULL;
            branch_instr_flag = 0;
            delay_slot_count = 0;
        }
    }

    // This means that the Last Basic Block hasn't been pushed to the bblist.
    // Due to the fact that it does not contain any branch instructions.
    // Which means that its an invalid BB; so we discard it now.
    if(pbb)
    {
        delete pbb;
        pbb = NULL;
    }

    if(check_branch_targets(explist))
    {
        DOUT << "Error: Marking Branch Targets" << endl;
        return;
    }

    if(add_pred_succ_cfg_info())
    {
        DOUT << "Error: Adding Pred/Succ CFG Info to Basic Blocks" << endl;
        return;
    }
}

basic_block * basic_block_list :: find_basic_block(uint32_t base_addr)
{
    for(list<basic_block*>::iterator pcurbb = bblist.begin(),
        plastbb = bblist.end(); pcurbb != plastbb; pcurbb++)
    {
        if((*pcurbb)->get_base_addr() == base_addr)
            return (*pcurbb);
    }
    return(NULL);
}

basic_block * basic_block_list :: get_basic_block(uint32_t id)
{
    for(list<basic_block*>::iterator pcurbb = bblist.begin(),
        plastbb = bblist.end(); pcurbb != plastbb; pcurbb++)
    {
        if((*pcurbb)->get_id() == id)
            return (*pcurbb);
    }
    return(NULL);
}

int32_t basic_block_list :: check_branch_targets(execute_packet_list * pexplist)
{
    set<int32_t>        br_targets_notfound;
    set<int32_t>       *p_branch_targets = pexplist->get_target_address_set();

    // For each branch target address; There should exist a Basic Block.
    for(set<int32_t>::iterator curaddr = p_branch_targets->begin(),
        lastaddr = p_branch_targets->end(); curaddr != lastaddr; curaddr++)
    {
        if(find_basic_block(*curaddr) == NULL)
        {
            br_targets_notfound.insert(*curaddr);
        }
    }

    if(br_targets_notfound.size() > 0)
    {
        DOUT << "Warning: Basic Blocks for the following Branch Targets were not found" << endl;
        for(set<int32_t>::iterator curaddr = br_targets_notfound.begin(),
            lastaddr = br_targets_notfound.end(); curaddr != lastaddr; curaddr++)
        {
            cout << FMT_INT << *curaddr << endl;
        }
        return(-1);
    }

    return(0);
}

int32_t basic_block_list :: add_pred_succ_cfg_info()
{
    // NOTE: Dereferencing the Iterator gives us the pointer to BasicBlock or List Item Type.
    for(list <basic_block*> :: iterator BBI = bblist.begin(), BBE = bblist.end(); BBI != BBE; ++BBI)
    {
        for(list <execute_packet*> :: iterator EPI = (*BBI)->execpkt_list.begin(), //BBI->execpkt_list.begin(),
            EPE = (*BBI)->execpkt_list.end(); EPI != EPE; ++EPI)
        {
            if((*EPI)->get_branch_instr_count())
            {
                // Iterate through all instructions to get successor basic block addresses.
                for (int32_t index = 0; index < (*EPI)->get_instr_count(); index++)
                {
                     dec_instr_t *pdecoded_instr = (*EPI)->get_decoded_instr(index);
                     if(pdecoded_instr->is_branch_instr())
                     {
                         if (pdecoded_instr->is_direct_branch())
                         {
                             basic_block * succ_bb = find_basic_block(pdecoded_instr->get_branch_address());
                             if(!succ_bb)
                             {
                                DOUT << "Error: Could Not Find Basic Block for " << ios::hex
                                     << pdecoded_instr->get_branch_address() << endl;
                                return -1;
                             }

                             (*BBI)->succ_set.insert(succ_bb);
                             succ_bb->pred_set.insert(*BBI);
                         }
                         else // Its a Branch Instruction but Indirect One.
                         {
                             // TODO: What should we do here ????
                         }
                     }
                }
            }
        }

        // Check if the successor set is empty? And this BB doesn't contain any Branch Instruction?
        // If yes, Add fallthrough BB as Successor to the current BB.
        if((*BBI)->succ_set.empty() && (*BBI)->get_branch_instr_count() == 0)
        {
            basic_block * succ_bb = find_basic_block((*BBI)->get_next_bb_addr());
            (*BBI)->succ_set.insert(succ_bb);
            succ_bb->pred_set.insert(*BBI);
        }
    }

    int32_t basic_block_count = 0;
    int32_t unknown_pred_bb_count = 0;
    int32_t unknown_succ_bb_count = 0;
    int32_t known_pred_succ_count = 0;

    for(list <basic_block*> :: iterator BBI = bblist.begin(), BBE = bblist.end(); BBI != BBE; ++BBI)
    {
        basic_block_count++;
        if((*BBI)->pred_set.empty()) unknown_pred_bb_count++;
        if((*BBI)->succ_set.empty()) unknown_succ_bb_count++;
        if((*BBI)->pred_set.size() && (*BBI)->succ_set.size()) known_pred_succ_count++;
    }

    cout << "Total Basic Blocks: " << basic_block_count << endl;
    cout << "Basic Blocks with Unknown Predecessors: " << unknown_pred_bb_count
         << " (" << (((float)unknown_pred_bb_count/(float)basic_block_count) * 100.0) << "%)" << endl;
    cout << "Basic Blocks with Unknown Successors  : " << unknown_succ_bb_count
         << " (" << (((float)unknown_succ_bb_count/(float)basic_block_count) * 100.0) << "%)" << endl;
    cout << "Basic Blocks with Known Predecessors & Successors  : " << known_pred_succ_count
         << " (" << (((float)known_pred_succ_count/(float)basic_block_count) * 100.0) << "%)" << endl;
    return 0;
}

int32_t basic_block_list :: find_function_calls()
{
    // NOTE: Dereferencing the Iterator gives us the pointer to BasicBlock or List Item Type.
    for(list <basic_block*> :: iterator BBI = bblist.begin(), BBE = bblist.end(); BBI != BBE; ++BBI)
    {
        for(list <execute_packet*> :: iterator EPI = (*BBI)->execpkt_list.begin(), //BBI->execpkt_list.begin(),
            EPE = (*BBI)->execpkt_list.end(); EPI != EPE; ++EPI)
        {
            if((*EPI)->get_branch_instr_count())
            {
                for (int32_t index = 0; index < (*EPI)->get_instr_count(); index++)
                {
                     dec_instr_t *pdecoded_instr = (*EPI)->get_decoded_instr(index);
                     if(pdecoded_instr->is_branch_instr())
                     {
                         if (pdecoded_instr->is_direct_branch())
                         {
                             basic_block * succ_bb = find_basic_block(pdecoded_instr->get_branch_address());
                             if(!succ_bb)
                             {
                                DOUT << "Error: Could Not Find Basic Block for " << ios::hex
                                     << pdecoded_instr->get_branch_address() << endl;
                                return -1;
                             }

                             succ_bb->get_execute_packet(0);
                         }
                         else // Its a Branch Instruction but Indirect One.
                         {
                             // TODO: What should we do here ????
                         }
                     }
                }
            }
        }
    }
    return 0;
}

#endif
