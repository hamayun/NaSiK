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

#include <stdio.h>
#include <kvm_wrapper.h>
#include <streambuf>

using namespace std;

#define DEBUG_KVM_WRAPPER false 
#define DOUT_NAME if(DEBUG_KVM_WRAPPER) std::cout << this->name() << ": "

extern "C" {
	#include <libkvm-main.h>

	typedef uint32_t u32;
	typedef uint8_t u8;

	extern int irq__register_device(u32 dev, u8 *num, u8 *pin, u8 *line);
	extern void kvm__irq_trigger(void *kvm, int irq);
    void kvm_wrapper_SLS_banner (void) __attribute__ ((constructor));
}

kvm_wrapper::kvm_wrapper (sc_module_name name, uint32_t node_id,
						  uint32_t ninterrupts, uint32_t *int_cpu_mask, uint32_t num_cpus,
						  uint64_t ram_size, const char * kernel, const char * boot_loader,
						  uintptr_t * kvm_userspace_mem_addr)
	: sc_module(name)
{
	m_ncpu = num_cpus;
    m_kvm_import_export.imp_kvm_wrapper = this;	// Export to KVM Tool Library.
    
	m_ninterrupts = ninterrupts;
    interrupt_ports = NULL;
    m_irq_cpu_mask = 0;

    m_interrupts_raw_status = 0;
	// Interrupts are disabled by default
    m_interrupts_enable = 0;
    if (m_ninterrupts)
    {
        interrupt_ports = new sc_in<bool> [m_ninterrupts];
        m_irq_cpu_mask = new int[m_ninterrupts];
        for (int i = 0; i < m_ninterrupts; i++)
		{
            DOUT_NAME << "m_irq_cpu_mask[" << i << "] = " << int_cpu_mask[i] << endl;
            m_irq_cpu_mask[i] = int_cpu_mask[i];
		}
    }

    m_cpu_interrupts_status     = new unsigned long[m_ncpu];
    m_cpu_interrupts_raw_status = new unsigned long[m_ncpu];
    for (int i = 0; i < m_ncpu; i++)
    {
        m_cpu_interrupts_status[i]     = 0;
        m_cpu_interrupts_raw_status[i] = 0;
    }

    m_cpus = new kvm_cpu_wrapper_t * [m_ncpu];
    m_kvm_instance = kvm_internal_init(& m_kvm_import_export, m_ncpu, ram_size);

	//TODO: Do the proper interrupt setup here
	{
		u32 dev = 1;
		u8 num, pin, line;

		if(irq__register_device(dev, &num, &pin, &line) < 0)
		{
			cout << "Error in Registering IRQ Device" << endl;
		}
	}

	kvm_setup_bios_and_ram(m_kvm_instance, kvm_userspace_mem_addr, kernel, boot_loader);

    for (int i = 0; i < m_ncpu; i++)
    {
        char            *s = new char [50];
		sprintf (s, "CPU-%d", i);
		m_cpus[i] = new kvm_cpu_wrapper_t (s, m_kvm_instance, node_id + i,
										   i, & m_kvm_import_export, this);
//        m_cpus[i]->m_port_access (*this);
		m_cpu_running[i] = false;
    }

	m_running_count = 0;
    SC_THREAD (interrupts_thread);
    SC_THREAD (trigger_thread);
}

void kvm_wrapper::trigger_thread()
{
	while(1)
	{
		wait(m_ev_cpu_got_blocked);
		if(m_running_count == 0)
		{
			cout << "Firing Boot CPU Event" << endl;
			kvm_cpu_unblock(0);
			m_cpus[0]->m_ev_runnable.notify();
		}
	}
}

void kvm_wrapper::kvm_cpu_block(int cpu_id)
{
	m_kvm_mutex.lock();

	if(m_cpu_running[cpu_id])		// If running then set status to blocked
	{
		m_cpu_running[cpu_id] = false;
		m_running_count--;
	}

	m_kvm_mutex.unlock();
	m_ev_cpu_got_blocked.notify();
}

void kvm_wrapper::kvm_cpu_unblock(int cpu_id)
{
	m_kvm_mutex.lock();

	if(!m_cpu_running[cpu_id])		// If blocked then set status to running
	{
		m_cpu_running[cpu_id] = true;
		m_running_count++;
	}

	m_kvm_mutex.unlock();
}

