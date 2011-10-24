
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
        ifstream *input_binary = GetInputFileHandle();

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
}
