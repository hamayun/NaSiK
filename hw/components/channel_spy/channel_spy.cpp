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
    do { printf("[CHANNEL_SPY]: " fmt, ##args); } while (0)
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
    DPRINTF("Destructor Called\n");
}

/*
 * CHANNEL_SPY_MASTER
 */
channel_spy_master::channel_spy_master (sc_module_name mod_name)
:channel_spy(mod_name)
{
    master_put_req_exp(*this);
    master_get_rsp_exp(*this);
}

channel_spy_master::~channel_spy_master ()
{
    DPRINTF("Destructor Called by Master (%s)\n", name());
}

void channel_spy_master::connect_master(int device_id,
                                        sc_port<VCI_PUT_REQ_IF> &master_put_port,
                                        sc_port<VCI_GET_RSP_IF> &master_get_port)
{
    DPRINTF("Connecting Master ID %2d (%s)\n", device_id, name());
    master_put_port(master_put_req_exp);
    master_get_port(master_get_rsp_exp);
}

// put/get interface implementations for master_put_req_exp and master_get_rsp_exp
void channel_spy_master::put (vci_request& req) 
{
	char * data = (char *) req.wdata;

//	data = (char *) req.wdata;
//	SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_NONE);

	// Forward call to the actual slave (NOC)
	put_req_port->put(req);

  	/* Do the Spying Work Here */
	if(req.cmd == CMD_READ){
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_READ);
	}
	else{
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_WRITE);
	}

	last_req_addr = req.address;
}

void channel_spy_master::get (vci_response& rsp) 
{
	char * data = (char *) rsp.rdata;

   	/* Do the Spying Work Here */
	SET_NOC_ACCESS(last_req_addr,*data,be_width(rsp.rbe),ACCESS_NONE);

	// Forward call to the actual slave (NOC)
	get_rsp_port->get(rsp);
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
    DPRINTF("Destructor Called by Slave (%s)\n", name());
}

void channel_spy_slave::connect_slave(int device_id,
                                      sc_port<VCI_GET_REQ_IF> &slave_get_port,
                                      sc_port<VCI_PUT_RSP_IF> &slave_put_port)
{
    DPRINTF("Connecting Slave ID %2d (%s)\n", device_id, name());
    slave_get_port(slave_get_req_exp);
    slave_put_port(slave_put_rsp_exp);
}

// get/put interface implementations for slave_get_req_exp and slave_put_rsp_exp
void channel_spy_slave::get (vci_request& req) 
{
	char * data = (char *) req.wdata;

  	/* Do the Spying Work Here */
	// 	data = (char *) req.wdata;
	//	SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_NONE);

	// Forward call to the actual master (NOC)
	get_req_port->get(req);

	if(req.cmd == CMD_READ){
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_READ);
	}
	else{
		SET_NOC_ACCESS(req.address,*data,be_width(req.be),ACCESS_WRITE);
	}
	
	last_req_addr = req.address;
}

void channel_spy_slave::put (vci_response& rsp) 
{
	char * data = (char *) rsp.rdata;

   	/* Do the Spying Work Here */
	SET_NOC_ACCESS(last_req_addr,*data,be_width(rsp.rbe),ACCESS_NONE);

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
