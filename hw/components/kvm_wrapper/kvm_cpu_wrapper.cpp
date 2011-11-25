#include <kvm_cpu_wrapper.h>
#include <streambuf>
#include <iomanip>

#define DEBUG_KVM_WRAPPER false
#define DOUT_NAME if(DEBUG_KVM_WRAPPER) std::cout << this->name() << ": "

extern "C" {
    void soc_kvm_run(unsigned int cpuid);
}

//#define QEMU_STNOC_ADDRESS_DIFFERENCE 0x00000000
//static unsigned char s_st_operation_codes[] = {0xDE, 0x00, 0x10, 0xDE, 0x20, 0xDE, 0xDE, 0xDE, 0x30};
//static unsigned char s_st_operation_mask_be[] = {0xDE, 0x01, 0x03, 0xDE, 0x0F, 0xDE, 0xDE, 0xDE, 0xFF};

kvm_cpu_wrapper::kvm_cpu_wrapper (sc_module_name name, char *elf_file, uintptr_t app_base_addr, int node_id)
	: master_device (name), ExecutionSpy(ANALYZE_OPTION, ONLINE_OPTION, NO_THREAD_OPTION, elf_file, app_base_addr)
{
    m_rqs = new qemu_wrapper_requests (100);
    m_unblocking_write = 0;
    m_node_id = node_id;

    m_cpu_instrs_count = 0;
    m_cpu_cycles_count = 0;
    m_cpu_loads_count  = 0;
    m_cpu_stores_count = 0;

    SC_THREAD (cpuThread);
}

kvm_cpu_wrapper::~kvm_cpu_wrapper ()
{
}

// Defined by Hao
// A thread used to simulate the kvm
void kvm_cpu_wrapper::cpuThread ()
{
    cout << "cpuThread: Calling KVM Run ... [" << m_node_id << "]" << endl;
    soc_kvm_run(m_node_id);
}

// Here we get the Annotation Trace (A buffer containing pointers to Annotation DBs)
// This function is used when Analyzer is Enabled.
void kvm_cpu_wrapper::compute(annotation_t *trace, uint32_t count)
{
    uint32_t  index;
    uint32_t  instructions = 0;
    uint32_t  cycles = 0;

    DOUT_NAME << __func__ << std::dec << " count = " << count << std::endl;
    for( index = 0 ; index < count; index++ )
    {
        trace[index].eu = (uintptr_t)this;
        if(trace[index].type == BB_DEFAULT)
        {
            cycles += trace[index].db->CycleCount;
            instructions += trace[index].db->InstructionCount;
        }
    }

    DOUT_NAME << __func__ << " wait " << std::dec << cycles << " cycles" << std::endl;
    wait(cycles, SC_NS);
}

void kvm_cpu_wrapper::update_cpu_stats(annotation_db_t *pdb)
{
    m_cpu_instrs_count += pdb->InstructionCount;
    m_cpu_cycles_count += pdb->CycleCount;
    m_cpu_loads_count  += pdb->LoadCount;
    m_cpu_stores_count += pdb->StoreCount;
}

void kvm_cpu_wrapper::log_cpu_stats()
{
    std::cout << "KVM STATS: cpu_instrs_count = " << m_cpu_instrs_count
              << " cpu_cycles_count = " << m_cpu_cycles_count
              << " cpu_loads_count = " << m_cpu_loads_count
              << " cpu_stores_count = " << m_cpu_stores_count << std::endl;
}

