
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

#ifndef COMMON_H
#define	COMMON_H

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
#include <iostream>
#include <sstream>
#include <iomanip>
#include <list>
#include <set>
#include <map>
using namespace std;
#endif

//#define PRINT_INSTR_OPCODE
//#define PRINT_DELAY_SLOTS
#define PRINT_BRANCH_TARGET_HINTS
#define GEN_FUNC_INFO 0

//#define DOUT            cerr << __PRETTY_FUNCTION__ << " "
#define DOUT            cout << "<" << __func__ << ">: "
#define COUT            cout << "<" << __func__ << ">: "
#define WARN            cerr << __func__ << "():" << __LINE__ << " :: WARNING !!! "
#define INFO            if(GEN_FUNC_INFO) cout

#define FMT_CHAR        "0x" << hex << setfill('0') << setw(2)
#define FMT_SHORT       "0x" << hex << setfill('0') << setw(4)
#define FMT_INT         "0x" << hex << setfill('0') << setw(8)

#ifdef __cplusplus
#define ASSERT(condition,msg) if(!(condition))                              \
{                                                                           \
    DOUT << "Assertion Failed in " << __FILE__ << " at " << __LINE__        \
         << ": " << msg << endl;                                            \
    exit(1);                                                                \
}
#else
#define ASSERT(condition,msg) if(!(condition))                              \
{                                                                           \
    printf("Assertion Failed in %s at %d: %s\n", __FILE__, __LINE__, msg);  \
    exit(1);                                                                \
}
#endif
#endif	/* COMMON_H */