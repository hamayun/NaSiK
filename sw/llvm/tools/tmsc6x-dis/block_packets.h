/*************************************************************************************
 * File   : block_packets.h,
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

#ifndef _BLOCK_PACKETS_H
#define	_BLOCK_PACKETS_H

#include "coff_reader.h"
#include "tms320C6x_instruction.h"
#include <list>
#include <set>

#define     FETCH_PKT_SIZE          8

#define     PRINT_MODE_RAW          0
#define     PRINT_MODE_DEC          1
#define     PRINT_MODE_BOTH         2

class fetch_packet
{
private:
    int                     m_nm_instrs;
    raw_instr_t            *p_raw_instr[FETCH_PKT_SIZE];
    coff_reader            *p_coff_reader; 

public:
    fetch_packet(coff_reader * ptr_reader){
        m_nm_instrs = 0;
        memset(p_raw_instr, 0x0, sizeof(raw_instr_t *) * FETCH_PKT_SIZE);
        if(ptr_reader){
            p_coff_reader = ptr_reader;
        }
        else{
            DOUT << "Error: Invalide COFF Reader Pointer" << endl;
        }
    }
    ~fetch_packet(){}

    int read();

    void print(ostream *out){
        for(int i=0; i<m_nm_instrs; i++){
            if(p_raw_instr[i])
            {
                p_raw_instr[i]->print(out);
                (*out) << endl;
            }
        }
    }

    unsigned int get_fetch_pkt_address()
    {
        if(p_raw_instr[0])
            return(p_raw_instr[0]->get_vir_addrs());
        else
            return(NULL); 
    }

    coff_reader * get_parent(){
        return(p_coff_reader); 
    }

    int get_nm_instrs(){
        return(m_nm_instrs);
    }

    raw_instr_t * get_instr(int instr_nm){
        return (p_raw_instr[instr_nm]); 
    }
};

class fetch_packet_list
{
private:
    list<fetch_packet*>  fplist;

public:
    fetch_packet_list(coff_reader * preader);

    ~fetch_packet_list(); 

    void print(ostream * out)
    {
        for(list<fetch_packet*>::iterator pcurfp = fplist.begin(),  plastfp = fplist.end();
            pcurfp != plastfp; pcurfp++)
        {
            (*pcurfp)->print(out);
        }
    }

    list<fetch_packet*> * get_list()
    {
        return(&fplist); 
    }
}; 

class execute_packet
{
private:
    fetch_packet           *p_parent;
    int                     m_nm_instrs;
    raw_instr_t            *p_raw_instr[FETCH_PKT_SIZE];
    dec_instr_t            *p_dec_instr[FETCH_PKT_SIZE];
public:

    execute_packet(){
        p_parent = NULL;
        m_nm_instrs = 0;
        memset(p_raw_instr, 0x0, sizeof(raw_instr_t *) * FETCH_PKT_SIZE);
        memset(p_dec_instr, 0x0, sizeof(dec_instr_t *) * FETCH_PKT_SIZE);
    }
    ~execute_packet(){}

    int decode_packet();

    void print(ostream *out, int mode);

    void set_parent(fetch_packet * parent)
    {
        p_parent = parent;
    }

    void set_raw_instr(raw_instr_t * ptr_raw_instr)
    {
        if(ptr_raw_instr)
           p_raw_instr[m_nm_instrs++] = ptr_raw_instr;
    }

    unsigned int get_addr()
    {
        if(p_raw_instr[0])
            return(p_raw_instr[0]->get_vir_addrs());
        else
            return (0);
    }

    raw_instr_t * get_raw_instr(int instr_num)
    {
        return(p_raw_instr[instr_num]);
    }

    dec_instr_t * get_dec_instr(int instr_num)
    {
        return(p_dec_instr[instr_num]); 
    }

    fetch_packet * get_parent()
    {
        return(p_parent);
    }

    int get_nm_instrs()
    {
        return(m_nm_instrs);
    }

    int get_nm_br_instrs()
    {
        int nm_br_instrs = 0;

        for(int i=0; i<m_nm_instrs; i++)
            if(p_dec_instr[i]->is_branch_instr())
                nm_br_instrs++;

        return(nm_br_instrs);
    }

    bool is_branch_target()
    {
        if(p_dec_instr[0]->is_branch_target() ||
           p_dec_instr[0]->is_label())
            return (true);
        else
            return (false);
    }

    int transcode()
    {
        for(int i=0; i<m_nm_instrs; i++)
        {
            ir_instr_t * pir_instr = p_dec_instr[i]->transcode();
            pir_instr->print(&cout);
        }
        return(0); 
    }
};

class execute_packet_list
{
private:
    list<execute_packet*>   eplist;
    set<int>                branch_targets;
    
    int mark_br_targets(); 

public:
    execute_packet_list(fetch_packet_list *fplist); 

    ~execute_packet_list(); 

    void print(ostream *out, int mode)
    {
        for(list<execute_packet*>::iterator pcurexp = eplist.begin(),
            plastexp = eplist.end(); pcurexp != plastexp; pcurexp++)
        {
            (*pcurexp)->print(out, mode);
        }
    }

    list<execute_packet*> * get_list()
    {
        return(&eplist); 
    }

    set<int> * get_target_address_set()
    {
        if(branch_targets.size() != 0)
            return (&branch_targets);
        else
            return(NULL); 
    }
};

class basic_block
{
private:
    unsigned int             m_id;
    unsigned int             m_base_addr;
    list <execute_packet*>   eplist;

public:
    basic_block(unsigned int id){
        m_id = id;
        m_base_addr = 0; 
    }
    ~basic_block(){}

    unsigned int get_id()
    {
        return (m_id); 
    }

    unsigned int get_base_addr()
    {
        return(m_base_addr); 
    }

    int get_nm_expkts()
    {
        return(eplist.size()); 
    }

    int get_nm_instrs()
    {
        int nm_instr = 0;

        for(list<execute_packet*>::iterator pcurexp = eplist.begin(),
            plastexp = eplist.end(); pcurexp != plastexp; pcurexp++)
        {
            nm_instr += (*pcurexp)->get_nm_instrs();
        }
        return(nm_instr);
    }

    void push_back(execute_packet * pexpkt)
    {
        if(eplist.size() == 0)
            m_base_addr = pexpkt->get_addr();

        eplist.push_back(pexpkt);
    }

    void print(ostream *out, int mode)
    {
        (*out) << "BB" << m_id
               << "\t\tAddress:" << FMT_INT << m_base_addr
               << "\t(" << dec << get_nm_expkts() << " Packets, "
               << get_nm_instrs() << " Instructions)"
               << "\t Predecessors: ?" << endl;

        for(list<execute_packet*>::iterator pcurexp = eplist.begin(),
            plastexp = eplist.end(); pcurexp != plastexp; pcurexp++)
        {
            (*pcurexp)->print(out, mode);
        }
    }

    int transcode()
    {
        for(list<execute_packet*>::iterator pcurexp = eplist.begin(),
            plastexp = eplist.end(); pcurexp != plastexp; pcurexp++)
        {
            if((*pcurexp)->transcode())
                return (-1); 
        }
        return(0); 
    }
};

class basic_block_list
{
private:
    list <basic_block*> bblist;
    int check_br_targets(execute_packet_list * pexplist);
    
public:
    basic_block_list(execute_packet_list * explist); 
    ~basic_block_list();

    basic_block * find_basic_block(unsigned int base_addr);
    basic_block * get_basic_block(unsigned int id);
    void print(ostream *out, int mode)
    {
        for(list<basic_block*>::iterator pcurbb = bblist.begin(),
            plastbb = bblist.end(); pcurbb != plastbb; pcurbb++)
        {
                (*pcurbb)->print(out, mode);
                (*out) << endl; 
        }
    }
};
#endif	/* _BLOCK_PACKETS_H */

