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

#include <kvm_cpu_wrapper.h>
#include <kvm_wrapper.h>
#include <kvm_import_export.h>
#include <kvm_wrapper_consts.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <iomanip>

#include <master_device.h>

//#define DEBUG_KVM_CPU_WRAPPER
#ifdef DEBUG_KVM_CPU_WRAPPER
#define PRINTF printf
#else
#define PRINTF if (0) printf
#endif

//#define ENABLE_CPU_STATS
#ifdef ENABLE_CPU_STATS
#define UPDATE_CPU_STATS(x) _this->update_cpu_stats(x)
#else
#define UPDATE_CPU_STATS(x) if(0) {} 
#endif

#define SHOW_CPU_TIMING
#ifdef SHOW_CPU_TIMING
#define KVM_CPUS_STATUS() kvm_cpus_status()
#else
#define KVM_CPUS_STATUS() if(0) {}
#endif

static struct timeval                   start_time;

extern "C" {
	#include <libkvm-main.h>
}

kvm_cpu_wrapper::kvm_cpu_wrapper (sc_module_name name, void * kvm_instance, unsigned int node_id,
								  int cpuindex, kvm_import_export_t * kvm_import_export,
   							      kvm_wrapper * parent)
: master_device (name)
{
	m_parent = parent;		// Parent KVM_Wrapper Instance
    m_kvm_import_export = kvm_import_export;
    m_kvm_instance = kvm_instance;
    m_node_id = node_id;
    m_cpuindex = cpuindex;
    m_rqs = new kvm_wrapper_requests (100);

    m_unblocking_write = 0;

    if (cpuindex == 0)
        gettimeofday (&start_time, NULL);

    m_kvm_cpu_instance = kvm_cpu_internal_init(kvm_instance, this, cpuindex);
    if(!m_kvm_cpu_instance)
    {
        cerr << "Error Initializing KVM CPU" << endl;
        return;
    }
	
	// Init the Annotation Counters
    m_cpu_instrs_count = 0;
    m_cpu_cycles_count = 0;
    m_cpu_loads_count  = 0;
    m_cpu_stores_count = 0;

	// Init the Semi-hosting Profile Support 
	char htime_fname[64] = {0};
	sprintf(htime_fname, "hosttime_cpu-%02d.log", m_cpuindex);
	m_hosttime_instance = new hosttime(htime_fname);
	if(!m_hosttime_instance)
	{
		cerr << "Error Initializing Semi-hosting Profile Support" << endl;
		return;
	}

    SC_THREAD (kvm_cpu_thread);
}

// A thread used to simulate the kvm processor
void kvm_cpu_wrapper::kvm_cpu_thread ()
{
	int cpu_status = KVM_CPU_OK;

	if(m_node_id)		// For Non-Boot CPUs
	{	
		do
		{
			wait (100, SC_NS, m_ev_runnable);
		} while(!kvm_cpu_init_received(m_kvm_cpu_instance));

		do
		{
			wait (100, SC_NS, m_ev_runnable);
		} while(!kvm_cpu_sipi_received(m_kvm_cpu_instance));
		
		// Now Wait Until You get the Green Flag
		wait (m_ev_runnable);
	}

	kvm_cpu_reset(m_kvm_cpu_instance);

	while(1)
	{
		m_parent->kvm_cpu_unblock(m_node_id);
		KVM_CPUS_STATUS();
		
		cpu_status = kvm_cpu_execute(m_kvm_cpu_instance);

		switch(cpu_status)
		{
			case KVM_CPU_RETRY:
				m_parent->kvm_cpu_block(m_node_id);
				KVM_CPUS_STATUS();
				// Ideally we should block for a minimum time; so the kicked cpu starts and 
				// we enter KVM after the other cpu has executed atleast once.
				do
				{
					wait (100, SC_NS, m_ev_runnable);
				} while(!kvm_cpu_is_runnable(m_kvm_cpu_instance));
			break;

			case KVM_CPU_BLOCK_AFTER_KICK:
				m_parent->kvm_cpu_block(m_node_id);
				do
				{
					wait (100, SC_NS, m_ev_runnable);
				} while(!kvm_cpu_is_runnable(m_kvm_cpu_instance));
			break;

			case KVM_CPU_PANIC:
				cout << "KVM PANICKED ... !!!" << endl;
			case KVM_CPU_SHUTDOWN:
				cout << "KVM Shuting Down!" << endl;
			break;
		}
	}

	return;
}

void kvm_cpu_wrapper::kvm_cpus_status()
{
	cout << "[KVM-CPU-" << m_node_id << "]: ";
	m_parent->kvm_cpus_status(); 
}

kvm_cpu_wrapper::~kvm_cpu_wrapper ()
{
    if (m_rqs)
	{
        delete m_rqs;
		m_rqs = NULL;
	}

	if(m_hosttime_instance)
	{
		delete m_hosttime_instance;
		m_hosttime_instance = NULL;
	}
}

void kvm_cpu_wrapper::set_unblocking_write (bool val)
{
    m_unblocking_write = val;
}

