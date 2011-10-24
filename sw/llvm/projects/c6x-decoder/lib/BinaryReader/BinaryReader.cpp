
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

#include "BinaryReader.h"

using namespace std;

namespace native
{
    BinaryReader :: BinaryReader(string input_binary_name) : p_symbol_table(NULL)
    {
        p_input_binary = new ifstream(input_binary_name.c_str(), ios::in | ios::binary);
        if(!p_input_binary->is_open())
        {
            DOUT << "Error: Opening Input File: " << input_binary_name.c_str() << endl;
            return;
        }
    }

    BinaryReader :: ~BinaryReader()
    {
        if(p_symbol_table)
        {
            delete p_symbol_table;
            p_symbol_table = NULL;
        }

        if(p_input_binary)
        {
            p_input_binary->close();
            delete p_input_binary;
            p_input_binary = NULL;
        }
    }
}
