#include <stdio.h>
#include <kvm_wrapper.h>
#include <streambuf>
#include <iomanip>

using namespace std;

#define DEBUG_KVM_WRAPPER false
#define DOUT_NAME if(DEBUG_KVM_WRAPPER) std::cout << this->name() << ": "

extern "C" {
	#include <libkvm-main.h>
}

kvm_wrapper::kvm_wrapper (sc_module_name name, uint32_t node_id,
						  uint32_t ninterrupts, uint32_t *int_cpu_mask, uint32_t num_cpus,
						  uint64_t ram_size, const char * kernel, const char * boot_loader,
						  void * kvm_userspace_mem_addr)
	: sc_module(name)
{
    m_ncpu = num_cpus;
    m_kvm_import_export.imp_kvm_wrapper = this;	// Export to KVM Tool Library.
    m_ninterrupts = ninterrupts;
    interrupt_ports = NULL;

    if (m_ninterrupts)
    {
        interrupt_ports = new sc_in<bool> [m_ninterrupts];
        m_irq_cpu_mask = new int[m_ninterrupts];
        for (int i = 0; i < m_ninterrupts; i++)
            m_irq_cpu_mask[i] = int_cpu_mask[i];
    }

    m_cpu_interrupts_status     = new unsigned long[m_ncpu];
    m_cpu_interrupts_raw_status = new unsigned long[m_ncpu];
    for (int i = 0; i < m_ncpu; i++)
    {
        m_cpu_interrupts_status[i]     = 0;
        m_cpu_interrupts_raw_status[i] = 0;
    }

	// TODO: See if we can move these counters to CPU threads
    m_cpu_instrs_count = 0;
    m_cpu_cycles_count = 0;
    m_cpu_loads_count  = 0;
    m_cpu_stores_count = 0;

    m_cpus = new kvm_cpu_wrapper_t * [m_ncpu];
    m_kvm_instance = kvm_internal_init(& m_kvm_import_export, m_ncpu, ram_size, kernel, 
                                       boot_loader, kvm_userspace_mem_addr);

    for (int i = 0; i < m_ncpu; i++)
    {
        char            *s = new char [50];
        sprintf (s, "CPU-%d", i);
        m_cpus[i] = new kvm_cpu_wrapper_t (s, m_kvm_instance, node_id + i, i, & m_kvm_import_export);
//        m_cpus[i]->m_port_access (*this);
    }

    SC_THREAD (interrupts_thread);
}

kvm_wrapper::~kvm_wrapper ()
{
    int                 i;

    if (m_cpus)
    {
        for (i = 0; i < m_ncpu; i++)
            delete m_cpus [i];

        delete [] m_cpus;
    }

    if (interrupt_ports)
        delete [] interrupt_ports;

    if (m_irq_cpu_mask)
        delete [] m_irq_cpu_mask;

    delete [] m_cpu_interrupts_raw_status;
    delete [] m_cpu_interrupts_status;
}

class my_sc_event_or_list : public sc_event_or_list		// SC Even OR List (Any one event)
{
public:
    inline my_sc_event_or_list (const sc_event& e, bool del = false)
        : sc_event_or_list (e, del) {}
    inline my_sc_event_or_list& operator | (const sc_event& e)
    {push_back (e);return *this;}
};

void kvm_wrapper::interrupts_thread ()
{
    if (!m_ninterrupts)			// quit if no interrupts
        return;

    int                     i, cpu;
    bool                    *bup, *bdown;
    unsigned long           val;
    my_sc_event_or_list     eventlist (interrupt_ports[0].default_event ());

    bup = new bool[m_ncpu];
    bdown = new bool[m_ncpu];

    for (i = 1; i < m_ninterrupts; i++)
        eventlist | interrupt_ports[i].default_event ();

    while (1)
    {
        wait (eventlist);

        for (cpu = 0; cpu < m_ncpu; cpu++)
        {
            bup[cpu] = false;
            bdown[cpu] = false;
        }

        val = 1;
        for (i = 0; i < m_ninterrupts; i++)
        {
            if (interrupt_ports[i].posedge ())
            {
                m_interrupts_raw_status |= val;
                for (cpu = 0; cpu < m_ncpu; cpu++)
                {
                    if (m_irq_cpu_mask[i] & (1 << cpu))
                    {
                        m_cpu_interrupts_raw_status[cpu] |= val;
                        if (val & m_interrupts_enable)
                        {
                            if (!m_cpu_interrupts_status[cpu]){
                                bup[cpu] = true;
                            }
                            m_cpu_interrupts_status[cpu] |= val;
                        }
                    }
                }
            }
            else
                if (interrupt_ports[i].negedge ())
                {
                    m_interrupts_raw_status &= ~val;
                    for (cpu = 0; cpu < m_ncpu; cpu++)
                    {
                        if (m_irq_cpu_mask[i] & (1 << cpu))
                        {
                            m_cpu_interrupts_raw_status[cpu] &= ~val;
                            if (val & m_interrupts_enable)
                            {
                                m_cpu_interrupts_status[cpu] &= ~val;
                                if (!m_cpu_interrupts_status[cpu])
                                    bdown[cpu] = true;
                            }
                        }
                    }
                }

            val <<= 1;
        }

		// cout << "MMH@Interrupt: ...... " << endl;
		// kvm_interrupt_cpu(m_kvm_instance, 0 /* cpu_id */);
		// kvm__irq_trigger(m_kvm_instance, 1);

        for (cpu = 0; cpu < m_ncpu; cpu++)
        {
            if (bup[cpu] && !m_cpus[cpu]->m_swi)
            {
                 cout << "******INT SENT (In Theory) ***** to cpu " << cpu << endl;
//                m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 1);
//                m_cpus[cpu]->wakeup ();
            }
            else
                if (bdown[cpu] && !m_cpus[cpu]->m_swi)
                {
                    //wait (2.5, SC_NS);
//                    m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 0);
                }
        }
    }
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
