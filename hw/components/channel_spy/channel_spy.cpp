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
#include <channel_spy.h>
#define DEBUG_CHANNEL_SPY

#ifdef DEBUG_CHANNEL_SPY
#define DPRINTF(fmt, args...)                               \
    do { printf("%s: " fmt, name(), ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

/*
 * CHANNEL_SPY
 */
channel_spy::channel_spy (sc_module_name mod_name)
:sc_module(mod_name),
_address(0), _data(0), _width(32), _op(ACCESS_NONE)
{}

channel_spy::~channel_spy ()
{
    DPRINTF("%s: Destructor Called\n", __func__);
}

/*
 * CHANNEL_SPY_SLAVE
 */
channel_spy_slave::channel_spy_slave (sc_module_name mod_name)
:channel_spy(mod_name)
{
    slave_get_req_exp(*this);
    slave_put_rsp_exp(*this);
}

channel_spy_slave::~channel_spy_slave ()
{
    DPRINTF("%s: Destructor Called\n", __func__);
}


void channel_spy_slave::connect_slave(int device_id,
                                      sc_port<VCI_GET_REQ_IF> &slave_get_port,
                                      sc_port<VCI_PUT_RSP_IF> &slave_put_port)
{
    DPRINTF("Connecting Slave Device %d\n", device_id);
    slave_get_port(slave_get_req_exp);
    slave_put_port(slave_put_rsp_exp);
}

// get/put interface implementations for slave_get_req_exp and slave_put_rsp_exp
void channel_spy_slave::get (vci_request& req) 
{
	char * data = (char *) req.wdata;

   	/* Do the Spying Work Here */
	if(req.cmd == CMD_READ){
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_READ);
	}
	else{
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_WRITE);
	}

	// Forward call to the actual master (NOC)
	get_req_port->get(req);

	data = (char *) req.wdata;
	SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_NONE);
}

void channel_spy_slave::put (vci_response& rsp) 
{
   	/* Do the Spying Work Here */
	// Forward call to the actual master (NOC)
	put_rsp_port->put(rsp);
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
