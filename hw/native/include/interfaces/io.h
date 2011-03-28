#ifndef __INTERFACES_IO_H__
#define __INTERFACES_IO_H__

#include "systemc.h"
#include "mapping.h"

#ifdef NOT_IMPLEMENTED
#undef NOT_IMPLEMENTED
#endif

#define NOT_IMPLEMENTED cerr << "ERROR: COM service " << __func__ << " not implemented" << endl;

namespace native {

struct IO:public sc_interface
{
	virtual void read (uint8_t  *addr, uint8_t  *data) { NOT_IMPLEMENTED; }
	virtual void read (uint16_t *addr, uint16_t *data) { NOT_IMPLEMENTED; }
	virtual void read (uint32_t *addr, uint32_t *data) { NOT_IMPLEMENTED; }
	virtual void read (uint64_t *addr, uint64_t *data) { NOT_IMPLEMENTED; }

	virtual void write (uint8_t  *addr, uint8_t  data) { NOT_IMPLEMENTED; }
	virtual void write (uint16_t *addr, uint16_t data) { NOT_IMPLEMENTED; }
	virtual void write (uint32_t *addr, uint32_t data) { NOT_IMPLEMENTED; }
	virtual void write (uint64_t *addr, uint64_t data) { NOT_IMPLEMENTED; }

	virtual void vector_write (uint8_t  *to, uint8_t  *from, unsigned long int len) { NOT_IMPLEMENTED; }
	virtual void vector_write (uint16_t *to, uint16_t *from, unsigned long int len) { NOT_IMPLEMENTED; }
	virtual void vector_write (uint32_t *to, uint32_t *from, unsigned long int len) { NOT_IMPLEMENTED; }
	virtual void vector_write (uint64_t *to, uint64_t *from, unsigned long int len) { NOT_IMPLEMENTED; }

	virtual void vector_write (double *to, double *from, unsigned long int len) { NOT_IMPLEMENTED; }
	virtual void vector_write (float *to, float *from, unsigned long int len)   { NOT_IMPLEMENTED; }

	virtual uint8_t  load_linked (uint8_t  *addr, uint32_t id) { NOT_IMPLEMENTED; return(-1);}
	virtual uint16_t load_linked (uint16_t *addr, uint32_t id) { NOT_IMPLEMENTED; return(-1);}
	virtual uint32_t load_linked (uint32_t *addr, uint32_t id) { NOT_IMPLEMENTED; return(-1);}
	virtual uint64_t load_linked (uint64_t *addr, uint32_t id) { NOT_IMPLEMENTED; return(-1);}

	virtual bool store_cond (uint8_t  *addr, uint8_t  data, uint32_t id) { NOT_IMPLEMENTED; return(false);}
	virtual bool store_cond (uint16_t *addr, uint16_t data, uint32_t id) { NOT_IMPLEMENTED; return(false);}
	virtual bool store_cond (uint32_t *addr, uint32_t data, uint32_t id) { NOT_IMPLEMENTED; return(false);}
	virtual bool store_cond (uint64_t *addr, uint64_t data, uint32_t id) { NOT_IMPLEMENTED; return(false);}

	virtual void grant(unsigned int addr, unsigned int nb_access) { NOT_IMPLEMENTED; }

	virtual std::vector< mapping::segment_t * > * get_mapping() { NOT_IMPLEMENTED; return(NULL); }

};

#undef NOT_IMPLEMENTED

} // end namespace native

#endif

