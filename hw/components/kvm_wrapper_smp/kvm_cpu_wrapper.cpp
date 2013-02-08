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

#include <kvm_cpu_wrapper.h>
#include <kvm_imported.h>
#include <kvm_wrapper_consts.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include <master_device.h>

//#define DEBUG_KVM_CPU_WRAPPER
#ifdef DEBUG_KVM_CPU_WRAPPER
#define PRINTF printf
#else
#define PRINTF if (0) printf
#endif

static struct timeval                   start_time;

kvm_cpu_wrapper::kvm_cpu_wrapper (sc_module_name name, void *kvm_instance, unsigned int node_id,
								  int cpuindex, kvm_import_t *kvm_import)
: master_device (name)
{
    m_kvm_import = kvm_import;
    m_kvm_instance = kvm_instance;
    m_node_id = node_id;
    m_cpuindex = cpuindex;
    m_rqs = new kvm_wrapper_requests (100);

    m_unblocking_write = 0;
    m_swi = 0;

    if (cpuindex == 0)
        gettimeofday (&start_time, NULL);
}

kvm_cpu_wrapper::~kvm_cpu_wrapper ()
{
    if (m_rqs)
        delete m_rqs;
}

void kvm_cpu_wrapper::set_unblocking_write (bool val)
{
    m_unblocking_write = val;
}

void kvm_cpu_wrapper::rcv_rsp(unsigned char tid, unsigned char *data, bool bErr, bool bWrite)
{

    kvm_wrapper_request            *localrq;

    localrq = m_rqs->GetRequestByTid (tid);
    if (localrq == NULL)
    {
		PRINTF("kvm_cpu_wrapper[%d] Error: %s received a response for an unknown TID = %d\n",
				m_cpuindex, name(), (unsigned int) tid);
        return;
    }

    if (m_unblocking_write && localrq->bWrite) //response for a write cmd
    {
        m_rqs->FreeRequest (localrq);
        return;
    }

    localrq->low_word = *((unsigned int *)data + 0);
    localrq->high_word = *((unsigned int *)data + 1);
    localrq->bDone = 1;
    localrq->evDone.notify ();

    return;
}

uint64_t kvm_cpu_wrapper::read (uint64_t addr, int nbytes, int bIO)
{
    unsigned char           adata[8];
    uint64_t                ret;
    int                     i;
    unsigned char           tid;
    kvm_wrapper_request   *localrq;

    if (m_unblocking_write)
        localrq = m_rqs->GetNewRequest (1);
    else
        localrq = m_rqs->GetNewRequest (0);

    if (localrq == NULL)
        return -2;

    localrq->bWrite = 0;
    tid = localrq->tid;

    PRINTF("kvm_cpu_wrapper[%d]: Read tid = %d, addr = 0x%08x, nbytes = %d\n", 
			m_cpuindex, tid, (uint32_t) addr, nbytes);
    send_req(tid, addr, adata, nbytes, 0);

    if (!localrq->bDone)
        wait (localrq->evDone);

    *((unsigned int *) adata + 0) = localrq->low_word;
    *((unsigned int *) adata + 1) = localrq->high_word;

    m_rqs->FreeRequest (localrq);

    for (i = 0; i < nbytes; i++)
        *((unsigned char *) &ret + i) = adata[i];

    wait(20, SC_US);
    return ret;
}

void kvm_cpu_wrapper::write (uint64_t addr,
    unsigned char *data, int nbytes, int bIO)
{
    unsigned char                   tid;
    kvm_wrapper_request            *localrq;

    if (m_unblocking_write)
        localrq = m_rqs->GetNewRequest (bIO);
    else
        localrq = m_rqs->GetNewRequest (0);

    if (localrq == NULL)
        return;

    localrq->bWrite = 1;
    tid = localrq->tid;

    PRINTF("kvm_cpu_wrapper[%d]: Write tid = %d, addr = 0x%08x, nbytes = %d, p = 0x%08x\n",
			m_cpuindex, tid, (uint32_t) addr, nbytes, (uint32_t) localrq);
    send_req(tid, addr, (unsigned char *)data, nbytes, 1);

    if (!m_unblocking_write)
    {
        if (!localrq->bDone)
            wait (localrq->evDone);
        m_rqs->FreeRequest (localrq);
    }

    wait(20, SC_US);
    return;

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
