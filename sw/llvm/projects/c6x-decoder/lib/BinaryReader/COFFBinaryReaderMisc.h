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

#ifndef COFF_BINARY_READER_MISC_H
#define COFF_BINARY_READER_MISC_H

#include "BinaryReader.h"
#include "COFFBinaryReader.h"
#include "Instruction.h"

namespace native
{
    class coff_file_header
    {
    private:
        uint16_t      m_vers_id;
        uint16_t      m_nm_sect_hdrs;
        int           m_time_stamp;
        int           m_sym_tbl_start;
        int           m_nm_sym_tbl_entries;
        uint16_t      m_nm_opt_hdr_bytes;
        uint16_t      m_flags;
        uint16_t      m_target_id;

    public:
        coff_file_header(){}
        ~coff_file_header(){}

        int read(ifstream *file);

        void print (ostream *out){
            (*out) << "--- COFF FILE HEADER --- " << endl;
            (*out) << "Version ID: " << FMT_SHORT << m_vers_id << endl;
            (*out) << "Section Headers: " << FMT_SHORT << m_nm_sect_hdrs << endl;
            (*out) << "Time Stamp: " << FMT_INT << m_time_stamp << endl;
            (*out) << "Symbol Table: " << FMT_INT << m_sym_tbl_start << endl;
            (*out) << "Symbol Entries: " << FMT_INT << m_nm_sym_tbl_entries << endl;
            (*out) << "Optional Header: " << FMT_SHORT << m_nm_opt_hdr_bytes << endl;
            (*out) << "Flags: " << FMT_SHORT << m_flags << endl;
            (*out) << "Target ID: " << FMT_SHORT << m_target_id << endl << endl;
        }

        void set_vers_id(uint16_t vers_id){
            m_vers_id = vers_id;
        }

        void set_nm_sect_hdrs(uint16_t nm_sect_hdrs){
            m_nm_sect_hdrs = nm_sect_hdrs;
        }

        void set_time_stamp(int time_stamp){
            m_time_stamp = time_stamp;
        }

        void set_sym_tbl_start(int sym_tbl_start){
            m_sym_tbl_start = sym_tbl_start;
        }

        void set_nm_sym_tbl_entries(int nm_sym_tbl_entries){
            m_nm_sym_tbl_entries = nm_sym_tbl_entries;
        }

        void set_nm_opt_hdr_bytes(uint16_t nm_opt_hdr_bytes){
            m_nm_opt_hdr_bytes = nm_opt_hdr_bytes;
        }

        void set_flags(uint16_t flags){
            m_flags = flags;
        }

        void set_target_id(uint16_t target_id){
            m_target_id = target_id;
        }

        uint16_t get_vers_id(){
            return(m_vers_id);
        }

        uint16_t get_nm_sect_hdrs(){
            return(m_nm_sect_hdrs);
        }

        int get_time_stamp(){
            return(m_time_stamp);
        }

        int get_sym_tbl_start(){
            return(m_sym_tbl_start);
        }

        int get_nm_sym_tbl_entries(){
            return(m_nm_sym_tbl_entries);
        }

        uint16_t get_nm_opt_hdr_bytes(){
            return(m_nm_opt_hdr_bytes);
        }

        uint16_t get_flags(){
            return(m_flags);
        }

        uint16_t get_target_id(){
            return(m_target_id);
        }

        int get_str_tbl_start(){
            return(get_sym_tbl_start() + (get_nm_sym_tbl_entries() * COFF_SYMTAB_ENTRY_SIZE));
        }
    };

    class coff_opt_file_header
    {
    private:
        short           m_magic;
        short           m_vers_stamp;
        int             m_exe_code_size;
        int             m_init_data_size;
        int             m_uninit_data_size;
        int             m_entry_point;
        int             m_exe_code_addr;
        int             m_init_data_addr;

    public:
        coff_opt_file_header(){}
        ~coff_opt_file_header(){}

        int read(ifstream *file);

