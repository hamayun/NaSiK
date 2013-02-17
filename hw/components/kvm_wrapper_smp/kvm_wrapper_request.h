/*
 *  Copyright (c) 2013 TIMA Laboratory
 *
 *  This file is part of NaSiK and inherits most of its features from 
 *  Rabbits Framework.
 *
 *  NaSiK is a free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  NaSiK is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with NaSiK.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _KVM_WRAPPER_REQUEST_
#define _KVM_WRAPPER_REQUEST_

#include <systemc.h>

class kvm_wrapper_request
{
public:
    kvm_wrapper_request (unsigned id);

public:
    unsigned char			tid;
    unsigned char			ropcode;
    unsigned char			bDone;
    unsigned char           bWrite;
    sc_event				evDone;
    unsigned int			low_word;
    unsigned int			high_word;

    kvm_wrapper_request		*m_next;
};

class kvm_wrapper_requests
{
public:
    kvm_wrapper_requests (int count);
    ~kvm_wrapper_requests ();

    kvm_wrapper_request* GetNewRequest (int bWaitEmpty);
    kvm_wrapper_request* GetRequestByTid (unsigned char tid);
    void FreeRequest (kvm_wrapper_request *rq);
    void WaitWBEmpty ();

private:
    kvm_wrapper_request		*m_headFree;
    kvm_wrapper_request		*m_headBusy;
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
