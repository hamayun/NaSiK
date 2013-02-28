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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <transaction_spy.h>
#include <errno.h>

//#define DEBUG_TRANSACTION_SPY

#ifdef DEBUG_TRANSACTION_SPY
#define DPRINTF(fmt, args...)                               \
    do { printf("%s: " fmt, name(), ##args); } while (0)
#else
#define DPRINTF(fmt, args...) do {} while(0)
#endif

/*
 * INTERCONNECT_SLAVE_SPY
 */
interconnect_slave_spy::interconnect_slave_spy (sc_module_name name, transaction_spy *parent)
: sc_module (name)
{
    m_parent = parent;

    m_queue_requests = new mwsr_ta_fifo<vci_request> (8);
    m_queue_responses = new mwsr_ta_fifo<vci_response> (8);

    SC_THREAD (dispatch_responses_thread);
}

interconnect_slave_spy::~interconnect_slave_spy ()
{
    printf("%s: Destructor Called\n", __func__);

    if (m_queue_requests)
        delete m_queue_requests;

    if (m_queue_responses)
        delete m_queue_responses;
}

//put interface
void interconnect_slave_spy::put (vci_response &rsp)
{
    m_queue_responses->Write (rsp);
}

//get interface
void interconnect_slave_spy::get (vci_request &req)
{
    req = m_queue_requests->Read ();
}

void interconnect_slave_spy::dispatch_responses_thread ()
{
    vci_response            rsp;

    while (1)
    {
        rsp = m_queue_responses->Read ();
		/* Do the Spying Work Here; Optional */
    	m_parent->send_rsp(rsp.rerror);
    }
}

/*
 * TRANSACTION_SPY
 */
transaction_spy::transaction_spy (sc_module_name name)
:slave_device(name)
{
	cout << __func__ << ": Creating Spy Interconnect Interface" << endl;
	m_slave_spy = new interconnect_slave_spy("DUMMY", this);
	if(!m_slave_spy)
	{
		cerr << __func__ << ": Error Creating the Spy Interconnect Interface Instance" << endl;
		exit(-1);
	}
}

transaction_spy::~transaction_spy ()
{
    printf("%s: Destructor Called\n", __func__);
}

void transaction_spy::connect_slave_64 (sc_port<VCI_GET_REQ_IF> &getp, sc_port<VCI_PUT_RSP_IF> &putp)
{
    printf("transaction_spy@connect_slave_64: Connecting Slave\n");
    getp (*m_slave_spy);
	putp(*m_slave_spy);
}

void transaction_spy::rcv_rqst (unsigned long ofs, unsigned char be,
                                unsigned char *data, bool bWrite)
{
	// cout << "Runing the " << __func__ << endl;
	/* Do the Spying Work Here */
	m_slave_spy->add_request(m_req);
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
