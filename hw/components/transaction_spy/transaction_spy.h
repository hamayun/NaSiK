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

#include <interconnect.h>
#include <mwsr_ta_fifo.h>
#include<slave_device.h>

class transaction_spy;
/*
 * INTERCONNECT_SLAVE_SPY
 */
class interconnect_slave_spy : public sc_module, public VCI_GET_REQ_IF, public VCI_PUT_RSP_IF
{
public:
    SC_HAS_PROCESS (interconnect_slave_spy);
	interconnect_slave_spy (sc_module_name name, transaction_spy *parent);
    ~interconnect_slave_spy ();

public:
    inline void add_request (vci_request &req)
    {
        m_queue_requests->Write (req);
    }

public:
    //get interface
    virtual void get (vci_request&);
    //put interface
    virtual void put (vci_response&);

private:
    //thread
    void dispatch_responses_thread ();

public:
    transaction_spy            *m_parent;
    mwsr_ta_fifo<vci_request>  *m_queue_requests;
    mwsr_ta_fifo<vci_response> *m_queue_responses;
};

/*
 * TRANSACTION_SPY
 */
class transaction_spy: public slave_device
{
public:
    SC_HAS_PROCESS(transaction_spy);
    transaction_spy (sc_module_name _name);
    virtual ~transaction_spy();

	void connect_slave_64 (sc_port<VCI_GET_REQ_IF> &getp, sc_port<VCI_PUT_RSP_IF> &putp);

	void rcv_rqst (unsigned long ofs, unsigned char be,	
                   unsigned char *data, bool bWrite);

private:
    interconnect_slave_spy              *m_slave_spy;
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
