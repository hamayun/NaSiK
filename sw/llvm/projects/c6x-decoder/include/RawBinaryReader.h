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

#ifndef RAW_BINARY_READER_H
#define RAW_BINARY_READER_H

#include <string.h>
#include <fstream>
#include <map>

#include "Common.h"
#include "Instruction.h"
#include "BinaryReader.h"

typedef map<string, uint32_t>               SectionMap_t;
typedef map<string, uint32_t>::iterator     SectionMap_Iterator_t;

namespace native
{
    class RawBinaryReader : public BinaryReader
    {
    protected:
        RawBinaryReader(){};

    public:
        RawBinaryReader(string input_binary_name);

        // This function should return the in memory handle for a given section name
        virtual uint32_t * GetSectionHandle(string section_name) { return (NULL); }

        // To find the Virtual Address (of the input binary) of the first instruction/data entry.
        virtual uint32_t GetSectionStartAddress(uint32_t * section_handle) { return (0x0); }

        // To find the Virtual Entry Point Address of the input binary; From where to Start Execution !!!
        virtual uint32_t GetEntryPoint() { return (0x0); }

        // If this Address (Abort Point) is Reached ... the Simulator Should Stop.
        virtual uint32_t GetExitPoint() { return (0xFFFFFFFF); }

        // If this Address (CIO Flush Point) is Reached ... We Should Flush the CIO Buffer
        virtual uint32_t GetCIOFlushPoint() { return (0xFFFFFFFF); }

        // This should give us the I/O Buffer Address in Target Memory.
        // Like _CIOBUF_ or .cio section address in COFF Binary Format or equivalent
        virtual uint32_t GetCIOBufferAddr() { return (0xFFFFFFFF); }

        // Every subclass must implement its own Read function.
        virtual Instruction * Read(uint32_t * section_handle, uint32_t address);

        virtual int32_t DumpSection(string infile, string outfile, string section_name);

        virtual ~RawBinaryReader() {}
    };
}
#endif // RAW_BINARY_READER_H
