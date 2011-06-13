
#include <kvm_cpu_wrapper.h>

#define DEBUG_KVM_WRAPPER true
#define DOUT_NAME if(DEBUG_KVM_WRAPPER) std::cout << this->name() << ": "

extern "C" {
	void soc_kvm_run();
}

#define QEMU_STNOC_ADDRESS_DIFFERENCE 0x00000000
static unsigned char s_st_operation_codes[] = {0xDE, 0x00, 0x10, 0xDE, 0x20, 0xDE, 0xDE, 0xDE, 0x30};
static unsigned char s_st_operation_mask_be[] = {0xDE, 0x01, 0x03, 0xDE, 0x0F, 0xDE, 0xDE, 0xDE, 0xFF};

kvm_cpu_wrapper::kvm_cpu_wrapper (sc_module_name name, char *elf_file, uintptr_t app_base_addr, int node_id)
	: master_device (name)/*,
          ExecutionSpy(ANALYZE_OPTION, ONLINE_OPTION, NO_THREAD_OPTION, elf_file, app_base_addr)*/
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
    //while (1)
    {
        soc_kvm_run();
    }
}

// Here we get the Annotation Trace (A buffer containing pointers to Annotation DBs)
/*
void kvm_cpu_wrapper::compute(annotation_t *trace, uint32_t count)
{
    uint32_t  index;
    uint32_t  instructions = 0;
    uint32_t  cycles = 0;

    DOUT_NAME << __func__ << " count " << count << std::endl;
    for( index = 0 ; index < count; index++ )
    {
        trace[index].eu = (uintptr_t)this;
        //trace[index].thread = _current_thread_id; // Store the current thread id in annotation object
        if(trace[index].type == BB_DEFAULT)
        {
            cycles += trace[index].db->CycleCount;
            instructions += trace[index].db->InstructionCount;
        }
    }
    DOUT_NAME << __func__ << " wait " << cycles << std::endl;
    wait(cycles, SC_NS);
}
*/
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
		/*
    void print_annotation_db(annotation_db_t *db)
    {
        printf("@db = 0x%08x\t", (uint32_t) db);
        printf("Type: [ ");
        if(db->Type == BB_DEFAULT) printf("BB_DEFAULT ");
        else
        {
            if(db->Type & BB_ENTRY) printf("BB_ENTRY ");
            if(db->Type & BB_RETURN) printf("BB_RETURN ");
        }
        printf("]\t");

        printf("Instr. Cnt = 0x%x, Cycle Cnt = 0x%x, Load Cnt = 0x%x, Store Cnt = 0x%x, FuncAddr = 0x%x\n",
               db->InstructionCount, db->CycleCount, db->LoadCount, db->StoreCount, db->FuncAddr);
    }
		*/
    void
    systemc_annotate_function(kvm_cpu_wrapper_t *_this, void *db)
    {
        //this->annotate((annotation_db_t *) db);
        annotation_db_t *pdp = (annotation_db_t *) db;
        wait(pdp->CycleCount, SC_NS);
    }
}

