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

#ifndef __KVM_CPU_WRAPPER_H__
#define __KVM_CPU_WRAPPER_H__

//#define USE_EXECUTION_SPY

#include <qemu_wrapper_request.h>
#include <execution_spy.h>
#include <master_device.h>

using namespace noc;
class cpu_logs;

#define ANALYZE_OPTION true
#define ONLINE_OPTION true
#define NO_THREAD_OPTION false        // if true analyzer thread will *NOT* be created.

class kvm_cpu_wrapper : public master_device
#ifdef USE_EXECUTION_SPY
    , public ExecutionSpy
#endif
{
public:
    SC_HAS_PROCESS (kvm_cpu_wrapper);
    kvm_cpu_wrapper (sc_module_name name, char *elf_file, uintptr_t app_base_addr, int node_id=0);
    ~kvm_cpu_wrapper ();

private:
    void cpuThread ();
#ifdef USE_EXECUTION_SPY
    void compute(annotation_t *trace, uint32_t count);
#endif

    void receiveThread ();
    void rcv_rsp(unsigned char tid, unsigned char *data, bool bErr, bool bWrite);

public:
    uint64_t read (uint64_t address, int nbytes, int bIO);
    void write (uint64_t address, unsigned char *data,
                int nbytes, int bIO);

private:
    qemu_wrapper_requests          *m_rqs;
    bool                            m_unblocking_write;
};

typedef kvm_cpu_wrapper kvm_cpu_wrapper_t;

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
