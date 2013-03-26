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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sstream>

#include <system_init.h>
#include <systemc.h>
#include <abstract_noc.h>
#include <interconnect_master.h>
#include <qemu-0.9.1/qemu_systemc.h>

#include <qemu_wrapper.h>
#include <qemu_cpu_wrapper.h>
#include <qemu_wrapper_cts.h>
#include <qemu_imported.h>

#include <kvm_wrapper.h>
#include <interconnect.h>
#include <tg_device.h>
#include <sl_tg_device.h>
#include <timer_device.h>
#include <sem_device.h>
#include <sl_tty_device.h>
#include <framebuffer_device.h>
#include <mem_device.h>
#include <channel_spy.h>

using namespace std;

#ifndef O_BINARY
#define O_BINARY 0
#endif

unsigned long no_frames_to_simulate = 0;

void simulation_stop(int signo)
{
	cout << "************************************************************" << endl;
	cout << "              USER REQUEST END OF SIMULATION                " << endl;
	cout << " Simulation time : " << sc_time_stamp() << endl;
	cout << "************************************************************" << endl;
	sc_stop();
	exit(0);
}

void usage_and_exit(char * name)
{
    cerr << "usage : " << name << "  <bootstrap_file> <application_path>" << endl;
    exit(EXIT_FAILURE);
}

mem_device          *shared_ram = NULL, *ram = NULL;
mem_device          *qemu_ram = NULL;
interconnect        *onoc = NULL;
slave_device        *slaves[50];
int                 nslaves = 0;
init_struct         is;

extern "C" {
        extern int      kvm_debug_port;
}