        void print (ostream *out){
            (*out) << "--- COFF OPTIONAL FILE HEADER --- " << endl;
            (*out) << "Magic: " << FMT_SHORT << m_magic << endl;
            (*out) << "Version Stamp: " << FMT_SHORT << m_vers_stamp << endl;
            (*out) << "Exe Code Size: " << FMT_INT << m_exe_code_size << endl;
            (*out) << "Init Data Size: " << FMT_INT << m_init_data_size << endl;
            (*out) << "Uninit Data Size: " << FMT_INT << m_uninit_data_size << endl;
            (*out) << "Entry Point: " << FMT_INT << m_entry_point << endl;
            (*out) << "Exe Code Address: " << FMT_INT << m_exe_code_addr << endl;
            (*out) << "Init Data Address: " << FMT_INT << m_init_data_addr << endl << endl;
        }

        void set_magic(short magic){
            if(magic != 0x0108)
                DOUT << "Error: Wrong Optional Header Magic" << endl;
            else
                m_magic = magic;
        }

        void set_vers_stamp(short vers_stamp){
            m_vers_stamp = vers_stamp;
        }

        void set_exe_code_size(int exe_code_size){
            m_exe_code_size = exe_code_size;
        }

        void set_init_data_size(int init_data_size){
            m_init_data_size = init_data_size;
        }

        void set_uninit_data_size(int uninit_data_size){
            m_uninit_data_size = uninit_data_size;
        }

        void set_entry_point(int entry_point){
            m_entry_point = entry_point;
        }

        void set_exe_code_addr(int exe_code_addr){
            m_exe_code_addr = exe_code_addr;
        }

        void set_init_data_addr(int init_data_addr){
            m_init_data_addr = init_data_addr;
        }

        short get_magic(){
            if(m_magic != 0x0108)
                DOUT << "Error: Wrong Optional Header Magic" << endl;
            return(m_magic);
        }

        short get_vers_stamp(){
            return(m_vers_stamp);
        }

        int get_exe_code_size(){
            return(m_exe_code_size);
        }

        int get_init_data_size(){
            return(m_init_data_size);
        }

        int get_uninit_data_size(){
            return(m_uninit_data_size);
        }

        int get_entry_point(){
            return(m_entry_point);
        }

        int get_exe_code_addr(){
            return(m_exe_code_addr);
        }

        int get_init_data_addr(){
            return(m_init_data_addr);
        }
    };

    class coff_strtab_entry
    {
    private:
        int         m_offset;
        int         m_size;
        char       *p_str;
    public:
        coff_strtab_entry(int offset, char *str, int size){
            m_offset = offset;
            m_size = size;
            p_str = new char [m_size];
            if(!p_str){
                DOUT << "Error: Allocating Memory for String Table Entry" << endl;
                return;
            }
            memcpy(p_str, str, m_size);
        }

        ~coff_strtab_entry(){
            if(p_str){
                delete p_str;
                p_str = NULL;
            }
        }

        int get_offset(){
            return(m_offset);
        }

        int get_size(){
            return(m_size);
        }

        char * get_string(){
            return(p_str);
        }

        void print (ostream *out){
            (*out) << setfill(' ') << "[" << dec << setw(6) << m_offset << "] " << "\t" << p_str << endl;
        }
    };

    class COFFStringTable : public StringTable
    {
    private:
        coff_strtab_entry   **p_strtab;
        int                   m_nm_str_entries;
        static int            str_entries_count;

    public:
        COFFStringTable(int nm_str_entries)
        {
            m_nm_str_entries = nm_str_entries;

            p_strtab = new coff_strtab_entry * [m_nm_str_entries];
            if(!p_strtab)
            {
                DOUT << "Error: Allocating Pointers Array for String Table Entries" << endl;
                return;
            }
            memset(p_strtab, 0x0, sizeof(coff_strtab_entry *) * m_nm_str_entries);
        }

        ~COFFStringTable()
        {
            for(int i=0; i<m_nm_str_entries; i++)
            {
                if(p_strtab[i])
                {
                    delete p_strtab[i];
                    p_strtab[i] = NULL;
                }
            }

            if(p_strtab)
            {
                delete [] p_strtab;
                p_strtab = NULL;
            }
        }

        int read(ifstream *file, int strtab_start_addr);
        static int coff_get_strings_count(ifstream *file, int strtab_start_addr);

