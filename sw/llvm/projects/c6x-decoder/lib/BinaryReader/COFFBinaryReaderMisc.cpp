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
    int coff2_section_header :: read(ifstream *file, COFFStringTable *p_string_table)
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
        set_nm_reloc_entries(*(uint32_t*)&buff[32]);
        set_nm_line_entries(*(uint32_t*)&buff[36]);
        set_flags(*(uint32_t*)&buff[40]);
        set_reserved_2(*(unsigned short*)&buff[44]);
        set_mem_page_nb(*(unsigned short*)&buff[46]);

        return (0);
    }

    // class coff_symtab_entry
    int coff_symtab_entry :: sym_entries_count = 0;
    int coff_symtab_entry :: aux_entries_count = 0;

    int coff_symtab_entry :: read(ifstream *file, COFFStringTable *p_string_table)
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
    int32_t COFFSymbolTable :: Read(ifstream *file, uint32_t symtab_start_addr, StringTable * string_table) const
    {
            int fptr_loc = file->tellg();
            if(fptr_loc == -1){
                DOUT << "Error: Invalid File Position" << endl;
                return (-1);
            }

            // Move File Pointer to Start of Symbol Table
            file->seekg(symtab_start_addr, ios::beg);

            for(int i=0; i<m_nm_entries; i++){
                if(p_symtab[i].read(file, (COFFStringTable *) string_table))
                    return (-1);
            }

            // Restore File Pointer to Old Value
            file->seekg(fptr_loc, ios::beg);
            return(0);
    }

    // class COFFStringTable
    int COFFStringTable :: str_entries_count = 0;

    int COFFStringTable :: coff_get_strings_count(ifstream *file, int strtab_start_addr)
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

    int COFFStringTable :: read(ifstream *file, int strtab_start_addr)
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
        uint32_t        m_cur_vir_addr = get_section_header()->get_vir_addr();
        uint32_t        m_cur_phy_addr = get_section_header()->get_phy_addr();
        uint32_t        value;

        int fptr_loc = file->tellg();
        if(fptr_loc == -1){
            DOUT << "Error: Invalid File Position" << endl;
            return (-1);
        }

        // Move File Pointer to Start of Section
        file->seekg(get_section_header()->get_fptr_raw_data(), ios::beg);

        for(int i=0; i<m_nm_entries; i++){
            file->read((char*)&value, sizeof(uint32_t));
            if(file->eof() || file->fail()){
                DOUT << "Error: Reading Instruction From File" << endl;
                return (-1);
            }

            p_sect_entries[i] = new coff_sect_entry(m_cur_vir_addr, m_cur_phy_addr, value);
            if(!p_sect_entries[i]){
                DOUT << "Error: Allocating Memory for Instruction Object" << endl;
                return(-1);
            }

            m_sect_entries_map[m_cur_vir_addr] = value;

            m_cur_vir_addr += VIR_ADDR_SIZE;
            m_cur_phy_addr += PHY_ADDR_SIZE;
        }

        // Restore File Pointer to Old Value
        file->seekg(fptr_loc, ios::beg);
        return(0);
    }
}