int sc_main (int argc, char ** argv)
{
    int i = 0, j = 0;

	bool trace_on = false;
	char *			kvm_argv[10];
	int				kvm_argc;

	uint32_t slave_trace_mask = 0xFFFFFE2B;		// If slave bit is set to 1; trace for that slave is masked.
	bool trace_non_cpu_masters = false;			// true: Trace All Masters; false: CPUs only.
	channel_spy_master **master_spies = {NULL};
	channel_spy_slave  **slave_spies  = {NULL};

    // if(argc < 3) usage_and_exit(argv[0]);
    // kvm_debug_port = 1234;

    signal(SIGINT, simulation_stop);

    memset (&is, 0, sizeof (init_struct));
    is.cpu_family = "arm";
    is.cpu_model = NULL;
    is.kernel_filename = NULL;
    is.initrd_filename = NULL;
    is.no_cpus = 1;
    is.ramsize = 128 * 1024 * 1024;
    parse_cmdline (argc, argv, &is, kvm_argc, kvm_argv);
    if (check_init (&is) != 0)
        return 1;

	char * boot_loader = (char *) kvm_argv[1];
	char * kernel = (char *) kvm_argv[2];

    fb_reset_t    fb_res_stat;
    {
      fb_res_stat.fb_start  = 1;
      fb_res_stat.fb_w      = 256;
      fb_res_stat.fb_h      = 144;
      fb_res_stat.fb_mode   = YV16;
      fb_res_stat.fb_display_on_wrap = 1;
    }

    /* Initialize the KVM Processor Wrapper and specify the number of cores here.*/
    int         kvm_num_cpus = 1;
    uint64_t    kvm_ram_size = 128 /* Size in MBs */;
    uintptr_t   kvm_userspace_mem_addr = 0;
	int         non_cpu_masters = 1;
	int 		total_masters = kvm_num_cpus + non_cpu_masters;
	int 		total_slaves = kvm_num_cpus + is.no_cpus + 8;	/* One Timer/CPU + Slave Devices */

	int32_t     intr_mask_size = total_masters;
	uint32_t   *intr_cpu_mask = new uint32_t [intr_mask_size];
    memset(intr_cpu_mask, 0x0, sizeof(uint32_t) * total_masters);

    kvm_wrapper_t kvm_wrapper ("KVM-0", 0, total_masters, intr_cpu_mask,
                               kvm_num_cpus, kvm_ram_size, kernel, boot_loader,
                               & kvm_userspace_mem_addr);

    qemu_ram   = new mem_device ("dynamic", is.ramsize + 0x1000);
    mem_device         *ram = new mem_device("ram", kvm_ram_size*(1024*1024), (unsigned char*) kvm_userspace_mem_addr);
    mem_device  *shared_ram = new mem_device("shared_ram", 0x10000);
    sl_tty_device     *tty0 = new sl_tty_device ("tty-kvm", kvm_num_cpus);
    sl_tty_device     *tty1 = new sl_tty_device ("tty-qemu", 1);
    sl_tg_device        *tg = new sl_tg_device ("tg", "fdaccess.0.0");
    fb_device           *fb = new fb_device("framebuffer", kvm_num_cpus + is.no_cpus + 3, &fb_res_stat);
    sem_device         *sem = new sem_device("sem", 0x100000);

	if(trace_on)
	{
		char master_name[20];
		master_spies = new channel_spy_master* [total_masters];
		for(i = 0; i < kvm_num_cpus; i++)
		{
			sprintf(master_name, "CPU-%02d", i);
			master_spies[i] = new channel_spy_master(master_name);
		}
		// Non-CPU Masters ...
		master_spies[i++] = new channel_spy_master("FB-MASTER");

		// Slave Spies
		slave_spies = new channel_spy_slave* [total_slaves];

		slave_spies[nslaves] = new channel_spy_slave("RAM");
	    slave_spies[nslaves]->connect_slave(nslaves, ram->get_port, ram->put_port);
		nslaves++;
		slave_spies[nslaves] = new channel_spy_slave("SHARED-RAM");
	    slave_spies[nslaves]->connect_slave(nslaves, shared_ram->get_port, shared_ram->put_port);
		nslaves++;
		slave_spies[nslaves] = new channel_spy_slave("TTY");
	    slave_spies[nslaves]->connect_slave(nslaves, tty0->get_port, tty0->put_port);
		nslaves++;
		slave_spies[nslaves] = new channel_spy_slave("TR-GEN");
	    slave_spies[nslaves]->connect_slave(nslaves, tg->get_port, tg->put_port);
		nslaves++;
		slave_spies[nslaves] = new channel_spy_slave("FB-SLAVE");
	    slave_spies[nslaves]->connect_slave(nslaves, fb->get_slave()->get_port, fb->get_slave()->put_port);
		nslaves++;
		slave_spies[nslaves] = new channel_spy_slave("SEM");
	    slave_spies[nslaves]->connect_slave(nslaves, sem->get_port, sem->put_port);
		nslaves++;
	}
	else
	{
	    slaves[nslaves++] = ram;                    // 0	0x00000000 - 0x08000000
        slaves[nslaves++] = qemu_ram;               // 1	0x00000000 - 0x08000000
    	slaves[nslaves++] = shared_ram;             // 2	0xAF000000 - 0xAFF00000
        /* 16 bytes for Each TTY; Should be enough for 32 Instances of TTYs */
	    slaves[nslaves++] = tty0;                   // 3	0xC0000000 - 0xC0000200 
	    slaves[nslaves++] = tty1;                   // 4	0xC0000200 - 0xC0000400
    	slaves[nslaves++] = tg;                     // 5	0xC3000000 - 0xC3001000
        /* Important: In Application ldscript the base address should be 0XC4001000 */
    	slaves[nslaves++] = fb->get_slave();        // 6	0xC4000000 - 0xC4100000 
	    slaves[nslaves++] = sem;                    // 7	0xC5000000 - 0xC5100000
	}

	// Create as many timers as many CPUs
    timer_device	* timers_kvm[kvm_num_cpus];
    int       ntimers_kvm = sizeof (timers_kvm) / sizeof (timer_device *);
    for (i = 0; i < ntimers_kvm; i ++)
    {
        char		buf[20];
        sprintf(buf, "timer_kvm-%d", i);
        timers_kvm[i] = new timer_device (buf);

		if(trace_on)
		{
        	sprintf(buf, "TIMER-%02d", i);
			slave_spies[nslaves] = new channel_spy_slave(buf);
		    slave_spies[nslaves]->connect_slave(nslaves, timers_kvm[i]->get_port, timers_kvm[i]->put_port);
			nslaves++;		
		}
		else
		{
	        slaves[nslaves++] = timers_kvm[i]; // 11 + i  from 0xC1000000
		}
    }

    int num_irqs_kvm = kvm_num_cpus + non_cpu_masters; /* 1 FB */
    sc_signal<bool> * kvm_irq_wires = new sc_signal<bool>[num_irqs_kvm];

	// Connect IRQ ports to Timers
    for (i = 0; i < ntimers_kvm; i++)
        timers_kvm[i]->irq (kvm_irq_wires[i]);

	// Connect IRQ ports to Non-CPU Masters
    fb->irq  (kvm_irq_wires[num_irqs_kvm - 1]);

	timer_device	*timers_qemu[1];
	int				ntimers_qemu = sizeof (timers_qemu) / sizeof (timer_device *);
	for (i = 0; i < ntimers_qemu; i ++)
	{
		char		buf[20];
		sprintf(buf, "timer_qemu-%d", i);
		timers_qemu[i] = new timer_device (buf);
		slaves[nslaves++] = timers_qemu[i]; // 11 + i  from 0xC1000000
	} 

	int							num_irqs_qemu = 1;
	int 						int_cpu_mask [] = {1};
    sc_signal<bool>             *wires_irq_qemu = new sc_signal<bool>[num_irqs_qemu];
    timers_qemu[0]->irq (wires_irq_qemu[0]);

    // Create the Interconnect Component
    onoc = new interconnect ("interconnect", total_masters + is.no_cpus, nslaves);

	// Connect All Slaves to Interconnect
    for (i = 0; i < nslaves; i++)
	{
		if(trace_on)
			onoc->connect_slave_64 (i, slave_spies[i]->get_req_port, slave_spies[i]->put_rsp_port);
		else
		    onoc->connect_slave_64 (i, slaves[i]->get_port, slaves[i]->put_port);
	}

    arm_load_kernel (&is);

    // Masters of QEMU
    qemu_wrapper                qemu1 ("QEMU1", 2, num_irqs_qemu, int_cpu_mask, is.no_cpus, 2, 
                                    is.cpu_family, is.cpu_model, is.ramsize);
    qemu1.add_map (0xA0000000, 0x10000000); // (base address, size)
    qemu1.set_base_address (QEMU_ADDR_BASE);
    for (j = 0; j < num_irqs_qemu; j++)
        qemu1.interrupt_ports[j] (wires_irq_qemu[j]);

    // Connect IRQs to KVM Wrapper
    for(i = 0; i < num_irqs_kvm; i++)
        kvm_wrapper.interrupt_ports[i] (kvm_irq_wires[i]);

	// Connect All CPU Masters
    for(i = 0; i < kvm_num_cpus; i++)
	{
		if(trace_on)
		{
			master_spies[i]->connect_master(i, kvm_wrapper.get_cpu(i)->put_port, kvm_wrapper.get_cpu(i)->get_port);
			onoc->connect_master_64 (i, master_spies[i]->put_req_port, master_spies[i]->get_rsp_port);
		}
		else
        	onoc->connect_master_64 (i, kvm_wrapper.get_cpu(i)->put_port, kvm_wrapper.get_cpu(i)->get_port);
	}

	// Initialize the Debugger, if required.
    if(kvm_debug_port)
    {
	    kvm_wrapper.m_kvm_import_export.exp_gdb_srv_start_and_wait(kvm_wrapper.m_kvm_instance, kvm_debug_port);
    }

	if(trace_on)
	{
		master_spies[i]->connect_master(i, fb->get_master()->put_port, fb->get_master()->get_port);
		onoc->connect_master_64 (i, master_spies[i]->put_req_port, master_spies[i]->get_rsp_port);
		i++;
	}
	else
	{
	    // Connect Frame Buffer
    	onoc->connect_master_64(i, fb->get_master()->put_port, fb->get_master()->get_port);
        i++;
	}

    // Connect QEMU Master
    for (j = 0; j < is.no_cpus; j++)
        onoc->connect_master_64 (i+j, qemu1.get_cpu (j)->put_port, qemu1.get_cpu (j)->get_port);

	// Start the Simulation
    cout << "Starting SystemC Hardware ... " << endl;

    sc_trace_file * trace_file = NULL;
    ofstream vcd_config_file;
    if (trace_on) {
        trace_file = sc_create_vcd_trace_file("waveforms");
        vcd_config_file.open("waveforms.sav");

		if(trace_non_cpu_masters)	
			cout << "Tracing Enabled for All Master Devices " << endl;
		else
			cout << "Tracing Enabled for CPU Master Only" << endl;

		for(i = 0; i < total_masters + is.no_cpus; i++)
		{	
			if(i >= kvm_num_cpus && !trace_non_cpu_masters)
				continue;
	        channel_spy_trace(trace_file, *master_spies[i], &vcd_config_file);
		}

		for(i = 0; i < total_slaves; i++)
		{
			if(~slave_trace_mask & (1 << i)) 
			{
				cout << "Tracing Enabled for Slave ID " << i << endl;
		        channel_spy_trace(trace_file, *slave_spies[i], &vcd_config_file);
			}
		}

        vcd_config_file.close();
    }

    sc_start();

    if (trace_on)
    {
        sc_close_vcd_trace_file(trace_file);
    }
    return (EXIT_SUCCESS);
}