        int add_strtab_entry(coff_strtab_entry * strtab_entry)
        {
            ASSERT(strtab_entry != NULL, "NULL String Table Entry");

            if(str_entries_count < m_nm_str_entries)
            {
                p_strtab[str_entries_count++] = strtab_entry;
                return(0);
            }
            return(-1);
        }

        coff_strtab_entry * get_strtab_entry(int offset)
        {
            for(int i=0; i<str_entries_count; i++)
            {
                if(p_strtab[i]->get_offset() == offset)
                {
                    return(p_strtab[i]);
                }
            }
            return(NULL);
        }

        static int get_str_entries_count()
        {
            return (str_entries_count);
        }

        void print (ostream *out, int nm_entries){
            // Take Which ever is smaller; So we don't pass over the end of String Table
            nm_entries = (nm_entries < m_nm_str_entries ? nm_entries : m_nm_str_entries);

            (*out) << "[Offset] "   << "\tString"  << endl;

            for(int i=0; i<nm_entries; i++){
                p_strtab[i]->print(out);
            }

            (*out) << "Total String Table Entries: " << dec << COFFStringTable :: get_str_entries_count() << endl;
        }

    };

    class coff2_section_header
    {
    private:
        char            m_name[8];
        char           *p_name_str;
        int             m_phy_addr;
        int             m_vir_addr;
        int             m_size;
        int             m_fptr_raw_data;
        int             m_fptr_reloc_entries;
        int             m_reserved_1;
        uint32_t    m_nm_reloc_entries;
        uint32_t    m_nm_line_entries;
        uint32_t    m_flags;
        uint16_t  m_reserved_2;
        uint16_t  m_mem_page_nb;
    public:
        coff2_section_header(){
            p_name_str = NULL;
        }
        ~coff2_section_header(){
        }

        int read(ifstream *file, COFFStringTable *p_string_table);

        void print (ostream *out, short sec_num){
            (*out) << "--- COFF SECTION HEADER --- [" << FMT_SHORT << sec_num << "]" << endl;
            (*out) << "Section Name: " << (p_name_str?p_name_str:m_name) << endl;
            (*out) << "Physical Address: " << FMT_INT << m_phy_addr << endl;
            (*out) << "Virtual Address: " << FMT_INT << m_vir_addr << endl;
            (*out) << "Section Size: " << FMT_INT << m_size << endl;
            (*out) << "File PTR Raw Data: " << FMT_INT << m_fptr_raw_data << endl;
            (*out) << "File PTR Reloc Entries: " << FMT_INT << m_fptr_reloc_entries << endl;
            (*out) << "Reserved #1: " << FMT_INT << m_reserved_1 << endl;
            (*out) << "Num Reloc Entries: " << FMT_INT << m_nm_reloc_entries << endl;
            (*out) << "Num Line Entries: " << FMT_INT << m_nm_line_entries << endl;
            (*out) << "Flags: " << FMT_INT << m_flags << endl;
            (*out) << "Reserved #2: " << FMT_SHORT << m_reserved_2 << endl;
            (*out) << "Mem Page Number: " << FMT_SHORT << m_mem_page_nb << endl << endl;
        }

        void set_name(char *name){
            memcpy(m_name, name, 8);
        }

        void set_name_str(char *name){
            p_name_str = name;
        }

        void set_phy_addr(int phy_addr){
            m_phy_addr = phy_addr;
        }

        void set_vir_addr(int vir_addr){
            m_vir_addr = vir_addr;
        }

        void set_size(int size){
            m_size = size;
        }

        void set_fptr_raw_data(int fptr_raw_data){
            m_fptr_raw_data = fptr_raw_data;
        }

        void set_fptr_reloc_entries(int fptr_reloc_entries){
            m_fptr_reloc_entries = fptr_reloc_entries;
        }

        void set_reserved_1(int reserved){
            m_reserved_1 = reserved;
        }

        void set_nm_reloc_entries(uint32_t nm_reloc_entries){
            m_nm_reloc_entries = nm_reloc_entries;
        }

        void set_nm_line_entries(uint32_t nm_line_entries){
            m_nm_line_entries = nm_line_entries;
        }

        void set_flags(uint32_t flags){
            m_flags = flags;
        }