void kvm_wrapper::kvm_cpus_status()
{
	int i = 0;

	cout << "KVM CPUs: " << m_running_count << "/" << m_ncpu << " [ ";
	for(; i< m_ncpu; i++)
		cout << (m_cpu_running[i] ? "1 " : "0 "); 
	cout << "] " << sc_time_stamp() << endl;
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

void kvm_wrapper::set_unblocking_write (bool val)
{
    int                 i;
    for (i = 0; i < m_ncpu; i++)
        m_cpus [i]->set_unblocking_write (val);
}

class my_sc_event_or_list : public sc_event_or_list		// SC Even OR List (Any one event)
{
public:
    inline my_sc_event_or_list (const sc_event& e)
        : sc_event_or_list (e) {}
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
                DOUT_NAME << "Interrupt ... posedge() @ " << sc_time_stamp() << endl;

                m_interrupts_raw_status |= val;		// global interrupts; actually occurred
                for (cpu = 0; cpu < m_ncpu; cpu++)
                {   
					// Check if this interrupt is enabled on the current cpu
                    if (m_irq_cpu_mask[i] & (1 << cpu))	
                    {
						// cpu-wise interrupt status; ones that allow to be interrupted.
                        m_cpu_interrupts_raw_status[cpu] |= val;	
                        if (val & m_interrupts_enable)			// globale interrupts status enabled ?
                        {
                            if (!m_cpu_interrupts_status[cpu]){	// current interrupt status of the cpu
                                bup[cpu] = true;
                            }
                            m_cpu_interrupts_status[cpu] |= val;// update the current interrupt status of cpu
                        }
                    }
                }
            }
            else
                if (interrupt_ports[i].negedge ())
                {
                    DOUT_NAME << "Interrupt ... negedge() @ " << sc_time_stamp() << endl;

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

        for (cpu = 0; cpu < m_ncpu; cpu++)
        {
            if (bup[cpu]) // if (bup[cpu] && !m_cpus[cpu]->m_swi)
            {
                cout << "***** INT SENT RE ***** to VCPU-" << cpu << endl;
                // TTY Interrupt Number; Given as i8259_VECTOR_OFFSET + 5 in linker script of Software
                kvm__irq_trigger(m_kvm_instance, 5);
            }
            else if (bdown[cpu]) 	// if (bdown[cpu] && !m_cpus[cpu]->m_swi)
            {
                // wait (2.5, SC_NS);
                // cout << "***** INT SENT FE ***** to VCPU-" << cpu << endl;
            }
        }
    }
}

// kvm_wrapper_access_interface
unsigned long kvm_wrapper::get_no_cpus  ()
{
    return m_ncpu;
}

unsigned long kvm_wrapper::get_int_status ()
{
    return m_interrupts_raw_status & m_interrupts_enable;
}

unsigned long kvm_wrapper::get_int_enable ()
{
    return m_interrupts_enable;
}

void kvm_wrapper::set_int_enable (unsigned long val)
{
    int            cpu;
    for (cpu = 0; cpu < m_ncpu; cpu++)
    {
        if (!m_cpu_interrupts_status[cpu] && (m_cpu_interrupts_raw_status[cpu] & val)) 
			/* && !m_cpus[cpu]->m_swi) */
		{
            //m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 1);
		}
        else
		{
            if (m_cpu_interrupts_status[cpu] && !(m_cpu_interrupts_raw_status[cpu] & val))
				/* && !m_cpus[cpu]->m_swi) */
			{
                //m_qemu_import.qemu_irq_update (m_qemu_instance, 1 << cpu, 0);
			}
		}

        m_cpu_interrupts_status[cpu] = m_cpu_interrupts_raw_status[cpu] & val;
    }

    m_interrupts_enable = val;
}

extern "C"
{
    void
    kvm_wrapper_SLS_banner (void)
    {
        fprintf (stdout,
                "================================================================================\n"
                "|  This simulation uses the KVM/SystemC Wrapper from the RABBITS' framework    |\n"
                "|                     Copyright (c) 2009 - 2013 TIMA/SLS                       |\n"
                "================================================================================\n");
    }
}