int systemc_load_image (const char *file, unsigned long ofs)
{
    if (file == NULL)
        return -1;

    int     fd, img_size, size;
    fd = open (file, O_RDONLY | O_BINARY);
    if (fd < 0)
        return -1;

    img_size = lseek (fd, 0, SEEK_END);
    if (img_size + ofs >  qemu_ram->get_size ())
    {
        printf ("%s - RAM size < %s size + %lx\n", __FUNCTION__, file, ofs);
        close(fd);
        exit (1);
    }

    lseek (fd, 0, SEEK_SET);
    size = img_size;
    if (read (fd, qemu_ram->get_mem () + ofs, size) != size)
    {
        printf ("Error reading file (%s) in function %s.\n", file, __FUNCTION__);
        close(fd);
        exit (1);
    }

    close (fd);

    return img_size;
}

unsigned char* systemc_get_sram_mem_addr ()
{
    return qemu_ram->get_mem ();
}

extern "C"
{

unsigned char   dummy_for_invalid_address[256];
struct mem_exclusive_t {unsigned long addr; int cpu;} mem_exclusive[100];
int no_mem_exclusive = 0;

void memory_mark_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
            break;
    
    if (i >= no_mem_exclusive)
    {
        mem_exclusive[no_mem_exclusive].addr = addr;
        mem_exclusive[no_mem_exclusive].cpu = cpu;
        no_mem_exclusive++;

#if 0 // Removed by Hao
        if (no_mem_exclusive > is.no_cpus)
        {
            printf ("Warning: number of elements in the exclusive list (%d) > cpus (%d) (list: ",
                no_mem_exclusive, is.no_cpus);
            for (i = 0; i < no_mem_exclusive; i++)
                printf ("%lx ", mem_exclusive[i].addr);
            printf (")\n");
            if (is.gdb_port > 0)
                kill (0, SIGINT);

        }
#endif
    }
}