        void set_reserved_2(uint16_t reserved){
            m_reserved_2 = reserved;
        }

        void set_mem_page_nb(uint16_t mem_page_nb){
            m_mem_page_nb = mem_page_nb;
        }

        char * get_name(){
            return(p_name_str?p_name_str:m_name);
        }

        int get_phy_addr(){
            return(m_phy_addr);
        }

        int get_vir_addr(){
            return(m_vir_addr);
        }

        int get_size(){
            return(m_size);
        }

        int get_fptr_raw_data(){
            return(m_fptr_raw_data);
        }

        int get_fptr_reloc_entries(){
            return(m_fptr_reloc_entries);
        }

        int get_reserved_1(){
            return(m_reserved_1);
        }

        uint32_t get_nm_reloc_entries(){
            return(m_nm_reloc_entries);
        }

        uint32_t get_nm_line_entries(){
            return(m_nm_line_entries);
        }

        uint32_t get_flags(){
            return(m_flags);
        }

        uint16_t get_reserved_2(){
            return(m_reserved_2);
        }

        uint16_t get_mem_page_nb(){
            return(m_mem_page_nb);
        }
    };

    class coff_symtab_entry
    {
    private:
        char                m_name[8];
        char               *p_name_str;
        uint32_t            m_sym_val;
        short               m_sec_num;
        uint16_t      m_reserved;
        char                m_storage_class;
        char                m_nm_aux_entries;

        static int          sym_entries_count;
        static int          aux_entries_count;

        void print_chars_hex(ostream *out, char *str, int len){
            for(int i=0; i<len; i++){
                (*out) << hex << setfill('0') << setw(2) << ((short) str[i] & 0x00FF) << " ";
            }
        }

    public:
        coff_symtab_entry(){
            p_name_str = NULL;
        }
        ~coff_symtab_entry(){}

        int read(ifstream *file, COFFStringTable *p_string_table);

        void print (ostream *out, int sym_num){
            (*out) << setfill(' ') << "[" << dec << setw(6) << sym_num << "]  ";
            print_chars_hex(out, m_name, 8);
            (*out) <<       "  0x" << hex << setfill('0') << setw(8) << m_sym_val;
            (*out) <<   "      0x" << hex << setfill('0') << setw(4) << m_sec_num;
            (*out) <<   "      0x" << hex << setfill('0') << setw(4) << m_reserved;
            (*out) << "        0x" << hex << setfill('0') << setw(2) << (short) m_storage_class;
            (*out) << "        0x" << hex << setfill('0') << setw(2) << (short) m_nm_aux_entries;
            if(p_name_str) (*out) << "  " << p_name_str;
            (*out) << endl;
        }

        void set_name(char *name){
            memcpy(m_name, name, 8);
        }

        void set_name_str(char *name){
            p_name_str = name;
        }

        void set_sym_val(uint32_t sym_val){
            m_sym_val = sym_val;
        }

        void set_sec_num(short sec_num){
            m_sec_num = sec_num;
        }

        void set_reserved(uint16_t reserved){
            m_reserved = reserved;
        }

        void set_storage_class(char storage_class){
            m_storage_class = storage_class;
        }

        void set_nm_aux_entries(char nm_aux_entries){
            m_nm_aux_entries = nm_aux_entries;
        }

        char * get_name_str(){
            return(p_name_str?p_name_str:m_name);
        }

        uint32_t get_sym_val(){
            return(m_sym_val);
        }

        short get_sec_num(){
            return(m_sec_num);
        }

        uint16_t get_reserved(){
            return(m_reserved);
        }

        char get_storage_class(){
            return(m_storage_class);
        }

        char get_nm_aux_entries(){
            return(m_nm_aux_entries);
        }

        static int get_tot_sym_entries(){
            return (sym_entries_count);
        }

        static int get_tot_aux_entries(){
            return (aux_entries_count);
        }
    };

    class COFFSymbolTable : public SymbolTable
    {
    private:
        int                     m_nm_entries;
        coff_symtab_entry      *p_symtab;

    public:
        COFFSymbolTable(int nm_entries){
            m_nm_entries = nm_entries;

            p_symtab = new coff_symtab_entry[m_nm_entries];
            if(!p_symtab){
                DOUT << "Error: Allocating Memory for Symbol Table" << endl;
                return;
            }
        }

