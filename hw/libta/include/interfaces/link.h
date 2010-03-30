#ifndef __INTERFACES_LINK_H__
#define __INTERFACES_LINK_H__

#include "systemc.h"
#include <vector>
#include "symbol.h"
#include "mapping.h"
#include <dlfcn.h>
#include <link.h>
#include <limits.h>


#ifdef NOT_IMPLEMENTED
#undef NOT_IMPLEMENTED
#endif

#define NOT_IMPLEMENTED cerr << "ERROR: " << __func__ << " not implemented" << endl;

namespace libta {

	struct LINKER:public sc_interface
	{
		struct LOADER:public sc_interface
		{
//			virtual void* load(const char *) = 0;
			virtual uintptr_t get_start_addr() = 0;
			virtual link_map* get_link_map() = 0;
			virtual mapping::segment_t * get_section(const char * section_name) = 0; 
		};

		virtual std::vector< symbol_base* > * get_symbols() { NOT_IMPLEMENTED; return(NULL); }
	};

}

#undef NOT_IMPLEMENTED

#endif