int memory_test_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
            return (cpu != mem_exclusive[i].cpu);
    
    return 1;
}

void memory_clear_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
        {
            for (; i < no_mem_exclusive - 1; i++)
            {
                mem_exclusive[i].addr = mem_exclusive[i + 1].addr;
                mem_exclusive[i].cpu = mem_exclusive[i + 1].cpu;
            }
            
            no_mem_exclusive--;
            return;
        }

    printf ("Warning in %s: cpu %d not in the exclusive list: ",
        __FUNCTION__, cpu);
    for (i = 0; i < no_mem_exclusive; i++)
        printf ("(%lx, %d) ", mem_exclusive[i].addr, mem_exclusive[i].cpu);
    printf ("\n");
#if 0
    if (is.gdb_port > 0)
        kill (0, SIGINT);
#endif
}

unsigned char   *systemc_get_mem_addr (qemu_cpu_wrapper_t *qw, unsigned long addr)
{
    int slave_id = onoc->get_master (qw->m_node_id)->get_slave_id_for_mem_addr (addr);
    if (slave_id == -1)
        return dummy_for_invalid_address;
    return slaves[slave_id]->get_mem () + addr;
}

}

# if 0
int systemc_load_image (const char *file, unsigned long ofs)
{
    if (file == NULL)
        return -1;

    int     fd, img_size, size;
    fd = open (file, O_RDONLY | O_BINARY);
    if (fd < 0)
        return -1;

    img_size = lseek (fd, 0, SEEK_END);
    if (img_size + ofs >  ram->get_size ())
    {
        printf ("%s - RAM size < %s size + %lx\n", __FUNCTION__, file, ofs);
        close(fd);
        exit (1);
    }

    lseek (fd, 0, SEEK_SET);
    size = img_size;
    if (read (fd, ram->get_mem () + ofs, size) != size)
    {
        printf ("Error reading file (%s) in function %s.\n", file, __FUNCTION__);
        close(fd);
        exit (1);
    }

    close (fd);

    return img_size;
}

