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

#ifndef COFF_BINARY_READER_H
#define COFF_BINARY_READER_H

#include <iostream>
#include <iomanip>
#include <string.h>

#include "BinaryReader.h"
#include "SymbolTable.h"

namespace native
{
    #define COFF_FILE_HDR_SIZE          22
    #define COFF_OPT_FILE_HDR_SIZE      28
    #define COFF2_SECTION_SIZE          48
    #define COFF_SYMTAB_ENTRY_SIZE      18
    #define COFF_MAX_STR_SIZE           256

    #define VIR_ADDR_SIZE               4
    #define PHY_ADDR_SIZE               4

    class coff_file_header;
    class coff_opt_file_header;
    class COFFSymbolTable;
    class COFFStringTable;
    class coff2_section_header;
    class coff_section;
    class coff_sect_entry;

    class COFFBinaryReader : public BinaryReader
    {
    private:
        ofstream                *p_file_out;
        coff_file_header        *p_file_header;
        coff_opt_file_header    *p_opt_file_header;
        COFFStringTable         *p_string_table;
        coff2_section_header   **p_section_header;
        coff_section           **p_section;

        COFFBinaryReader(){}     // Private so can't initialize object without filename

    public:
        COFFBinaryReader(string input_binary_name);
        virtual ~COFFBinaryReader();
        virtual uint32_t *GetSectionHandle(string section_name);
        virtual uint32_t  GetSectionEntryAddress(uint32_t * section_handle);
        virtual Instruction * Read(uint32_t * section_handle, uint32_t address);

        coff2_section_header * get_section_header(char * name);

        coff_file_header * get_file_header(){
            return(p_file_header);
        }

        coff_opt_file_header * get_opt_file_header(){
            return(p_opt_file_header);
        }

        COFFStringTable * get_string_table(){
            return(p_string_table);
        }

        uint32_t get_instr_count();
        coff_sect_entry * fetch_next_instr();
    };
}
#endif // COFF_BINARY_READER_H
