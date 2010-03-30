#ifndef __INTERFACES_INTERRUPT_H__
#define __INTERFACES_INTERRUPT_H__

#include "systemc.h"
#include "stdint.h"


#ifdef NOT_IMPLEMENTED
#undef NOT_IMPLEMENTED
#endif

#define NOT_IMPLEMENTED cerr << "ERROR: " << __func__ << " not implemented" << endl;

namespace libta {

  struct INTERRUPT:public sc_interface
  {
    virtual void interrupt_request(uint32_t id) { NOT_IMPLEMENTED; return(NULL); }
  };

}

#undef NOT_IMPLEMENTED

#endif

