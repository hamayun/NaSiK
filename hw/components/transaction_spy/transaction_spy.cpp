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

// get/put interfaces
void transaction_spy::get (vci_request& req) 
{
	char * data = (char *) req.wdata;

   	/* Do the Spying Work Here */
	if(req.cmd == CMD_READ){
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_READ);
	}
	else{
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_WRITE);
	}

	get_port->get(req);

	data = (char *) req.wdata;
	SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_NONE);
}

void transaction_spy::put (vci_response& rsp) 
{
   	/* Do the Spying Work Here */
	put_port->put(rsp);
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
