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
//#define USE_EXECUTION_SPY
#define ENABLE_CPU_STATS

#include <kvm_wrapper_request.h>
#include <kvm_imported.h>
#include <master_device.h>

using namespace noc;
class cpu_logs;

#define KVM_ADDR_BASE                               0xE0000000
#define KVM_ADDR_END                                0xEFFFFFFF
#define LOG_KVM_STATS                               0x0050
#define LOG_KVM_STATS_DELTA                         0x0058

class kvm_wrapper : public master_device
{
public:
    SC_HAS_PROCESS (kvm_wrapper);
    kvm_wrapper (sc_module_name name, uint32_t num_cores, int node_id=0);
    ~kvm_wrapper ();

public:
    void                           *m_kvm_instance;
    kvm_import_t                    m_kvm_import;   		// Functions that are defined in KVM Library

private:
    void kvm_cpu_thread ();

    void receiveThread ();
    void rcv_rsp(unsigned char tid, unsigned char *data, bool bErr, bool bWrite);

public:
    uint64_t read (uint64_t address, int nbytes, int bIO);
    void write (uint64_t address, unsigned char *data, int nbytes, int bIO);

    //void update_cpu_stats(annotation_db_t *pdb);
    void log_cpu_stats();
    void log_cpu_stats_delta(unsigned char *data);

private:
    uint32_t                        m_nr_cores;
    kvm_wrapper_requests           *m_rqs;
    bool                            m_unblocking_write;

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
