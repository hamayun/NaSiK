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

#include <kvm_wrapper_request.h>

using namespace std;

// class kvm_wrapper_request
kvm_wrapper_request::kvm_wrapper_request (unsigned id)
{
    tid = id;
    bDone = false;
    m_next = 0;
}


// class kvm_wrapper_requests
kvm_wrapper_requests::kvm_wrapper_requests (int count)
{
    m_headFree = NULL;
    m_headBusy = NULL;

    kvm_wrapper_request		**pp = &m_headFree;
    for (int i = 0; i < count; i++)
    {
        *pp = new kvm_wrapper_request (i + 1);
        pp = &(*pp)->m_next;
    }
}

kvm_wrapper_requests::~kvm_wrapper_requests ()
{
    kvm_wrapper_request		*p;

    while (m_headFree)
    {
        p = m_headFree;
        m_headFree = m_headFree->m_next;
        delete p;
    }
}

void kvm_wrapper_requests::WaitWBEmpty ()
{
    while (m_headBusy)
        wait (m_evEmpty);
}

kvm_wrapper_request* kvm_wrapper_requests::GetNewRequest (int bWaitEmpty)
{
    while (bWaitEmpty && m_headBusy)
        wait (m_evEmpty);

    kvm_wrapper_request	*p = m_headFree;

    if (!p)
    {
        cerr << "[Error: no request available for kvm_wrapper_requests::GetNewRequest.]" << endl;
        exit (1);
    }

    m_headFree = m_headFree->m_next;
    p->m_next = m_headBusy;
    m_headBusy = p;

    p->bDone = 0;
    p->low_word = 0xDEADDEAD;
    p->high_word = 0xDEADDEAD;

    return p;
}

kvm_wrapper_request* kvm_wrapper_requests::GetRequestByTid (unsigned char tid)
{
    kvm_wrapper_request	*p = m_headBusy;

    while (p && p->tid != tid)
        p = p->m_next;

    return p;
}

void kvm_wrapper_requests::FreeRequest (kvm_wrapper_request *rq)
{
    kvm_wrapper_request	**pp = &m_headBusy;

    while (*pp && *pp != rq)
        pp = &(*pp)->m_next;
    if (!*pp)
    {
        cerr << "[Error: Cannot find the request kvm_wrapper_request::FreeRequest]" << endl;
        return;
    }

    *pp = rq->m_next;
    rq->m_next = m_headFree;
    m_headFree = rq;

    if (!m_headBusy)
        m_evEmpty.notify (0, SC_NS);
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