
#include <kvm_cpu_wrapper.h>

extern "C" {
	void soc_kvm_run();
}

#define QEMU_STNOC_ADDRESS_DIFFERENCE 0x00000000
static unsigned char s_st_operation_codes[] = {0xDE, 0x00, 0x10, 0xDE, 0x20, 0xDE, 0xDE, 0xDE, 0x30};
static unsigned char s_st_operation_mask_be[] = {0xDE, 0x01, 0x03, 0xDE, 0x0F, 0xDE, 0xDE, 0xDE, 0xFF};

kvm_cpu_wrapper::kvm_cpu_wrapper (sc_module_name name, int node_id)
	: master_device (name)
{
    m_rqs = new qemu_wrapper_requests (100);
    m_unblocking_write = 0;
    m_node_id = node_id;

    SC_THREAD (cpuThread);
}

kvm_cpu_wrapper::~kvm_cpu_wrapper ()
{
}

// Defined by Hao
// A thread used to simulate the kvm
void kvm_cpu_wrapper::cpuThread ()
{
	while (1)
	{
		soc_kvm_run();
	}
}

void kvm_cpu_wrapper::rcv_rsp(unsigned char tid, unsigned char *data,
                               bool bErr, bool bWrite)
{

    qemu_wrapper_request            *localrq;

    localrq = m_rqs->GetRequestByTid (tid);
    if (localrq == NULL)
    {
        cout << "[Error: " << name () << " received a response for an unknown TID 0x"
             << std::hex << (unsigned long) tid << "]" << endl;
        return;
    }

    if (m_unblocking_write && localrq->bWrite) //response for a write cmd
    {
        m_rqs->FreeRequest (localrq);
        return;
    }

    localrq->low_word = *((unsigned long *)data + 0);
    localrq->high_word = *((unsigned long *)data + 1);
    localrq->bDone = 1;
    localrq->evDone.notify ();

    return;
}

uint64_t kvm_cpu_wrapper::read (uint64_t addr,
    int nbytes, int bIO)
{
    unsigned char           adata[8];
    uint64_t                ret;
    int                     i;
    unsigned char           tid;
    qemu_wrapper_request   *localrq;

    if (m_unblocking_write)
        localrq = m_rqs->GetNewRequest (1);
    else
        localrq = m_rqs->GetNewRequest (0);

    if (localrq == NULL)
        return -2;

    localrq->bWrite = 0;
    tid = localrq->tid;

    send_req(tid, addr, adata, nbytes, 0);

    if (!localrq->bDone)
        wait (localrq->evDone);

    *((unsigned long *) adata + 0) = localrq->low_word;
    *((unsigned long *) adata + 1) = localrq->high_word;

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
    qemu_wrapper_request            *localrq;

    if (m_unblocking_write)
        localrq = m_rqs->GetNewRequest (bIO);
    else
        localrq = m_rqs->GetNewRequest (0);

    if (localrq == NULL)
        return;

    localrq->bWrite = 1;
    tid = localrq->tid;

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

extern "C"
{
    uint64_t
    systemc_kvm_read_memory (kvm_cpu_wrapper_t *_this, uint64_t addr,
        int nbytes, unsigned long *ns, int bIO)
    {
        uint64_t						ret;
 
        ret = _this->read (addr, nbytes, bIO);

        return ret;
    }

    void
    systemc_kvm_write_memory (kvm_cpu_wrapper_t *_this, uint64_t addr,
        unsigned char *data, int nbytes, unsigned long *ns, int bIO)
    {
       _this->write (addr, data, nbytes, bIO);
	}
	
}

