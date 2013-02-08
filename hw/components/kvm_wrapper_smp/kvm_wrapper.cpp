#include <stdio.h>
#include <kvm_wrapper.h>
#include <streambuf>
#include <iomanip>

using namespace std;

#define DEBUG_KVM_WRAPPER false
#define DOUT_NAME if(DEBUG_KVM_WRAPPER) std::cout << this->name() << ": "

extern "C" {
	void * kvm_internal_init(struct kvm_import_t *ki, uint32_t num_cpus, uint64_t ram_size /* MBs */, 
							 const char * kernel, const char * boot_loader, void * kvm_userspace_mem_addr);
    int kvm_run_cpus();
}

kvm_wrapper::kvm_wrapper (sc_module_name name, uint32_t num_cpus, uint64_t ram_size, 
				 const char * kernel, const char * boot_loader, void * kvm_userspace_mem_addr,
				 int node_id)
	: sc_module(name)
{
    m_cpu_instrs_count = 0;
    m_cpu_cycles_count = 0;
    m_cpu_loads_count  = 0;
    m_cpu_stores_count = 0;

    m_ncpus = num_cpus;
    m_cpus = new kvm_cpu_wrapper_t * [m_ncpus];

    m_kvm_instance = kvm_internal_init(& m_kvm_import, m_ncpus, ram_size, kernel, 
									   boot_loader, kvm_userspace_mem_addr);

    for (int i = 0; i < m_ncpus; i++)
    {
        char            *s = new char [50];
        sprintf (s, "CPU-%d", i);
        m_cpus[i] = new kvm_cpu_wrapper_t (s, m_kvm_instance, node_id + i, i, &m_kvm_import);
        //m_cpus[i]->m_port_access (*this);
    }

    SC_THREAD (kvm_cpus_thread);
}

kvm_wrapper::~kvm_wrapper ()
{
    int                 i;

    if (m_cpus)
    {
        for (i = 0; i < m_ncpus; i++)
            delete m_cpus [i];

        delete [] m_cpus;
    }

/*
    if (interrupt_ports)
        delete [] interrupt_ports;

    if (m_irq_cpu_mask)
        delete [] m_irq_cpu_mask;

    delete [] m_cpu_interrupts_raw_status;
    delete [] m_cpu_interrupts_status;
*/
}

// A thread used to simulate the kvm processors
// We use only one SystemC thread here, each cpu is represented by a pthread in linux-kvm library.
void kvm_wrapper::kvm_cpus_thread ()
{
    int rval = 0;
    rval = kvm_run_cpus();
    cout << "kvm_cpus_thread: KVM Run Exited ... Return Value = " << rval << endl;
}

/*
void kvm_wrapper::update_cpu_stats(annotation_db_t *pdb)
{
    m_cpu_instrs_count += pdb->InstructionCount;
    m_cpu_cycles_count += pdb->CycleCount;
    m_cpu_loads_count  += pdb->LoadCount;
    m_cpu_stores_count += pdb->StoreCount;
}
*/

void kvm_wrapper::log_cpu_stats()
{
    std::cout << "KVM STATS: cpu_instrs_count = " << m_cpu_instrs_count
              << " cpu_cycles_count = " << m_cpu_cycles_count
              << " cpu_loads_count = " << m_cpu_loads_count
              << " cpu_stores_count = " << m_cpu_stores_count << std::endl;
}

void kvm_wrapper::log_cpu_stats_delta(unsigned char *data)
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

uint64_t kvm_wrapper::read (uint32_t cpu_id, uint64_t addr, int nbytes, int bIO)
{
	// Forward Request to Appropriate KVM_CPU_WRAPPER
	return (this->m_cpus[cpu_id]->read(addr, nbytes, bIO));
}

void kvm_wrapper::write (uint32_t cpu_id, uint64_t addr, unsigned char *data, int nbytes, int bIO)
{
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

	// Forward Request to Appropriate KVM_CPU_WRAPPER
	this->m_cpus[cpu_id]->write(addr, data, nbytes, bIO);

	return;
}

extern "C"
{
    uint64_t
    systemc_kvm_read_memory (kvm_wrapper_t *_this, uint32_t cpu_id, uint64_t addr,
        int nbytes, unsigned int *ns, int bIO)
    {
        uint64_t ret;

        ret = _this->read (cpu_id, addr, nbytes, bIO);
        return ret;
    }

    void
    systemc_kvm_write_memory (kvm_wrapper_t *_this, uint32_t cpu_id, uint64_t addr,
        unsigned char *data, int nbytes, unsigned int *ns, int bIO)
    {
        _this->write (cpu_id, addr, data, nbytes, bIO);
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
    systemc_annotate_function(kvm_wrapper_t *_this, void *vm_addr, void *ptr)
    {
        _this->annotate((void *)vm_addr, (db_buffer_desc_t *) ptr);
    }
#else
    void
    systemc_annotate_function(kvm_wrapper_t *_this, void *vm_addr, void *ptr)
    {
/*
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
 */
    }
#endif
#else
    void
    systemc_annotate_function(kvm_wrapper_t *_this, void *vm_addr, void *ptr)
    {
/*
        annotation_db_t *pdb = (annotation_db_t *) ptr;
        wait(pdb->CycleCount, SC_NS);
#ifdef ENABLE_CPU_STATS
        update_cpu_stats(pdb);
#endif
*/
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
