/*************************************************************************************
 * File   : block_packets.cpp,
 *
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

#include "block_packets.h"
#include <algorithm>
#include <set>

// class fetch_packet
// Read one Fetch Packet from the Binary File Reader
int fetch_packet :: read()
{
    for(int i=0; i<FETCH_PKT_SIZE; i++){
        p_raw_instr[i] = p_coff_reader->fetch_next_instr();

        if(!p_raw_instr[i]){                        // This means end of instruction stream.
            if(i == 0){
                m_nm_instrs = 0;
                return(-1);
            }
        }

        m_nm_instrs = i + 1;
    }

    if(!m_nm_instrs) // # of Instructions should not be zero here.
    {
        DOUT << "Error: Invalid number of Instruction in Fetch Packet" << endl; 
        return(-1);
    }
    
    return(0);
}

// class fetch_packet_list
fetch_packet_list :: fetch_packet_list(coff_reader * preader)
{
    fetch_packet       *pfetpkt = NULL;

    while(1)
    {
        if(!pfetpkt)
        {
            pfetpkt = new fetch_packet(preader);
            if(!pfetpkt)
            {
                DOUT << "Error: Allocating Memory for Fetch Packet" << endl;
                return;
            }
        }

        if(pfetpkt->read())
        {
            delete pfetpkt;
            pfetpkt = NULL;
            break;
        }

        fplist.push_back(pfetpkt);
        pfetpkt = NULL;
    }
}

fetch_packet_list :: ~fetch_packet_list()
{
    for(list<fetch_packet*>::iterator pcurfp = fplist.begin(),  plastfp = fplist.end();
        pcurfp != plastfp; pcurfp++)
    {
        delete (*pcurfp);
        *pcurfp = NULL;
    }
}

// class execute_packet
int execute_packet :: decode_packet(){
    for(int i=0; i<m_nm_instrs; i++){
        p_dec_instr[i] = new tms320C6x_instruction();
        if(!p_dec_instr[i]){
            DOUT << "Error: Allocating Memory for Instruction Decoding" << endl;
            return (-1);
        }

        p_dec_instr[i]->set_parent(this);   // For Refering to the Parent Execute Packet

        if(p_dec_instr[i]->decode(p_raw_instr[i])){
            DOUT << "Error: Decoding Instruction" << endl;
            return (-1);
        }
    }
    return (0);
}

void execute_packet :: print(ostream *out, int mode)
{
    coff_symbol_table * p_symtab = get_parent()->get_parent()->get_symbol_table();

    for(int i=0; i<m_nm_instrs; i++)
    {
        if(mode == PRINT_MODE_RAW || mode == PRINT_MODE_BOTH)
        {
            if(p_raw_instr[i]){
                // Check whether this instruction is a Label; If yes print labels first
                p_symtab->print_labels(p_raw_instr[i]->get_vir_addrs(), out);

                // Now print the Raw Instruction.
                p_raw_instr[i]->print(out);
            }
        }

        if(i > 0)
            (*out) << " || ";
        else
            (*out) << "    ";

        if(mode == PRINT_MODE_DEC || mode == PRINT_MODE_BOTH)
        {
            if(p_dec_instr[i])
                p_dec_instr[i]->print(out);
        }
        
        (*out) << endl;
    }
}

// class execute_packet_list
execute_packet_list :: execute_packet_list(fetch_packet_list *fplist)
{
    execute_packet     *p_expkt = NULL;
    raw_instr_t        *p_instr = NULL;

    for(list<fetch_packet*>::iterator pcurfp = fplist->get_list()->begin(),
        plastfp = fplist->get_list()->end(); pcurfp != plastfp; pcurfp++)
    {
        for(int i=0; i< (*pcurfp)->get_nm_instrs(); i++)
        {
            if(!p_expkt)
            {
                p_expkt = new execute_packet();
                if(!p_expkt)
                {
                    DOUT << "Error: Allocating Memory for Execute Packet" << endl;
                    return;
                }

                p_expkt->set_parent(*pcurfp);
            }

            // Build Execute Packet
            p_instr = (*pcurfp)->get_instr(i);
            p_expkt->set_raw_instr(p_instr);

            if(!(p_instr->get_value() & 0x1)) {  // This marks the end of Execute Packet

                // Decode this packet
                if(p_expkt->decode_packet())
                {
                    DOUT << "Error: Decoding Execute Packet" << endl;
                    return;
                }

                // Insert into list
                eplist.push_back(p_expkt);
                p_expkt = NULL;
           }
        }
    }

    if(mark_br_targets())
    {
        DOUT << "Error: Marking Branch Targets" << endl;
        return;
    }
}

execute_packet_list :: ~execute_packet_list()
{
    for(list<execute_packet*>::iterator pcurexp = eplist.begin(),
        plastexp = eplist.end(); pcurexp != plastexp; pcurexp++)
    {
        delete (*pcurexp);
        *pcurexp = NULL;
    }
}

int execute_packet_list :: mark_br_targets()
{
    set<int>        branch_targets_marked;
    set<int>        branch_targets_missed;

    dec_instr_t *   p_dec_instr = NULL;
    raw_instr_t *   p_raw_instr = NULL;
    int             instr_addrs;

    // Here we extract all the branch target addresses
    for(list<execute_packet*>::iterator pcurexp = eplist.begin(),
        plastexp = eplist.end(); pcurexp != plastexp; pcurexp++)
    {
        for(int i=0; i<(*pcurexp)->get_nm_instrs(); i++)
        {
            p_dec_instr = (*pcurexp)->get_dec_instr(i);
            if(p_dec_instr->is_direct_branch())
            {
                branch_targets.insert(p_dec_instr->get_branch_address());
            }
        }
    }

    // Now we have all the branch addresses; Mark Instructions that are Branch Targets
    for(set<int>::iterator curaddr = branch_targets.begin(), lastaddr = branch_targets.end();
        curaddr != lastaddr;)
    {
        for(list<execute_packet*>::iterator pcurexp = eplist.begin(),
            plastexp = eplist.end(); pcurexp != plastexp; pcurexp++)
        {
            for(int i=0; i<(*pcurexp)->get_nm_instrs(); i++)
            {
                p_raw_instr = (*pcurexp)->get_raw_instr(i);
                instr_addrs = p_raw_instr->get_vir_addrs();

                if(instr_addrs == (*curaddr))
                {
                    p_dec_instr = (*pcurexp)->get_dec_instr(i);
                    p_dec_instr->set_branch_target();

                    branch_targets_marked.insert(*curaddr);
                    curaddr++;
                }

                if(instr_addrs > (*curaddr))
                {
                    curaddr++;
                }
            }
        }
    }

    // Lets see if we missed any address?
    set_difference(branch_targets.begin(), branch_targets.end(),
                   branch_targets_marked.begin(), branch_targets_marked.end(),
                   inserter(branch_targets_missed,branch_targets_missed.end()));

    if(branch_targets_missed.size() > 0)
    {
        cout << "Warning: The following Branch Target Addresses were missed during Marking Phase" << endl;
        for(set<int>::iterator curaddr = branch_targets_missed.begin(),
            lastaddr = branch_targets_missed.end(); curaddr != lastaddr; curaddr++)
        {
            cout << FMT_INT << *curaddr << endl;
        }
        return(-1);
    }

    return (0);
}

// class basic_block_list
basic_block_list :: basic_block_list(execute_packet_list * explist)
{
    basic_block *pbb    = NULL;
    unsigned int bbnum  = 0;
    int          num_br_instr = 0;
    int          is_br_target = 0;

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
        num_br_instr = (*pcurexp)->get_nm_br_instrs();

        // See if the Next Execute Packet is a Branch Target?
        pnextexp++;
        if(pnextexp != plastexp)
            // How to Check the Branch Targets which are not the First Instruction of ExPkt?
            // OR Write a function that checks whether all target addresses do have a corresponding BB
            is_br_target = (*pnextexp)->is_branch_target();
        else
            is_br_target = 0;

        if(num_br_instr > 0 || is_br_target)
        {
            bblist.push_back(pbb);
            pbb = NULL;
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

    if(check_br_targets(explist))
    {
        DOUT << "Error: Marking Branch Targets" << endl;
        return;
    }
}

basic_block_list :: ~basic_block_list()
{
    for(list<basic_block*>::iterator pcurbb = bblist.begin(),
        plastbb = bblist.end(); pcurbb != plastbb; pcurbb++)
    {
        delete (*pcurbb);
        *pcurbb = NULL;
    }
}

basic_block * basic_block_list :: find_basic_block(unsigned int base_addr)
{
    for(list<basic_block*>::iterator pcurbb = bblist.begin(),
        plastbb = bblist.end(); pcurbb != plastbb; pcurbb++)
    {
        if((*pcurbb)->get_base_addr() == base_addr)
            return (*pcurbb);
    }
    return(NULL);
}

basic_block * basic_block_list :: get_basic_block(unsigned int id)
{
    for(list<basic_block*>::iterator pcurbb = bblist.begin(),
        plastbb = bblist.end(); pcurbb != plastbb; pcurbb++)
    {
        if((*pcurbb)->get_id() == id)
            return (*pcurbb);
    }
    return(NULL);
}

int basic_block_list :: check_br_targets(execute_packet_list * pexplist)
{
    set<int>        br_targets_notfound;
    set<int>       *p_branch_targets = pexplist->get_target_address_set();

    // For each branch target address; There should exist a Basic Block.
    for(set<int>::iterator curaddr = p_branch_targets->begin(),
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
        for(set<int>::iterator curaddr = br_targets_notfound.begin(),
            lastaddr = br_targets_notfound.end(); curaddr != lastaddr; curaddr++)
        {
            cout << FMT_INT << *curaddr << endl;
        }
        return(-1);
    }

    return(0);
}




