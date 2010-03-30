/*************************************************************************************
 * File   : debug.cpp,     
 *
 * Copyright (C) 2008 TIMA Laboratory
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

#include "debug.h"
#include <map>
#include <iostream>
#include <string>

namespace libta
{

	static std::map< std::string , bool > debug_option_map;
	static bool debug_all = false;
	static std::map< std::string , bool > option_map;

	bool isOptionSet(std::string option)
	{
		std::map< std::string , bool >::iterator option_map_it;
		option_map_it = option_map.find(option);

		if(option_map_it != option_map.end())
		{
			return(true);
		}
		return(false);

	}

	bool isDebugOptionSet(std::string option)
	{
		std::map< std::string , bool >::iterator debug_option_map_it;
		debug_option_map_it = debug_option_map.find(option);

		if(debug_option_map_it != debug_option_map.end())
		{
			return(true);
		}
		return(debug_all);

	}

	void setDebugOption(std::string option)
	{
		if(option == "all")
			debug_all = true;
		else
			debug_option_map[option] = true;

              std::cout << "Set Debug Option " << option << std::endl;
	}

	void setOption(std::string option)
	{
            option_map[option] = true;
            std::cout << "Set Option " << option << std::endl;
	}

#define DELIM           " \t-="
#define LIBTA_TOKEN     "libta"
#define LIBTA_HELP      "help"
#define LIBTA_DEBUG     "debug"
#define LIBTA_OPTION    "option"

#define STRCMP_TOKEN(s1,s2)    strncmp(s1,s2,strlen(s2))

	static unsigned int argv_index = 0;

	static inline char * next_token(char *argv[], const char * delim)
	{
		char * token;

		token = strtok( NULL, delim);
		while(token == NULL)
		{
			argv_index++;
			if(argv[argv_index] == NULL)
				return(NULL);

			token = strtok( argv[argv_index] , delim);
		}
		return(token);
	}

	int parseCommandLine(char *argv[])
	{
		unsigned int index;
		char *token;
		char *module;

		argv_index = 0;
		index = 0;

		token = strtok(argv[0],DELIM);
		while(token != NULL)
		{

			if(STRCMP_TOKEN(token,LIBTA_TOKEN) == 0)
			{
				token = next_token( argv, DELIM);
				if(STRCMP_TOKEN(token,LIBTA_HELP) == 0)
				{
				}
				if(STRCMP_TOKEN(token,LIBTA_DEBUG) == 0)
				{
					module = next_token( argv, " \t=");
					while(module != NULL)
					{	
						setDebugOption(module);
						module = strtok( NULL, " \t");
					}
				}
				if(STRCMP_TOKEN(token,LIBTA_OPTION) == 0)
				{
					module = next_token( argv, " \t=");
					while(module != NULL)
					{	
						setOption(module);
						module = strtok( NULL, " \t");
					}
				}
				token = next_token( argv, DELIM);
			}
			else
			{
				// Not a libta command
				argv[index++] = argv[argv_index];
				token = next_token( argv, DELIM);
			}
		}
		argv[index] = 0;
		return(index);
	}
}
