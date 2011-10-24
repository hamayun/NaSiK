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

#include "COFFBinaryReader.h"
#include "COFFBinaryReaderMisc.h"

namespace native
{
    COFFBinaryReader :: COFFBinaryReader(string input_binary_name) : BinaryReader(input_binary_name)
    {
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

        if(p_file_header->read(GetInputFileHandle()))
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

            if(p_opt_file_header->read(GetInputFileHandle()))
            {
                DOUT << "Error: Reading Optional COFF File Header" << endl;
                return;
            }

            p_opt_file_header->print(p_file_out);
        }

        // Create String Table
        int strtab_start = p_file_header->get_str_tbl_start();
        int strings_count = COFFStringTable :: coff_get_strings_count(GetInputFileHandle(), strtab_start);
        if(strings_count == -1){
            DOUT << "Error: Invalid Number of String Count" << endl;
            return;
        }

        p_string_table = new COFFStringTable(strings_count);
        if(!p_string_table){
            DOUT << "Error: Allocating Memory for String Table" << endl;
            return;
        }

        if(p_string_table->read(GetInputFileHandle(), strtab_start)){
            DOUT << "Error: Reading String Table" << endl;
            return;
        }
        p_string_table->print(p_file_out, COFFStringTable :: get_str_entries_count());

        // Create Symbol Table Here
        p_symbol_table = new COFFSymbolTable(p_file_header->get_nm_sym_tbl_entries());
        if(!p_symbol_table){
            DOUT << "Error: Allocating Memory for Symbol Table" << endl;
            return;
        }
        if(p_symbol_table->Read(GetInputFileHandle(), p_file_header->get_sym_tbl_start(), p_string_table)){
            DOUT << "Error: Reading Symbol Table" << endl;
            return;
        }
        p_symbol_table->Print(p_file_out, p_file_header->get_nm_sym_tbl_entries());

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
            if(p_section_header[i]->read(GetInputFileHandle(), p_string_table)){
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
            if(p_section[i]->read(GetInputFileHandle())){
                DOUT << "Error: Reading Section" << endl;
                return;
            }
            //p_section[i]->print(p_file_out);
        }

        // Now create the sections names to in-memory section structures mapping
        for(uint32_t i = 0; i < p_file_header->get_nm_sect_hdrs(); i++)
        {
            string section_name = p_section_header[i]->get_name();
            m_sections_map[section_name] = (uint32_t) p_section[i];
        }

        /*
        for(SectionMap_Iterator_t SMI = m_sections_map.begin(), SME = m_sections_map.end(); SMI != SME; ++SMI)
        {
            DOUT << "Section: " << setw(18) << setfill(' ') << SMI->first
                 << "  Mapped to: " << FMT_INT << SMI->second << endl;
        }*/
    }

    COFFBinaryReader :: ~COFFBinaryReader()
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
    }

    uint32_t * COFFBinaryReader :: GetSectionHandle(string section_name)
    {
        SectionMap_Iterator_t EPI = m_sections_map.find(section_name);
        if(EPI != m_sections_map.end())
        {
            return ((uint32_t *)EPI->second);
        }

        DOUT << "Error: Section Not Found" << endl;
        return (NULL);
    }

    uint32_t COFFBinaryReader :: GetSectionEntryAddress(uint32_t * section_handle)
    {
        coff_section * p_section = (coff_section *) section_handle;

        if(p_section->get_nm_entries())
        {
            return (p_section->get_sect_entry(0)->get_vir_addrs());
        }

        return (NULL);
    }

    Instruction * COFFBinaryReader :: Read(uint32_t * section_handle, uint32_t address)
    {
        coff_section * p_section   = (coff_section * ) section_handle;
        Instruction *  instruction = p_section->get_instruction_by_address(address, this);

        if(!instruction)
        {
            //DOUT << "Error: Could not Get Instruction By Address" << endl;
            return NULL;
        }

        return (instruction);
    }

    coff2_section_header * COFFBinaryReader :: get_section_header(char * name)
    {
        for(int i=0; i<p_file_header->get_nm_sect_hdrs(); i++)
        {
            if(strcmp(p_section_header[i]->get_name(), name) == 0)
            {
                return (p_section_header[i]);
            }
        }
        return (NULL);
    }

    coff_sect_entry * COFFBinaryReader :: fetch_next_instr()
    {
        static uint32_t next_instr_num = 0;
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

    uint32_t COFFBinaryReader :: get_instr_count()
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
}
