
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

#include "RawBinaryReader.h"

namespace native
{
    RawBinaryReader :: RawBinaryReader(string input_binary_name) :
        BinaryReader(input_binary_name) {}

    // Every subclass must implement its own Read function.
    Instruction * RawBinaryReader :: Read(uint32_t * section_handle, uint32_t address)
    {
        uint32_t binary_instr = 0;
        ifstream * input_binary = GetInputFileHandle();

        // Move File Pointer the Address.
        input_binary->seekg(address, ios::beg);

        input_binary->read((char *) & binary_instr, sizeof(uint32_t));
        if(input_binary->eof() || input_binary->fail())
        {
            //DOUT << "Error: Reading Instruction From File" << endl;
            return (NULL);
        }

        return (new Instruction(address, binary_instr, this));
    }

    int32_t RawBinaryReader :: DumpSection(string infile, string outfile, string section_name)
    {
        uint32_t        binary_value     = 0;
        uint32_t        binary_address   = 0x0;         // Where this section should be loaded ?
        uint32_t        binary_size      = 0;
        string          output_file_name = outfile + section_name;

        if(strcmp(section_name.c_str(), ".text") != 0)
        {
            // Because RAW Binary Contains .text only.
            return (-1);
        }

        ifstream * input_binary = new ifstream(infile.c_str(), ios::in | ios::binary);
        if(! input_binary->is_open())
        {
            DOUT << "Error: Opening Input File: " << infile.c_str() << endl;
            return (-1);
        }

        ofstream * output_binary = new ofstream(output_file_name.c_str(), ios::out | ios::binary);
        if(! output_binary->is_open())
        {
            DOUT << "Error: Opening Output File: " << output_file_name.c_str() << endl;
            return (-1);
        }

        // Move File Pointer the Beginning Address.
        input_binary->seekg(0, ios::beg);

        // First Four Bytes indicate the Address where this Section should be loaded in KVM Memory
        // And for RAW input binaries this Address is 0x00000000
        output_binary->write((char *) & binary_address, sizeof(uint32_t));
        if(output_binary->fail())
        {
            DOUT << "Error: Writing to Output File: " << output_file_name.c_str() << endl;
            return (-1);
        }

        // The Next Four Bytes indicate the Size of this section; Will be written later;
        // For the moment write Zeros.
        output_binary->write((char *) & binary_value, sizeof(uint32_t));
        if(output_binary->fail())
        {
            DOUT << "Error: Writing to Output File: " << output_file_name.c_str() << endl;
            return (-1);
        }

        while(! input_binary->eof())
        {
            input_binary->read((char *) & binary_value, sizeof(uint32_t));
            if(input_binary->fail() || input_binary->eof())
                break;

            binary_size += sizeof(uint32_t);

            output_binary->write((char *) & binary_value, sizeof(uint32_t));
            if(output_binary->fail())
                break;
        }

        // Move File Pointer the Location Where we Write the Section Size.
        output_binary->seekp(sizeof(uint32_t), ios::beg);

        // Now write the actual size.
        output_binary->write((char *) & binary_size, sizeof(uint32_t));
        if(output_binary->fail())
        {
            DOUT << "Error: Writing to Output File: " << output_file_name.c_str() << endl;
            return (-1);
        }

        if(output_binary)
        {
            output_binary->close();
            delete output_binary;
            output_binary = NULL;
        }

        if(input_binary)
        {
            input_binary->close();
            delete input_binary;
            input_binary = NULL;
        }

        return (0);
    }
}
