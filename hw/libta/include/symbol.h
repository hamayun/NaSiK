/*************************************************************************************
 * File   : symbol.h,     
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

#ifndef __SYMBOL_H__
#define __SYMBOL_H__

#include <string>
#include <vector>
#include "assertion.h"

namespace libta {

class symbol_base
{
	public:
		symbol_base(std::string name):_name(name){};
		~symbol_base(){};
		virtual bool link(void * sw_image) = 0;
		virtual uint32_t size() = 0;

	protected:
		std::string _name;
};

template <class T>
class symbol : public symbol_base
{
	public:
		symbol(std::string name) : symbol_base(name) {}
		~symbol() {}

		void push_back(T value) { _values.push_back(value); }
		std::vector<T> * get_values() {return(&_values);} 

		bool link(void * sw_image) { ASSERT_MSG(false, "link method must be specialized"); return(false); }
		uint32_t size() { return(_values.size()); }

	private:
		std::vector<T> _values;

};

}
#endif				// __LD_SYMBOL_H__

