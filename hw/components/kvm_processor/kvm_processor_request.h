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

#ifndef _QEMU_WRAPPER_REQUEST_
#define _QEMU_WRAPPER_REQUEST_

#include <systemc.h>

class kvm_processor_request
{
public:
    kvm_processor_request (unsigned id);

public:
    unsigned char			tid;
    unsigned char			ropcode;
    unsigned char			bDone;
    unsigned char           bWrite;
    sc_event				evDone;
    unsigned int			low_word;
    unsigned int			high_word;

    kvm_processor_request		*m_next;
};

class kvm_processor_requests
{
public:
    kvm_processor_requests (int count);
    ~kvm_processor_requests ();

    kvm_processor_request* GetNewRequest (int bWaitEmpty);
    kvm_processor_request* GetRequestByTid (unsigned char tid);
    void FreeRequest (kvm_processor_request *rq);
    void WaitWBEmpty ();

private:
    kvm_processor_request		*m_headFree;
    kvm_processor_request		*m_headBusy;
    sc_event                    m_evEmpty;
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
