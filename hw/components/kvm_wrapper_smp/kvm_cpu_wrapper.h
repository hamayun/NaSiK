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

#ifndef __KVM_CPU_WRAPPER__
#define __KVM_CPU_WRAPPER__

#include <kvm_wrapper_request.h>
#include <kvm_wrapper_access_interface.h>
#include <kvm_wrapper_consts.h>
#include <kvm_imported.h>

#include <master_device.h>

using namespace noc;

class kvm_cpu_wrapper : public master_device
{
public:
    SC_HAS_PROCESS (kvm_cpu_wrapper);
    kvm_cpu_wrapper (sc_module_name name, void *kvm_instance, unsigned int node_id,
					 int cpuindex, kvm_import_t *kvm_import);
    ~kvm_cpu_wrapper ();

public:
    uint64_t read (uint64_t address, int nbytes, int bIO);
    void write (uint64_t address, unsigned char *data, int nbytes, int bIO);

    void set_unblocking_write (bool val);
/*
    uint64 get_no_cycles ();
    void wait_wb_empty ();
    void wakeup ();
    void sync ();
*/

private:
	void rcv_rsp(unsigned char tid, unsigned char *data, bool bErr, bool bWrite);

    // local functions
    uint32_t read (uint32_t address, uint8_t nbytes, int bIO);
    void write (uint32_t address, uint32_t data, uint8_t nbytes, int bIO);

public:
	// TODO: Enable this Interface
    // ports
    //sc_port<kvm_wrapper_access_interface>  m_port_access;

private:
    // signals & events
//    sc_event                                m_ev_wakeup;
//    sc_event                                m_ev_sync;

    //other attributes
    kvm_wrapper_requests                   *m_rqs;
    void                                   *m_cpuenv;
    void                                   *m_kvm_instance;
    bool                                    m_unblocking_write;

    kvm_import_t                           *m_kvm_import;

public:
    int                                     m_cpuindex;
    unsigned long                           m_crt_cpu_thread;
    unsigned long                           m_swi;
};

typedef kvm_cpu_wrapper kvm_cpu_wrapper_t;

#endif // __KVM_CPU_WRAPPER__

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
