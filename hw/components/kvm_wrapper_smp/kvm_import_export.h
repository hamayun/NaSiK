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

#ifndef __KVM_IMPORT_EXPORT__
#define __KVM_IMPORT_EXPORT__

#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

    struct kvm_import_export_t;
    struct kvm_counters_t;

    typedef int             (*gdb_srv_start_and_wait_kvm_fc_t) (void* instance, int port);

    // Imported by KVM
    struct kvm_counters_t
    {
        uint64_t            no_instructions;
        uint64_t            no_cycles;
        uint64_t            no_mem_write;
        uint64_t            no_mem_read;
        uint64_t            no_dcache_miss;
        uint64_t            no_icache_miss;
        uint64_t            no_io_write;
        uint64_t            no_io_read;
    };

    struct kvm_import_export_t
    {
        void *                          imp_kvm_wrapper;              // KVM Imports KVM WRAPPER Reference from SystemC
        gdb_srv_start_and_wait_kvm_fc_t exp_gdb_srv_start_and_wait;   // KVM Exports GDB Server Reference to SystemC
    };

#ifdef __cplusplus
}
#endif

#endif  // __KVM_IMPORT_EXPORT__

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
