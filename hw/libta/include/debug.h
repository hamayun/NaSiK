/*************************************************************************************
 * File   : debug.h,     
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Patrice, GERIN patrice.gerin@imag.fr
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

#include <string>

// Debug options
#define NO_THREAD_OPTION        "no_thread"

// Options
#define ANALYZE_OPTION          "analyze"
#define ONLINE_OPTION           "online"
#define PROFILE_FUNCTION_OPTION "profile"

#ifdef __DEBUG_H__

#error "toto"

#else


#define __DEBUG_H__

namespace libta {

	extern bool isOptionSet(std::string option);
	extern void setOption(std::string option);
	extern bool isDebugOptionSet(std::string option);
	extern void setDebugOption(std::string option);

	extern int parseCommandLine(char * argv[]);

}

#ifndef DEBUG_OPTION
#define CURRENT_DEBUG_OPTION ""
#else
#define CURRENT_DEBUG_OPTION DEBUG_OPTION
#endif

#ifdef DEBUG

#define DOUT if( isDebugOptionSet(CURRENT_DEBUG_OPTION) ) std::cout 
#define DOUT_FCT if( isDebugOptionSet(CURRENT_DEBUG_OPTION) ) std::cout << __func__ << ": "
#define DOUT_CTOR if( isDebugOptionSet(CURRENT_DEBUG_OPTION) ) std::cout << __func__ << " ctor: " 
#define DOUT_DTOR if( isDebugOptionSet(CURRENT_DEBUG_OPTION) ) std::cout << __func__ << " dtor: " 
#define DOUT_NAME if( isDebugOptionSet(CURRENT_DEBUG_OPTION) ) std::cout << this->name() << ": " 

#else

#define DOUT if(0) std::cout
#define DOUT_FCT if(0) std::cout
#define DOUT_CTOR if(0) std::cout
#define DOUT_DTOR if(0) std::cout
#define DOUT_NAME if(0) std::cout

#endif

#endif				// __DEBUG_H__
