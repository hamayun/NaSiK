/*************************************************************************************
 * File   : devices.h,     
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

#ifndef __DEVICES_H__
#define __DEVICES_H__

#include "systemc.h"
#include "interfaces/io.h"
#include "interfaces/link.h"
#include <vector>
#include "symbol.h"

#define WAIT_LATENCY(l) \
	if(l > 0) \
  { \
    DOUT_NAME << __func__ << " wait " << l << std::endl; \
    wait(_clock_period->value * l,(sc_time_unit)_clock_unit->value); \
  }

#define REL_ADDR(addr) \
  ((uintptr_t)(addr) - _registers.ptr)

#define REGISTER_INDEX(type,addr) REGISTER_INDEX_##type(addr)
#define REGISTER_INDEX_UINT8(addr)    (uint32_t)REL_ADDR(addr)
#define REGISTER_INDEX_UINT16(addr)   (uint32_t)(REL_ADDR(addr)>>1)
#define REGISTER_INDEX_UINT32(addr)   (uint32_t)(REL_ADDR(addr)>>2)
#define REGISTER_INDEX_UINT64(addr)   (uint64_t)(REL_ADDR(addr)>>4)

#define ACCESS_NONE  0
#define ACCESS_READ  1                                                                                                                                               
#define ACCESS_WRITE 2
#define SET_BUS_ACCESS(address,data,width,op) \
  _address = (uintptr_t)address; \
_data = (uintptr_t)data; \
_width = width; \
_op = op; 

namespace native {

SC_MODULE(dev_channel),
  public IO
{
	public:
    sc_export < IO > exp_io;
    sc_port < IO > p_io;

    dev_channel(sc_module_name name);
    ~dev_channel();

    void read (uint8_t  *addr, uint8_t  *data);
    void read (uint16_t *addr, uint16_t *data);
    void read (uint32_t *addr, uint32_t *data);
    void read (uint64_t *addr, uint64_t *data);

    void write (uint8_t  *addr, uint8_t  data);
    void write (uint16_t *addr, uint16_t data);
    void write (uint32_t *addr, uint32_t data);
    void write (uint64_t *addr, uint64_t data);

	  uint32_t load_linked (uint32_t *addr, uint32_t id);
	  bool store_cond (uint32_t *addr, uint32_t data, uint32_t id);

    std::vector< mapping::segment_t * > * get_mapping();

    uintptr_t     _address;
    uintptr_t     _data;
    uint8_t       _width;
    uint8_t       _op; // none=0, read=1, write=2

    inline bool operator == (const dev_channel& c) const
    {
      return (c._address == _address && c._data == _data && c._width == _width && c._op == _op);
    }
};

SC_MODULE(device)
{
  public:
    virtual void end_of_elaboration();

    device(sc_module_name name);
    ~device();

  protected:
    sc_attribute < uint32_t >    *_clock_period;
    sc_attribute < uint32_t >    *_clock_unit;

  public:
    std::string                   _generic_name;

};

class device_slave: 
  public device,
  public IO,
  public LINKER
{
  public:
    sc_export < IO > exp_io;
    sc_export < LINKER >  exp_linker;

    typedef union {
      uint8_t      *reg8;
      uint16_t     *reg16;
      uint32_t     *reg32;
      uint64_t     *reg64;
      uintptr_t    ptr;
    } register_addr_t;

    void read (uint8_t  *addr, uint8_t  *data);
    void read (uint16_t *addr, uint16_t *data);
    void read (uint32_t *addr, uint32_t *data);
    void read (uint64_t *addr, uint64_t *data);

    void write (uint8_t  *addr, uint8_t  data);
    void write (uint16_t *addr, uint16_t data);
    void write (uint32_t *addr, uint32_t data);
    void write (uint64_t *addr, uint64_t data);

    virtual void slv_read (uint8_t  *addr, uint8_t  *data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_read (uint16_t *addr, uint16_t *data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_read (uint32_t *addr, uint32_t *data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_read (uint64_t *addr, uint64_t *data) { ASSERT_MSG(false, "NOT Implemented"); }

    virtual void slv_write (uint8_t  *addr, uint8_t  data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_write (uint16_t *addr, uint16_t data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_write (uint32_t *addr, uint32_t data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_write (uint64_t *addr, uint64_t data) { ASSERT_MSG(false, "NOT Implemented"); }

    device_slave(sc_module_name name);
    ~device_slave();
    virtual void end_of_elaboration();

    virtual std::vector< mapping::segment_t * > * get_mapping();
    virtual std::vector< symbol_base* > * get_symbols();

  protected:
    std::vector< mapping::segment_t *> 		_segments;
    std::vector< symbol_base *> 		      _symbols;

    register_addr_t                       _registers;

  protected:
    sc_attribute < uint32_t >    *_req_latency;
    sc_attribute < uint32_t >    *_rsp_latency;

};

class device_master:
  public device
{
  public:
    sc_port < IO,256 >  p_io;

    device_master(sc_module_name name);
    ~device_master();
    virtual void end_of_elaboration();

    virtual void mst_read (uint8_t  *addr, uint8_t  *data, uint8_t index = 0);
    virtual void mst_read (uint16_t *addr, uint16_t *data, uint8_t index = 0);
    virtual void mst_read (uint32_t *addr, uint32_t *data, uint8_t index = 0);
    virtual void mst_read (uint64_t *addr, uint64_t *data, uint8_t index = 0);

    virtual void mst_write (uint8_t  *addr, uint8_t  data, uint8_t index = 0);
    virtual void mst_write (uint16_t *addr, uint16_t data, uint8_t index = 0);
    virtual void mst_write (uint32_t *addr, uint32_t data, uint8_t index = 0);
    virtual void mst_write (uint64_t *addr, uint64_t data, uint8_t index = 0);

  protected:
    sc_attribute < uint32_t >    *_req_latency;
    sc_attribute < uint32_t >    *_rsp_latency;

};

class device_master_slave: 
  public device,
  public IO,
  public LINKER
{
  public:
    sc_export < IO > exp_io;
    sc_export < LINKER >  exp_linker;

    sc_port < IO,256 >  p_io;

    typedef union {
      uint8_t      *reg8;
      uint16_t     *reg16;
      uint32_t     *reg32;
      uint64_t     *reg64;
      uintptr_t    ptr;
    } register_addr_t;

    void read (uint8_t  *addr, uint8_t  *data);
    void read (uint16_t *addr, uint16_t *data);
    void read (uint32_t *addr, uint32_t *data);
    void read (uint64_t *addr, uint64_t *data);

    void write (uint8_t  *addr, uint8_t  data);
    void write (uint16_t *addr, uint16_t data);
    void write (uint32_t *addr, uint32_t data);
    void write (uint64_t *addr, uint64_t data);

    virtual void slv_read (uint8_t  *addr, uint8_t  *data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_read (uint16_t *addr, uint16_t *data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_read (uint32_t *addr, uint32_t *data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_read (uint64_t *addr, uint64_t *data) { ASSERT_MSG(false, "NOT Implemented"); }

    virtual void slv_write (uint8_t  *addr, uint8_t  data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_write (uint16_t *addr, uint16_t data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_write (uint32_t *addr, uint32_t data) { ASSERT_MSG(false, "NOT Implemented"); }
    virtual void slv_write (uint64_t *addr, uint64_t data) { ASSERT_MSG(false, "NOT Implemented"); }

    virtual void mst_read (uint8_t  *addr, uint8_t  *data, uint8_t index = 0);
    virtual void mst_read (uint16_t *addr, uint16_t *data, uint8_t index = 0);
    virtual void mst_read (uint32_t *addr, uint32_t *data, uint8_t index = 0);
    virtual void mst_read (uint64_t *addr, uint64_t *data, uint8_t index = 0);

    virtual void mst_write (uint8_t  *addr, uint8_t  data, uint8_t index = 0);
    virtual void mst_write (uint16_t *addr, uint16_t data, uint8_t index = 0);
    virtual void mst_write (uint32_t *addr, uint32_t data, uint8_t index = 0);
    virtual void mst_write (uint64_t *addr, uint64_t data, uint8_t index = 0);

    device_master_slave(sc_module_name name);
    ~device_master_slave();
    virtual void end_of_elaboration();

    virtual std::vector< mapping::segment_t * > * get_mapping();
    virtual std::vector< symbol_base* > * get_symbols();

  protected:
    std::vector< mapping::segment_t *> 		_segments;
    std::vector< symbol_base *> 		      _symbols;

    register_addr_t                       _registers;

  protected:
    sc_attribute < uint32_t >    *_mst_req_latency;
    sc_attribute < uint32_t >    *_mst_rsp_latency;
    sc_attribute < uint32_t >    *_slv_req_latency;
    sc_attribute < uint32_t >    *_slv_rsp_latency;

};
inline void sc_trace(sc_trace_file *tf, const dev_channel &dev, std::ofstream *vcd_conf)
{
  ::sc_trace(tf, dev._address, (std::string)(dev.name()) + ".address");
  ::sc_trace(tf, dev._data, (std::string)(dev.name()) + ".data");
  ::sc_trace(tf, dev._width, (std::string)(dev.name()) + ".width");
  ::sc_trace(tf, dev._op, (std::string)(dev.name()) + ".op");

  if(vcd_conf != NULL)
    *vcd_conf << "@200\n-" << dev.name() << ":\n";
  if(vcd_conf != NULL)
  {
    *vcd_conf << "@20\n+address SystemC.\\" << dev.name() << ".address" << "[" << sizeof(uintptr_t) * 8 - 1 << ":0]\n";
    *vcd_conf << "@20\n+data SystemC.\\" << dev.name() << ".data" << "[" << sizeof(uintptr_t) * 8 - 1 << ":0]\n";
    *vcd_conf << "@2024\n^1 filter_op\n+op SystemC.\\" << dev.name() << ".op" << "[7:0]\n";
    *vcd_conf << "@2024\n^1 filter_width\n+width SystemC.\\" << dev.name() << ".width" << "[7:0]\n";
  }
}


//inline void sc_trace(sc_trace_file *tf, const device &dev,std::ofstream *vcd_conf)
//{
//  if(vcd_conf != NULL)
//    *vcd_conf << "@200\n-" << dev._generic_name << ": " << dev.name() << "\n";
//}

}

#endif				// __DEVICE_BASE_H__