        ~COFFSymbolTable(){
            delete [] p_symtab;
            p_symtab = NULL;
        }

        virtual int32_t Read(ifstream *file, uint32_t symtab_start_addr, StringTable * string_table) const;

        virtual void Print (ostream *out, int32_t nm_entries) const
        {
            // Take Which ever is smaller; So we don't pass over the end of Symbol Table
            nm_entries = (nm_entries < m_nm_entries ? nm_entries : m_nm_entries);

            (*out) << setfill(' ')
                   << setw(8)  << "Symbol #"  << setw(26) << "Name "    << setw(12) << "Value"
                   << setw(12) << "Section"   << setw(12) << "Reserved" << setw(12) << "Storage"
                   << setw(12) << "Auxiliary" <<     "  " << "Name String" << endl;

            for(int i=0; i<nm_entries; i++){
                p_symtab[i].print(out, i);
            }

            (*out) << "Total Symbol Table Entries: " << dec << coff_symtab_entry :: get_tot_sym_entries() << endl;
            (*out) << "Total Auxiliary Entries: " << dec << coff_symtab_entry :: get_tot_aux_entries() << endl;
        }

        void set_symtab_entry(int entry_num, coff_symtab_entry * symtab_entry){
            memcpy(&p_symtab[entry_num], symtab_entry, sizeof(coff_symtab_entry));
        }

        coff_symtab_entry * get_symtab_entry(int entry_num){
            return (&p_symtab[entry_num]);
        }

        coff_symtab_entry * get_symtab_entry_byval(uint32_t sym_val) const
        {
            for(int i=0; i<m_nm_entries; i++)
            {
                if(p_symtab[i].get_sym_val() == sym_val)
                    return (&p_symtab[i]);
            }
            return NULL;
        }

        virtual char * GetSymbol(uint32_t address) const
        {
            coff_symtab_entry * ptr_symtab_entry = get_symtab_entry_byval(address);

            if(ptr_symtab_entry){
                char * symbol_name = ptr_symtab_entry->get_name_str();
                if(is_hidden_label(symbol_name))
                    return (NULL);
                else
                    return(symbol_name);
            }

            return(NULL);
        }

        uint32_t FindSymbolAddr(string sym_name) const
        {
            for(int i=0; i < m_nm_entries; i++)
            {
                if(strcmp(p_symtab[i].get_name_str(), sym_name.c_str()) == 0)
                    return (p_symtab[i].get_sym_val());
            }
            return 0x0;
        }

        virtual char * GetRawSymbol(uint32_t address) const
        {
            coff_symtab_entry * ptr_symtab_entry = get_symtab_entry_byval(address);

            if(ptr_symtab_entry){
                char * symbol_name = ptr_symtab_entry->get_name_str();
                if(memcmp(symbol_name, ".debug", 6) != 0)
                    return(symbol_name);
            }

            return(NULL);
        }

        bool is_hidden_label(char * ptr_label) const
        {
            if(memcmp((void *) ptr_label, ".text:", 6) == 0) ptr_label += 6;

            if (memcmp(ptr_label, ".debug", 6) == 0 ||
                memcmp(ptr_label, "__SYSMEM_SIZE", 13) == 0 ||
                memcmp(ptr_label, "__STACK_SIZE", 12) == 0 ||
                memcmp(ptr_label, "___isnan", 8) == 0 ||
                memcmp(ptr_label, "__pproc_", 8) == 0 ||
                memcmp(ptr_label, "__ecpy", 6) == 0 ||
                memcmp(ptr_label, "__fcpy", 6) == 0 ||
                memcmp(ptr_label, "__mcpy", 6) == 0 ||
                memcmp(ptr_label, "__pconv_", 8) == 0 ||
                memcmp(ptr_label, "_mremove", 8) == 0 ||
                memcmp(ptr_label, "_minsert", 8) == 0 ||
                memcmp(ptr_label, "__ltostr", 8) == 0 ||
                memcmp(ptr_label, "_ecvt", 5) == 0 ||
                memcmp(ptr_label, "_finddevice", 11) == 0 ||
                memcmp(ptr_label, "_fcvt", 5) == 0 ||
                memcmp(ptr_label, "__getarg_diouxp", 15) == 0 ||
                memcmp(ptr_label, "__setfield", 10) == 0 )
                return (1);
            else
                return (0);
        }