unsigned char* systemc_get_sram_mem_addr ()
{
    return ram->get_mem ();
}

extern "C"
{

unsigned char   dummy_for_invalid_address[256];
struct mem_exclusive_t {unsigned long addr; int cpu;} mem_exclusive[100];
int no_mem_exclusive = 0;

void memory_mark_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
            break;

    if (i >= no_mem_exclusive)
    {
        mem_exclusive[no_mem_exclusive].addr = addr;
        mem_exclusive[no_mem_exclusive].cpu = cpu;
        no_mem_exclusive++;

#if 0 // Removed by Hao
        if (no_mem_exclusive > is.no_cpus)
        {
            printf ("Warning: number of elements in the exclusive list (%d) > cpus (%d) (list: ",
                no_mem_exclusive, is.no_cpus);
            for (i = 0; i < no_mem_exclusive; i++)
                printf ("%lx ", mem_exclusive[i].addr);
            printf (")\n");
            if (is.gdb_port > 0)
                kill (0, SIGINT);

        }
#endif
    }
}

int memory_test_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
            return (cpu != mem_exclusive[i].cpu);

    return 1;
}

void memory_clear_exclusive (int cpu, unsigned long addr)
{
    int             i;

    addr &= 0xFFFFFFFC;

    for (i = 0; i < no_mem_exclusive; i++)
        if (addr == mem_exclusive[i].addr)
        {
            for (; i < no_mem_exclusive - 1; i++)
            {
                mem_exclusive[i].addr = mem_exclusive[i + 1].addr;
                mem_exclusive[i].cpu = mem_exclusive[i + 1].cpu;
            }

            no_mem_exclusive--;
            return;
        }

    printf ("Warning in %s: cpu %d not in the exclusive list: ",
        __FUNCTION__, cpu);
    for (i = 0; i < no_mem_exclusive; i++)
        printf ("(%lx, %d) ", mem_exclusive[i].addr, mem_exclusive[i].cpu);
    printf ("\n");
#if 0
    if (is.gdb_port > 0)
        kill (0, SIGINT);
#endif
}

unsigned char   *systemc_get_mem_addr (qemu_cpu_wrapper_t *qw, unsigned long addr)
{
    int slave_id = onoc->get_master (qw->m_node_id)->get_slave_id_for_mem_addr (addr);
    if (slave_id == -1)
        return dummy_for_invalid_address;
    return slaves[slave_id]->get_mem () + addr;
}

}

#endif
