/*
 *  Copyright (c) 2010 TIMA Laboratory
 *
 *  This file is part of Rabbits.
 *
 *  Rabbits is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Rabbits is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Rabbits.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <systemc.h>
#include <transaction_spy.h>
//#define DEBUG_TRANSACTION_SPY

#ifdef DEBUG_TRANSACTION_SPY
#define DPRINTF(fmt, args...)                               \
    do { printf("%s: " fmt, name(), ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

/*
 * TRANSACTION_SPY
 */
transaction_spy::transaction_spy (sc_module_name mod_name)
:sc_module(mod_name),
_address(0), _data(0), _width(32), _op(ACCESS_NONE)
{
	get_req_port(*this);
	put_rsp_port(*this);
}

transaction_spy::~transaction_spy ()
{
    printf("%s: Destructor Called\n", __func__);
}

void transaction_spy::connect_slave_side(sc_port<VCI_GET_REQ_IF> &slave_get_port,
                                         sc_port<VCI_PUT_RSP_IF> &slave_put_port)
{
    printf("transaction_spy: Connecting Slave Side\n");

    slave_get_port(get_req_port);
	slave_put_port(put_rsp_port);
}

inline void sc_trace(sc_trace_file *tf, const transaction_spy &tspy, std::ofstream *vcd_conf)
{
//  ::sc_trace(tf, tspy._address, (std::string)(tspy.name()) + ".address");
//  ::sc_trace(tf, tspy._data, (std::string)(tspy.name()) + ".data");
//  ::sc_trace(tf, tspy._width, (std::string)(tspy.name()) + ".width");
//  ::sc_trace(tf, tspy._op, (std::string)(tspy.name()) + ".op");
 
/* 
  if(vcd_conf != NULL)
    *vcd_conf << "@200\n-" << tspy.name() << ":\n";
  if(vcd_conf != NULL)
  {
    *vcd_conf << "@20\n+address SystemC.\\" << tspy.name() << ".address" << "[" << sizeof(uintptr_t) * 8 - 1 << ":0]\n";
    *vcd_conf << "@20\n+data SystemC.\\" << tspy.name() << ".data" << "[" << sizeof(uintptr_t) * 8 - 1 << ":0]\n";    
    *vcd_conf << "@2024\n^1 filter_op\n+op SystemC.\\" << tspy.name() << ".op" << "[7:0]\n";                          
    *vcd_conf << "@2024\n^1 filter_width\n+width SystemC.\\" << tspy.name() << ".width" << "[7:0]\n";                 
  }
*/
}

/*
 * Vim standard variables
 * vim:set ts=4 expandtab tw=80 cindent syntax=c:
 *
 * Emacs standard variables
 * Local Variables:
 * mode: c
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 */
