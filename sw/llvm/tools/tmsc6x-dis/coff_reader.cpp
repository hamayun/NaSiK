/*************************************************************************************
 * File   : coff_reader.cpp,
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

#include <iostream>
#include <fstream>
#include "coff_reader.h"

using namespace std; 

// class coff_file_header
int coff_file_header :: read (ifstream *file)
{
    char buff[COFF_FILE_HDR_SIZE];

    if(!file->is_open()){
        DOUT << "Error: Input File Not Open" << endl;
        return (-1);
    }

    file->seekg (0, ios::beg);    // Set the Get Pointer at the Start of Stream

    file->read((char *)buff, COFF_FILE_HDR_SIZE);
    if(file->eof() || file->fail()){
        return (-1);
    }

    set_vers_id(*((unsigned short*)&buff[0]));
    set_nm_sect_hdrs(*(unsigned short*)&buff[2]);
    set_time_stamp(*(int*)&buff[4]);
    set_sym_tbl_start(*(int*)&buff[8]);
    set_nm_sym_tbl_entries(*(int*)&buff[12]);
    set_nm_opt_hdr_bytes(*(unsigned short*)&buff[16]);
    set_flags(*(unsigned short*)&buff[18]);
    set_target_id(*(unsigned short*)&buff[20]);

    return (0);
}

// class coff_opt_file_header
int coff_opt_file_header :: read(ifstream *file)
{
    char buff[COFF_OPT_FILE_HDR_SIZE];

    if(!file->is_open()){
        DOUT << "Error: Input File Not Open" << endl;
        return (-1);
    }

    file->seekg(22, ios::beg); // Set the Get Pointer to the Start of its Location in Stream

    file->read((char*)buff, COFF_OPT_FILE_HDR_SIZE);
    if(file->eof() || file->fail()){
        return (-1);
    }

    set_magic(*(short*)&buff[0]);
    set_vers_stamp(*(short*)&buff[2]);
    set_exe_code_size(*(int*)&buff[4]);
    set_init_data_size(*(int*)&buff[8]);
    set_uninit_data_size(*(int*)&buff[12]);
    set_entry_point(*(int*)&buff[16]);
    set_exe_code_addr(*(int*)&buff[20]);
    set_init_data_addr(*(int*)&buff[24]);

    return (0);
}

// class coff2_section_header
int coff2_section_header :: read(ifstream *file, coff_string_table *p_string_table)
{
    char buff[COFF2_SECTION_SIZE];
    int  *p_offset = NULL;

    if(!file->is_open()){
        DOUT << "Error: Input File Not Open" << endl;
        return (-1);
    }

    file->read((char*)buff, COFF2_SECTION_SIZE);
    if(file->eof() || file->fail()){
        return (-1);
    }

    p_offset = (int *) &buff[0];
    if(p_offset[0] == 0){
        coff_strtab_entry * p_strtab_entry = p_string_table->get_strtab_entry(p_offset[1]);
        if(p_strtab_entry)
            set_name_str(p_strtab_entry->get_string());
    }
    
    set_name((char*)&buff[0]);
    set_phy_addr(*(int*)&buff[8]);
    set_vir_addr(*(int*)&buff[12]);
    set_size(*(int*)&buff[16]);
    set_fptr_raw_data(*(int*)&buff[20]);
    set_fptr_reloc_entries(*(int*)&buff[24]);
    set_reserved_1(*(int*)&buff[28]);
    set_nm_reloc_entries(*(unsigned int*)&buff[32]);
    set_nm_line_entries(*(unsigned int*)&buff[36]);
    set_flags(*(unsigned int*)&buff[40]);
    set_reserved_2(*(unsigned short*)&buff[44]);
    set_mem_page_nb(*(unsigned short*)&buff[46]);

    return (0);
}

// class coff_symtab_entry
int coff_symtab_entry :: sym_entries_count = 0;
int coff_symtab_entry :: aux_entries_count = 0;

int coff_symtab_entry :: read(ifstream *file, coff_string_table *p_string_table)
{
    char buff[COFF_SYMTAB_ENTRY_SIZE];
    int  *p_offset = NULL;

    if(!file->is_open()){
        DOUT << "Error: Input File Not Open" << endl;
        return (-1);
    }

    file->read((char*)buff, COFF_SYMTAB_ENTRY_SIZE);
    if(file->eof() || file->fail()){
        return (-1);
    }

    p_offset = (int *) &buff[0];
    if(p_offset[0] == 0){
        coff_strtab_entry * p_strtab_entry = p_string_table->get_strtab_entry(p_offset[1]);
        if(p_strtab_entry)
            set_name_str(p_strtab_entry->get_string());
    }

    set_name((char*)&buff[0]);
    set_sym_val(*(int*)&buff[8]);
    set_sec_num(*(short*)&buff[12]);
    set_reserved(*(unsigned short*)&buff[14]);
    set_storage_class(buff[16]);
    set_nm_aux_entries(buff[17]);

    if((int)buff[17] == 1){
        aux_entries_count++;
    }

    sym_entries_count++;
    return (0);
}

// class coff_symbol_table
int coff_symbol_table :: read(ifstream *file, int symtab_start_addr, coff_string_table *p_string_table)
{
        int fptr_loc = file->tellg();
        if(fptr_loc == -1){
            DOUT << "Error: Invalid File Position" << endl;
            return (-1); 
        }

        // Move File Pointer to Start of Symbol Table
        file->seekg(symtab_start_addr, ios::beg);

        for(int i=0; i<m_nm_entries; i++){
            if(p_symtab[i].read(file, p_string_table))
                return (-1);
        }

        // Restore File Pointer to Old Value
        file->seekg(fptr_loc, ios::beg);
        return(0); 
}

// class coff_string_table
int coff_string_table :: str_entries_count = 0;

int coff_string_table :: coff_get_strings_count(ifstream *file, int strtab_start_addr)
{
        int str_tbl_size;
        int actual_str_size; 
        int str_count = 0; 
        char * buffer;

        int fptr_loc = file->tellg();
        if(fptr_loc == -1){
            DOUT << "Error: Invalid File Position" << endl;
            return (-1);
        }

        // Move File Pointer to Start of String Table
        file->seekg(strtab_start_addr, ios::beg);

        // Get the String Table Size
        file->read((char*)&str_tbl_size, sizeof(int));
        if(file->eof() || file->fail()){
            return (-1);
        }

        actual_str_size = str_tbl_size - 4; // Because First 4 bytes Contain the Size itself.
        
        buffer = new char[actual_str_size];
        if(!buffer){
            DOUT << "Error: Allocating buffer for String Counting" << endl;
            return (-1); 
        }

        file->read((char *) buffer, actual_str_size);
        if(file->eof() || file->fail()){
            DOUT << "Error: Reading buffer for String Counting" << endl;
            return(-1); 
        }

        for(int i=0; i< actual_str_size; i++){
            if(!buffer[i]){
                str_count++;
            }
        }

        delete [] buffer;
        // Restore File Pointer to Old Value
        file->seekg(fptr_loc, ios::beg);
        return (str_count);
}

int coff_string_table :: read(ifstream *file, int strtab_start_addr)
{
        char buff[COFF_MAX_STR_SIZE];
        coff_strtab_entry *ptr_strtab_entry; 
        int str_tbl_size;

        int fptr_loc = file->tellg();
        if(fptr_loc == -1){
            DOUT << "Error: Invalid File Position" << endl;
            return (-1);
        }

        // Move File Pointer to Start of String Table
        file->seekg(strtab_start_addr, ios::beg);

        // Get the String Table Size
        file->read((char*)&str_tbl_size, sizeof(int));
        if(file->eof() || file->fail()){
            return (-1);
        }

        for(int str_tbl_offset=4; str_tbl_offset<(str_tbl_size+4);){
            int cur_str_offset = str_tbl_offset;
            int cur_str_size = 0;
            memset(buff, 0x0, COFF_MAX_STR_SIZE);

            while(1){
                file->read((char*)&buff[cur_str_size], sizeof(char));
                if(file->eof()){
                    // Restore File Pointer to Old Value
                    file->clear();
                    file->seekg(fptr_loc, ios::beg);
                    return (0);
                }
                if(file->fail()){
                    DOUT << "Error: Failed to Read String" << endl;
                    return (-1);
                }

                str_tbl_offset++;           
                cur_str_size++;
            
                if(!buff[cur_str_size - 1]){
                    break;
                }
            }

            // Create a New String Entry Object
            ptr_strtab_entry = new coff_strtab_entry(cur_str_offset, buff, cur_str_size);
            if(!ptr_strtab_entry){
                DOUT << "Error: Allocating String Entry Object" << endl;
                return(-1); 
            }

            if(add_strtab_entry(ptr_strtab_entry)){
                DOUT << "Error: Adding String Table Entry" << endl;
                return(-1); 
            }
        }

        return(-1);
}

// class coff_section
int coff_section :: read(ifstream *file)
{
    unsigned int        m_cur_vir_addr = get_section_header()->get_vir_addr();
    unsigned int        m_cur_phy_addr = get_section_header()->get_phy_addr();
    unsigned int        value;

    int fptr_loc = file->tellg();
    if(fptr_loc == -1){
        DOUT << "Error: Invalid File Position" << endl;
        return (-1);
    }

    // Move File Pointer to Start of Section
    file->seekg(get_section_header()->get_fptr_raw_data(), ios::beg);

    for(int i=0; i<m_nm_entries; i++){
        file->read((char*)&value, sizeof(unsigned int));
        if(file->eof() || file->fail()){
            DOUT << "Error: Reading Instruction From File" << endl;
            return (-1);
        }

        p_sect_entries[i] = new coff_sect_entry(m_cur_vir_addr, m_cur_phy_addr, value);
        if(!p_sect_entries[i]){
            DOUT << "Error: Allocating Memory for Instruction Object" << endl;
            return(-1); 
        }

        m_cur_vir_addr += VIR_ADDR_SIZE;
        m_cur_phy_addr += PHY_ADDR_SIZE;
    }

    // Restore File Pointer to Old Value
    file->seekg(fptr_loc, ios::beg);
    return(0);
}

// class coff_reader
coff_reader :: coff_reader(string filename)
{
    p_file_in = new ifstream(filename.c_str(), ios::in|ios::binary);
    if(!p_file_in->is_open()){
        DOUT << "Error: Opening Input File: " << filename.c_str() << endl;
        return;
    }

    p_file_out = new ofstream("header_tables.txt", ios::out);
    if(!p_file_out->is_open()){
        DOUT << "Error: Opening Output File: " << endl;
        return;
    }

    p_file_header = new coff_file_header();
    if(!p_file_header){
        DOUT << "Error: Create File Header Object" << endl;
        return;
    }

    if(p_file_header->read(p_file_in))
    {
        DOUT << "Error: Reading COFF File Header" << endl;
        return;
    }

    p_file_header->print(p_file_out);     // We can also pass "&cout" to see output on screen

    if(p_file_header->get_nm_opt_hdr_bytes())
    {
        p_opt_file_header = new coff_opt_file_header();
        if(!p_opt_file_header)
        {
            DOUT << "Error: Create Optional File Header Object" << endl;
            return;
        }

        if(p_opt_file_header->read(p_file_in))
        {
            DOUT << "Error: Reading Optional COFF File Header" << endl;
            return;
        }

        p_opt_file_header->print(p_file_out);
    }

    // Create String Table
    int strtab_start = p_file_header->get_str_tbl_start();
    int strings_count = coff_string_table :: coff_get_strings_count(p_file_in, strtab_start);
    if(strings_count == -1){
        DOUT << "Error: Invalid Number of String Count" << endl;
        return;
    }

    p_string_table = new coff_string_table(strings_count);
    if(!p_string_table){
        DOUT << "Error: Allocating Memory for String Table" << endl;
        return;
    }
    
    if(p_string_table->read(p_file_in, strtab_start)){
        DOUT << "Error: Reading String Table" << endl;
        return;
    }
    p_string_table->print(p_file_out, coff_string_table :: get_str_entries_count());

    // Create Symbol Table Here
    p_symbol_table = new coff_symbol_table(p_file_header->get_nm_sym_tbl_entries());
    if(!p_symbol_table){
        DOUT << "Error: Allocating Memory for Symbol Table" << endl;
        return;
    }
    if(p_symbol_table->read(p_file_in, p_file_header->get_sym_tbl_start(), p_string_table)){
        DOUT << "Error: Reading Symbol Table" << endl;
        return;
    }
    p_symbol_table->print(p_file_out, p_file_header->get_nm_sym_tbl_entries());

    // Allocate Section Header Objects
    p_section_header = new coff2_section_header * [p_file_header->get_nm_sect_hdrs()];
    if(!p_section_header){
        DOUT << "Error: Memory Allocation for Section Header Pointers" << endl;
        return;
    }

    for (int i=0; i<p_file_header->get_nm_sect_hdrs(); i++){
        p_section_header[i] = new coff2_section_header();
        if(!p_section_header[i]){
            DOUT << "Error: Memory Allocation for Section Header Objects" << endl;
            return;
        }
        if(p_section_header[i]->read(p_file_in, p_string_table)){
            DOUT << "Error: Reading Section Header" << endl;
            return;
        }
        p_section_header[i]->print(p_file_out, i);
    }

    // Read the All Sections Now
    p_section = new coff_section * [p_file_header->get_nm_sect_hdrs()];
    if(!p_section){
        DOUT << "Error: Memory Allocation for Section Pointers " << endl;
        return;
    }

    for (int i=0; i<p_file_header->get_nm_sect_hdrs(); i++){
        p_section[i] = new coff_section(p_section_header[i]);
        if(!p_section[i]){
            DOUT << "Error: Memory Allocation for Section Objects" << endl;
            return;
        }
        if(p_section[i]->read(p_file_in)){
            DOUT << "Error: Reading Section" << endl;
            return;
        }
        //p_section[i]->print(p_file_out);
    }
}

coff_reader :: ~coff_reader()
{
    for (int i=0; i<p_file_header->get_nm_sect_hdrs(); i++){
        if(p_section[i]){
            delete p_section[i];
            p_section[i] = NULL;
        }
    }

    if(p_section){
        delete [] p_section;
        p_section = NULL;
    }

    for (int i=0; i<p_file_header->get_nm_sect_hdrs(); i++){
        if(p_section_header[i]){
            delete p_section_header[i];
            p_section_header[i] = NULL;
        }
    }

    if(p_section_header){
        delete [] p_section_header;
        p_section_header = NULL;
    }

    if(p_symbol_table){
        delete p_symbol_table;
        p_symbol_table = NULL;
    }

    if(p_string_table){
        delete p_string_table;
        p_string_table = NULL;
    }

    if(p_opt_file_header){
        delete p_opt_file_header;
        p_opt_file_header = NULL;
    }

    if(p_file_header){
        delete p_file_header;
        p_file_header = NULL;
    }

    if(p_file_out){
        p_file_out->close();
        delete p_file_out;
        p_file_out = NULL; 
    }

    if(p_file_in){
        p_file_in->close();
        delete p_file_in;
        p_file_in = NULL;
    }
}

coff_sect_entry * coff_reader :: fetch_next_instr()
{
    static int next_instr_num = 0;
    static coff_section * ptr_text_section = NULL;

    if(!ptr_text_section)
    {
        for(int i=0; i<p_file_header->get_nm_sect_hdrs(); i++){
            if(strcmp(p_section_header[i]->get_name(), (const char*)".text") == 0){
                ptr_text_section = p_section[i];
                break;
            }
        }
    }

    if(!ptr_text_section){
        DOUT << "Error Finding Text Section" << endl;
        return (NULL);
    }

    if(next_instr_num < ptr_text_section->get_nm_entries()){
        return(ptr_text_section->get_sect_entry(next_instr_num++));
    }
    else{
        return(NULL); 
    }
}

int coff_reader :: get_instr_count()
{
    coff_section * ptr_text_section = NULL;

    if(!ptr_text_section)
    {
        for(int i=0; i<p_file_header->get_nm_sect_hdrs(); i++){
            if(strcmp(p_section_header[i]->get_name(), (const char*)".text") == 0){
                ptr_text_section = p_section[i];
                break;
            }
        }
    }

    if(!ptr_text_section){
        DOUT << "Error Finding Text Section" << endl;
        return (NULL);
    }

    return(ptr_text_section->get_nm_entries()); 
}



