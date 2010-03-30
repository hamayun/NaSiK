#ifndef __MAPPING_H__
#define __MAPPING_H__

#include "systemc.h"
#include <vector>

namespace libta
{

#define FLAG_WRITE	     (1 << 0)	/* Writable */
#define	FLAG_ALLOC	     (1 << 1)	/* Occupies memory during execution */
#define	FLAG_EXECINSTR	     (1 << 2)	/* Executable */
#define	FLAG_MERGE	     (1 << 4)	/* Might be merged */
#define	FLAG_STRINGS	     (1 << 5)	/* Contains nul-terminated strings */
#define	FLAG_INFO_LINK	     (1 << 6)	/* `sh_info' contains SHT index */
#define	FLAG_LINK_ORDER	     (1 << 7)	/* Preserve order after combining */
#define	FLAG_OS_NONCONFORMING (1 << 8)	/* Non-standard OS specific handling */

	namespace mapping
	{

		typedef struct 
		{
			const char * name;
			uintptr_t       base_addr;
			uintptr_t       end_addr;
			uint32_t  	     size;
			uint8_t	     flags;
		} segment_t; 

		segment_t *init(const char * str, uint32_t sz);
		segment_t *init(const char * str, uintptr_t base, uint32_t sz);
		bool compare(segment_t *s1, segment_t *s2);

	}
}

#endif