void kvm_cpu_wrapper::log_cpu_stats_delta(unsigned char *data)
{
    static uint64_t cpu_instrs_count_prev = 0, cpu_cycles_count_prev = 0, cpu_loads_count_prev = 0, cpu_stores_count_prev = 0;
    uint32_t value = (uint32_t) *data;

    std::cout << "KVM STATS Delta[" << value << "]:"
              << " cpu_instrs_count = " << std::setw(12) << (m_cpu_instrs_count - cpu_instrs_count_prev)
              << " cpu_cycles_count = " << std::setw(10) << (m_cpu_cycles_count - cpu_cycles_count_prev)
              << " cpu_loads_count = "  << std::setw(10) << (m_cpu_loads_count  - cpu_loads_count_prev )
              << " cpu_stores_count = " << std::setw(10) << (m_cpu_stores_count - cpu_stores_count_prev) << std::endl;

    cpu_instrs_count_prev = m_cpu_instrs_count;
    cpu_cycles_count_prev = m_cpu_cycles_count;
    cpu_loads_count_prev  = m_cpu_loads_count;
    cpu_stores_count_prev = m_cpu_stores_count;

    if(value == 0){
        std::cout << "Exiting on Application Request ..." << std::endl;
        exit(0);
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
             << std::hex << (unsigned int) tid << "]" << endl;
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
    qemu_wrapper_request            *localrq;

    /* See if this request is for KVM Wrapper? */
    if(addr >= KVM_ADDR_BASE && addr <= KVM_ADDR_END)
    {
        addr = addr - KVM_ADDR_BASE;
        switch(addr)
        {
            case LOG_KVM_STATS:
                log_cpu_stats();
                break;

            case LOG_KVM_STATS_DELTA:
                log_cpu_stats_delta(data);
                break;
        }
        return;
    }

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
        int nbytes, unsigned int *ns, int bIO)
    {
        uint64_t						ret;
        ret = _this->read (addr, nbytes, bIO);
        return ret;
    }

    void
    systemc_kvm_write_memory (kvm_cpu_wrapper_t *_this, uint64_t addr,
        unsigned char *data, int nbytes, unsigned int *ns, int bIO)
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

#ifdef USE_ANNOTATION_BUFFERS
#ifdef USE_EXECUTION_SPY
    void
    systemc_annotate_function(kvm_cpu_wrapper_t *_this, void *vm_addr, void *ptr)
    {
        _this->annotate((void *)vm_addr, (db_buffer_desc_t *) ptr);
    }
#else
    void
    systemc_annotate_function(kvm_cpu_wrapper_t *_this, void *vm_addr, void *ptr)
    {
        db_buffer_desc_t *pbuff_desc = (db_buffer_desc_t *) ptr;
        annotation_db_t *pdb = NULL;
        uint32_t buffer_cycles = 0;

        while(pbuff_desc->StartIndex != pbuff_desc->EndIndex)
        {
            // Get pointer to the annotation db;
            pdb = (annotation_db_t *)((uint32_t)vm_addr + (uint32_t)pbuff_desc->Buffer[pbuff_desc->StartIndex].pdb);
            buffer_cycles += pdb->CycleCount;
#ifdef ENABLE_CPU_STATS
            _this->update_cpu_stats(pdb);
#endif
            pbuff_desc->StartIndex = (pbuff_desc->StartIndex + 1) % pbuff_desc->Capacity;
        }

        wait(buffer_cycles, SC_NS);
    }
#endif
#else
    void
    systemc_annotate_function(kvm_cpu_wrapper_t *_this, void *vm_addr, void *ptr)
    {
        annotation_db_t *pdb = (annotation_db_t *) ptr;
        wait(pdb->CycleCount, SC_NS);
#ifdef ENABLE_CPU_STATS
        update_cpu_stats(pdb);
#endif
    }
#endif /* USE_ANNOTATION_BUFFERS */
}

/*
printf("BufferID = %d, StartIndex = %d,  EndIndex = %d, Capacity = %d\n",
    pbuff_desc->BufferID, pbuff_desc->StartIndex,
    pbuff_desc->EndIndex, pbuff_desc->Capacity);

printf("@db: 0x%08x, Type: %d, CC = %d, FuncAddr: 0x%08x\n",
       (uint32_t)pbuff_desc->Buffer[pbuff_desc->StartIndex], pdb->Type, pdb->CycleCount, pdb->FuncAddr);
*/

/*
db_buffer_desc_t *pbuff_desc = (db_buffer_desc_t *) pdesc;
uint32_t db_count = 0;

if(pbuff_desc->EndIndex > pbuff_desc->StartIndex)
    db_count = pbuff_desc->EndIndex - pbuff_desc->StartIndex;
else
    db_count = pbuff_desc->Capacity - (pbuff_desc->StartIndex - pbuff_desc->EndIndex);

printf("BufferID [%d] %5d --> %5d (Count = %d)\n",
        pbuff_desc->BufferID, pbuff_desc->StartIndex,
        pbuff_desc->EndIndex, db_count);
 */
