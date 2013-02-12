#include <stdio.h>
#include <kvm_wrapper.h>
#include <streambuf>

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

    m_cpus = new kvm_cpu_wrapper_t * [m_ncpu];
    m_kvm_instance = kvm_internal_init(& m_kvm_import_export, m_ncpu, ram_size, kernel, 
                                       boot_loader, kvm_userspace_mem_addr);

    for (int i = 0; i < m_ncpu; i++)
    {
        char            *s = new char [50];
        sprintf (s, "CPU-%d", i);
        m_cpus[i] = new kvm_cpu_wrapper_t (s, m_kvm_instance, node_id + i,
										   i, & m_kvm_import_export);
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
