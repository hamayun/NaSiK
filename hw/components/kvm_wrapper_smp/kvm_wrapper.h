/*
 *  Copyright (c) 2010 TIMA Laboratory
 *
 *  This file is part of NaSiK.
 *
 *  NaSiK is free software: you can redistribute it and/or modify
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

#ifndef __KVM_WRAPPER_H__
#define __KVM_WRAPPER_H__

#define USE_ANNOTATION_BUFFERS
// Note: Execution Spy may not work properly due to regression.
//#define USE_EXECUTION_SPY
#define ENABLE_CPU_STATS

#include <kvm_wrapper_access_interface.h>
#include <kvm_wrapper_request.h>
#include <kvm_cpu_wrapper.h>
#include <kvm_import_export.h>
#include <master_device.h>

using namespace noc;
class cpu_logs;

// TODO: Implement the kvm_wrapper_access_interface
class kvm_wrapper : public sc_module //, public kvm_wrapper_access_interface
{
public:
    SC_HAS_PROCESS (kvm_wrapper);
    kvm_wrapper (sc_module_name name, uint32_t node_id,
				 uint32_t ninterrupts, uint32_t *int_cpu_mask, uint32_t num_cpus,
				 uint64_t ram_size, const char * kernel, const char * boot_loader,
				 void * kvm_userspace_mem_addr);
    ~kvm_wrapper ();

    void        		   *m_kvm_instance;
    kvm_import_export_t 	m_kvm_import_export;

	int             		m_ncpu;		/* Number of CPUs in this Wrapper */
	kvm_cpu_wrapper_t     **m_cpus;

	inline kvm_cpu_wrapper_t * get_cpu (int i) {return m_cpus[i];}

    uint64_t read (uint32_t cpu_id, uint64_t address, int nbytes, int bIO);
    void write (uint32_t cpu_id, uint64_t address, unsigned char *data, int nbytes, int bIO);

    //void update_cpu_stats(annotation_db_t *pdb);
    void log_cpu_stats();
    void log_cpu_stats_delta(unsigned char *data);

	// Interrupt Support
    sc_in<bool>                         *interrupt_ports;
	
private:
    void interrupts_thread ();

    int                                  m_ninterrupts;
    unsigned long                       *m_cpu_interrupts_raw_status;
    unsigned long                       *m_cpu_interrupts_status;
    unsigned long                        m_interrupts_raw_status;
    unsigned long                        m_interrupts_enable;
    int                                 *m_irq_cpu_mask;

    // Statistics extracted from Annotations
    uint64_t                        m_cpu_instrs_count;
    uint64_t                        m_cpu_cycles_count;
    uint64_t                        m_cpu_loads_count;
    uint64_t                        m_cpu_stores_count;
};

typedef kvm_wrapper kvm_wrapper_t;

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
