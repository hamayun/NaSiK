
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

#ifndef BINARY_READER_H
#define BINARY_READER_H

#include <fstream>

#include "Common.h"
#include "SymbolTable.h"

namespace native
{
    typedef map<string, uint32_t>               SectionMap_t;
    typedef map<string, uint32_t>::iterator     SectionMap_Iterator_t;

    class Instruction;
    class BinaryReader
    {
    private:
        ifstream                *p_input_binary;      // Read from this File

    protected:
        BinaryReader() : p_input_binary(NULL), p_symbol_table(NULL) {};
        SymbolTable             *p_symbol_table;
        SectionMap_t             m_sections_map;      // This will keep track of binary section
                                                      // names to in-memory structure pointers
    public:
        BinaryReader(string input_binary_name);
        ifstream * GetInputFileHandle() { return (p_input_binary); }

        // This function should return the in memory handle for a given section name
        virtual uint32_t *GetSectionHandle(string section_name) = 0;

        // To find the Virtual Address (of the input binary) of the first instruction/data entry.
        virtual uint32_t GetSectionStartAddress(uint32_t * section_handle) = 0;

        // To find the Virtual Entry Point Address of the input binary; From where to Start Execution !!!
        virtual uint32_t GetEntryPoint() = 0;

        // If this Address (Abort Point) is Reached ... the Simulator Should Stop.
        virtual uint32_t GetExitPoint() { ASSERT(0, "Not Implemented"); }

        virtual SymbolTable * GetSymbolTable() { return (p_symbol_table); }

        // Every subclass must implement its own Read function.
        virtual Instruction * Read(uint32_t * section_handle, uint32_t address) = 0;

        // Dump A Given Section to file.
        virtual int32_t DumpSection(string infile, string outfile, string section_name) { ASSERT(0, "Not Implemented"); }

        virtual ~BinaryReader();
    };
}
#endif // BINARY_READER_H
