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

#ifndef __TRANSACTION_SPY__
#define __TRANSACTION_SPY__

//#include <interconnect.h>
#include <abstract_noc.h>

/*
 * TRANSACTION_SPY
 */
#define ACCESS_NONE  0
#define ACCESS_READ  1
#define ACCESS_WRITE 2

#define SET_NOC_ACCESS(address,data,width,op) \
	_address = (uintptr_t)address; \
	_data = (uintptr_t)data; \
	_width = width; \
	_op = op; 

SC_MODULE(transaction_spy), public VCI_GET_REQ_IF, public VCI_PUT_RSP_IF
     //public VCI_PUT_REQ_IF, public VCI_GET_RSP_IF,
{
public:
    /* Master Side Ports */
    sc_port < VCI_GET_REQ_IF > get_port;
    sc_port < VCI_PUT_RSP_IF > put_port;

    /* Slave Side Ports */
    sc_export < VCI_GET_REQ_IF > get_req_port;
    sc_export < VCI_PUT_RSP_IF > put_rsp_port;

    SC_HAS_PROCESS(transaction_spy);
    transaction_spy (sc_module_name mod_name);
    virtual ~transaction_spy();

//    void request_transfer();
//    void response_transfer();
    
public:

//    void connect_master_side(sc_port<VCI_GET_REQ_IF> &getp, sc_port<VCI_PUT_RSP_IF> &putp);
    void connect_slave_side(sc_port<VCI_GET_REQ_IF> &slave_get_port, sc_port<VCI_PUT_RSP_IF> &slave_put_port);

    // get/put interfaces
    virtual void get (vci_request& req) 
	{
		get_port->get(req);
	}
    virtual void put (vci_response& rsp) {
		put_port->put(rsp);
	}

/*
    // get/put interfaces
    virtual void put (vci_request&);
    virtual void get (vci_response&);
*/

public:
    uintptr_t     _address;
    uintptr_t     _data;
    uint8_t       _width;
    uint8_t       _op; // none=0, read=1, write=2

    inline bool operator == (const transaction_spy& t) const
    {
      return (t._address == _address && t._data == _data && t._width == _width && t._op == _op);
    }

	inline uint8_t be_width(uint8_t be)
	{
		uint8_t  width = 0;
	    switch (be)
    	{
        //byte access
        case 0x01:	case 0x02:	case 0x04:	case 0x08:
        case 0x10:  case 0x20:  case 0x40:  case 0x80:
			width = 1; break;
        //word access
        case 0x03:  case 0x0C:  case 0x30:  case 0xC0:
			width = 2; break;
        //dword access
        case 0x0F:  case 0xF0:
			width = 4; break;
        //qword access
        case 0xFF:
			width = 8; break;
		}
		return width;
	}
};

#endif

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