void kvm_cpu_wrapper::rcv_rsp(unsigned char tid, unsigned char *data, bool bErr, bool bWrite)
{

    kvm_wrapper_request            *localrq;

    localrq = m_rqs->GetRequestByTid (tid);
    if (localrq == NULL)
    {
		PRINTF("kvm_cpu_wrapper[%d] Error: %s received a response for an unknown TID = %d\n",
				m_cpuindex, name(), (unsigned int) tid);
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

uint64_t kvm_cpu_wrapper::read (uint64_t addr, int nbytes, int bIO)
{
    unsigned char           adata[8];
    uint64_t                ret;
    int                     i;
    unsigned char           tid;
    kvm_wrapper_request   *localrq;

    if (m_unblocking_write)
        localrq = m_rqs->GetNewRequest (1);
    else
        localrq = m_rqs->GetNewRequest (0);

    if (localrq == NULL)
        return -2;

    localrq->bWrite = 0;
    tid = localrq->tid;

    PRINTF("kvm_cpu_wrapper[%d]: Read tid = %d, addr = 0x%08x, nbytes = %d\n", 
			m_cpuindex, tid, (uint32_t) addr, nbytes);
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
    kvm_wrapper_request            *localrq;

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

    PRINTF("kvm_cpu_wrapper[%d]: Write tid = %d, addr = 0x%08x, nbytes = %d, p = 0x%p\n",
			m_cpuindex, tid, (uint32_t) addr, nbytes, localrq);
    send_req(tid, addr, (unsigned char *)data, nbytes, 1);

    if (!m_unblocking_write)
    {
        if (!localrq->bDone)
            wait (localrq->evDone);
        m_rqs->FreeRequest (localrq);
    }

    wait(20, SC_US);

//	cout << name() << "MMH:SC Time = " << sc_time_stamp() << endl;
    return;
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
    static uint64_t cpu_instrs_count_prev = 0, cpu_cycles_count_prev = 0,
                    cpu_loads_count_prev = 0, cpu_stores_count_prev = 0;
    uint32_t value = (uint32_t) *data;

    std::cout << "KVM STATS Delta[" << value << "]:"
              << " cpu_instrs_count = " << std::setw(12) 
              << (m_cpu_instrs_count - cpu_instrs_count_prev)
              << " cpu_cycles_count = " << std::setw(10) 
              << (m_cpu_cycles_count - cpu_cycles_count_prev)
              << " cpu_loads_count = "  << std::setw(10) 
              << (m_cpu_loads_count  - cpu_loads_count_prev )
              << " cpu_stores_count = " << std::setw(10) 
              << (m_cpu_stores_count - cpu_stores_count_prev) << std::endl;

    cpu_instrs_count_prev = m_cpu_instrs_count;
    cpu_cycles_count_prev = m_cpu_cycles_count;
    cpu_loads_count_prev  = m_cpu_loads_count;
    cpu_stores_count_prev = m_cpu_stores_count;

    if(value == 0){
        std::cout << "Exiting on Application Request ..." << std::endl;
        exit(0);
    }
}

void kvm_cpu_wrapper::notify_runnable_event()
{
	cout << "Notifying Runnable Event for CPU-" << m_node_id
         << " Current SC Time = " << sc_time_stamp() << endl;
	m_ev_runnable.notify();
}

void kvm_cpu_wrapper::wait_until_runnable()
{
	cout << "Calling Wait for Runnable Event on CPU-" << m_node_id 
         << " Current SC Time = " << sc_time_stamp() << endl;
	m_parent->kvm_cpu_block(m_node_id);
	KVM_CPUS_STATUS();
	wait(m_ev_runnable);
	m_parent->kvm_cpu_unblock(m_node_id);
	KVM_CPUS_STATUS();
}

void kvm_cpu_wrapper::wait_zero_time()
{
	cout << "Calling Wait for Zero Time on CPU-" << m_node_id 
         << " Current SC Time = " << sc_time_stamp() << endl;
	wait(SC_ZERO_TIME);
}

void kvm_cpu_wrapper::wait_us_time(int us)
{
	wait(us, SC_US);
}

// This principally called as a result of CPU_TEST_AND_SET execution by a processor.
void kvm_cpu_wrapper::wait_until_kick_or_timeout()
{
	// TODO: Instead to doing this; 
	// Advance the Simulation Time to the Minimum of other processors.
	wait(1000, SC_NS, m_ev_runnable);
}

extern "C"
{
	void systemc_notify_runnable_event(kvm_cpu_wrapper_t *_this)
	{
		_this->notify_runnable_event();
	}

	void systemc_wait_until_runnable(kvm_cpu_wrapper_t *_this)
	{
		_this->wait_until_runnable();
	}

	void systemc_wait_zero_time(kvm_cpu_wrapper_t *_this)
	{
		_this->wait_zero_time();
	}

	void systemc_wait_until_kick_or_timeout(kvm_cpu_wrapper_t *_this)
	{
		_this->wait_until_kick_or_timeout();
	}

	int systemc_running_cpu_count(kvm_cpu_wrapper_t *_this)
	{
		return(_this->m_parent->m_running_count);	
	}

	/*	
	void kvm_lock_run_mutex(kvm_cpu_wrapper_t *_this)
	{
		_this->m_parent->m_kvm_run_mutex.lock();	
	}

	void kvm_unlock_run_mutex(kvm_cpu_wrapper_t *_this)
	{
		_this->m_parent->m_kvm_run_mutex.unlock();	
	}
	*/

	void systemc_wait_us(kvm_cpu_wrapper_t *_this, int us)
	{
		_this->wait_us_time(us);
	}

    uint64_t
    systemc_mmio_read (kvm_cpu_wrapper_t *_this, uint64_t addr,
        int nbytes, unsigned int *ns, int bIO)
    {
        uint64_t ret;

        ret = _this->read (addr, nbytes, bIO);
        return ret;
    }

    void
    systemc_mmio_write (kvm_cpu_wrapper_t *_this, uint64_t addr,
        unsigned char *data, int nbytes, unsigned int *ns, int bIO)
    {
		// printf("%s: addr = 0x%x, data=%c, nbytes=%d\n", __func__, (uint32_t)addr, *data, nbytes);
        _this->write (addr, data, nbytes, bIO);
    }

#if 0
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
#endif

    void
    semihosting_profile_function(kvm_cpu_wrapper_t *_this, uint32_t value)
    {
        _this->m_hosttime_instance->hosttime_handler(value);
    }

#ifdef USE_ANNOT_BUFF
#ifdef USE_EXECUTION_SPY
    void
    systemc_annotate_function(kvm_cpu_wrapper_t *_this, void *vm_addr, void *ptr)
    {
        _this->annotate((void *)vm_addr, (db_buffer_desc_t *) ptr);
    }
#else	/* USE_EXECUTION_SPY */
    void
    systemc_annotate_function(kvm_cpu_wrapper_t *_this, void *vm_addr, void *ptr)
    {
        db_buffer_desc_t *pbuff_desc = (db_buffer_desc_t *) ptr;
        annotation_db_t *pdb = NULL;
        uint32_t buffer_cycles = 0;

        //printf("%s: annotate buffer[%2d]: Start = %4d, End = %4d\n", _this->name(),
        //        pbuff_desc->BufferID, pbuff_desc->StartIndex, pbuff_desc->EndIndex);

        while(pbuff_desc->StartIndex != pbuff_desc->EndIndex)
        {
            /* get the guest physical address depending the host machine is x86 or x86_64 */
            intptr_t guest_db_addr;
#ifndef __i386__
            guest_db_addr = ((intptr_t)(pbuff_desc->Buffer[pbuff_desc->StartIndex].pdb) >> 32);
#else
            guest_db_addr = (intptr_t)(pbuff_desc->Buffer[pbuff_desc->StartIndex].pdb);
#endif
            // Get pointer to the annotation db;
            pdb = (annotation_db_t *)((intptr_t)vm_addr + guest_db_addr);

            buffer_cycles += pdb->CycleCount;

            UPDATE_CPU_STATS(pdb);
            pbuff_desc->StartIndex = (pbuff_desc->StartIndex + 1) % pbuff_desc->Capacity;
        }

        wait(buffer_cycles, SC_NS);
    } 
#endif /* USE_EXECUTION_SPY */
#else  /* USE_ANNOT_BUFF */
    void
    systemc_annotate_function(kvm_cpu_wrapper_t *_this, void *vm_addr, void *ptr)
    {
        annotation_db_t *pdb = (annotation_db_t *) ptr;
		
		PRINTF("%s: annotate function db=%x\n", _this->name(), (unsigned int) ptr);
        wait(pdb->CycleCount, SC_NS);

		UPDATE_CPU_STATS(pdb);
    }
#endif /* USE_ANNOT_BUFF */
}



#if 0
printf("BufferID = %d, StartIndex = %d,  EndIndex = %d, Capacity = %d\n",
    pbuff_desc->BufferID, pbuff_desc->StartIndex,
    pbuff_desc->EndIndex, pbuff_desc->Capacity);

printf("@db: 0x%08x, Type: %d, CC = %d, FuncAddr: 0x%08x\n",
       (uint32_t)pbuff_desc->Buffer[pbuff_desc->StartIndex], pdb->Type, pdb->CycleCount, pdb->FuncAddr);

db_buffer_desc_t *pbuff_desc = (db_buffer_desc_t *) pdesc;
uint32_t db_count = 0;

if(pbuff_desc->EndIndex > pbuff_desc->StartIndex)
    db_count = pbuff_desc->EndIndex - pbuff_desc->StartIndex;
else
    db_count = pbuff_desc->Capacity - (pbuff_desc->StartIndex - pbuff_desc->EndIndex);

printf("BufferID [%d] %5d --> %5d (Count = %d)\n",
        pbuff_desc->BufferID, pbuff_desc->StartIndex,
        pbuff_desc->EndIndex, db_count);
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
