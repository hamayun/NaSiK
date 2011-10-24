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

#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include "Common.h"

namespace native
{
    class StringTable
    {
    public:
        StringTable() {}
        virtual ~StringTable() {}
    };

    class SymbolTable
    {
    public:
        SymbolTable() {}

        virtual int32_t Read(ifstream *file, uint32_t symtab_start_addr, StringTable * string_table) const { DOUT << "Not Implemented !!!"; return (0); }

        virtual void Print (ostream *out, int32_t nm_entries) const { DOUT << "Not Implemented !!!"; }

        // The following function returns the symbol but return null for hidden symbols
        virtual char * GetSymbol(uint32_t address) const { DOUT << "Not Implemented !!!"; return (NULL); }

        // The following function does not hide any symbol; except for .debug ones
        virtual char * GetRawSymbol(uint32_t address) const { DOUT << "Not Implemented !!!"; return (NULL); }

        virtual ~SymbolTable() {}
    };

}
#endif // SYMBOL_TABLE