        void print_labels(uint32_t sym_val, ostream *out)
        {
            for(int i=m_nm_entries-1; i>=0; i--)
            {
                if(p_symtab[i].get_sym_val() == sym_val)
                {
                    char * ptr_label = p_symtab[i].get_name_str();

                    if(ptr_label && is_hidden_label(ptr_label)){
                        ptr_label = NULL;
                    }
                    else
                    {
                        (*out) << hex << setfill('0') << setw(8) << sym_val
                               << "\t\t" << ptr_label << ":" << endl;
                    }
                }
            }

            return;
        }
    };

    class coff_sect_entry
    {
    private:
        uint32_t            m_vir_addrs;
        uint32_t            m_phy_addrs;
        uint32_t            m_value;        // Can be Instruction or Data

    public:
        coff_sect_entry(uint32_t vir_addrs, uint32_t phy_addrs, uint32_t value){
            m_vir_addrs = vir_addrs;
            m_phy_addrs = phy_addrs;
            m_value = value;
        }

        ~coff_sect_entry(){}

        void print (ostream *out){
            (*out) << hex << setfill('0') << setw(8) << m_vir_addrs << "   ";
            (*out) << hex << setfill('0') << setw(8) << m_value;
        }

        uint32_t get_vir_addrs(){
            return(m_vir_addrs);
        }

        uint32_t get_phy_addrs(){
            return(m_phy_addrs);
        }

        uint32_t get_value(){
            return(m_value);
        }

    };

    typedef map<uint32_t, uint32_t>               COFFEntryMap_t;
    typedef map<uint32_t, uint32_t>::iterator     COFFEntryMap_Iterator_t;

    class coff_section
    {
    private:
        coff2_section_header        *p_section_header;
        int                          m_nm_entries;
        coff_sect_entry            **p_sect_entries;
        COFFEntryMap_t               m_sect_entries_map;

    public:
        coff_section(coff2_section_header * section_header){
            p_section_header = section_header;
            m_nm_entries = p_section_header->get_size() / 4;
            if(m_nm_entries){
                p_sect_entries = new coff_sect_entry * [m_nm_entries];
                if(!p_sect_entries){
                    DOUT << "Error: Allocating Memory for Section Entry" << endl;
                    return;
                }
            }
        }

        ~coff_section(){
            for(int i=0; i<m_nm_entries; i++){
                if(p_sect_entries[i]){
                    delete p_sect_entries[i];
                    p_sect_entries[i] = NULL;
                }
            }

            if(p_sect_entries){
                delete [] p_sect_entries;
                p_sect_entries = NULL;
            }

            m_sect_entries_map.clear();
        }

        int read(ifstream *file);

        void print (ostream *out){
            (*out) << "SECTION " << p_section_header->get_name() << "\t"
                   << "SIZE = " << FMT_INT << p_section_header->get_size() << "\t"
                   << "ADDRESS = " << FMT_INT << p_section_header->get_vir_addr() << endl;

            for(int i=0; i<m_nm_entries; i++){
                if(p_sect_entries[i]){
                    p_sect_entries[i]->print(out);
                }
            }
        }

        coff2_section_header * get_section_header(){
            return(p_section_header);
        }

        uint32_t get_nm_entries(){
            return(m_nm_entries);
        }

        coff_sect_entry * get_sect_entry(uint32_t entry_num){
            return(p_sect_entries[entry_num]);
        }

        Instruction * get_instruction_by_address(uint32_t address, BinaryReader * handle)
        {
            COFFEntryMap_Iterator_t CEMI = m_sect_entries_map.find(address);
            if(CEMI != m_sect_entries_map.end())
            {
                return (new Instruction(address, CEMI->second, handle));
            }

            //DOUT << "Error: COFF Entry Not Found" << endl;
            return (NULL);
        }
    };
}
#endif // COFF_BINARY_READER_MISC_H
