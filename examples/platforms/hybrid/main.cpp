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
#include <tty_serial_device.h>
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
	cerr << "usage : " << name << " " << endl;
	exit(EXIT_FAILURE);
}

mem_device          *qemu_ram = NULL;
mem_device          *kvm_ram = NULL;
mem_device          *shared_ram = NULL;
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

	char *			kvm_argv[10];
	int				kvm_argc;

	/*
	uint32_t slave_trace_mask = 0xFFFFFE2B;		// If slave bit is set to 1; trace for that slave is masked.
	bool trace_non_cpu_masters = false;			// true: Trace All Masters; false: CPUs only.
	channel_spy_master **master_spies = {NULL};
	channel_spy_slave  **slave_spies  = {NULL};
	*/

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

    /* Initialize the KVM Processor Wrapper and specify the number of cores here.*/
    int         kvm_num_cpus = 1;
    uint64_t    kvm_ram_size = 128 /* Size in MBs */;
    uintptr_t   kvm_userspace_mem_addr = 0;
	int 		total_masters = kvm_num_cpus;

	int32_t     intr_mask_size = total_masters;
	uint32_t   *intr_cpu_mask = new uint32_t [intr_mask_size];
    memset(intr_cpu_mask, 0x0, sizeof(uint32_t) * total_masters);

    kvm_wrapper_t kvm_wrapper ("KVM-0", 1, total_masters, intr_cpu_mask,
                               kvm_num_cpus, kvm_ram_size, kernel, boot_loader,
                               & kvm_userspace_mem_addr);
    
	//slaves
    qemu_ram = new mem_device ("dynamic", is.ramsize + 0x1000);
    kvm_ram 	= new mem_device("kvm_ram", kvm_ram_size*(1024*1024), (unsigned char*) kvm_userspace_mem_addr);
	shared_ram = new mem_device("shared_ram", 0x10000);
    tty_serial_device   *tty = new tty_serial_device ("tty");
    sl_tty_device   *tty0 = new sl_tty_device ("tty0", kvm_num_cpus);
	sl_tty_device   *tty1 = new sl_tty_device ("tty1", kvm_num_cpus);
	sl_tg_device		*tg = new sl_tg_device ("tg", "fdaccess.0.0");
    sl_tty_device     *dummy = new sl_tty_device ("dummy", kvm_num_cpus);
	sem_device		*sem = new sem_device("sem", 0x100000);

    slaves[nslaves++] = qemu_ram;		// 0
    slaves[nslaves++] = kvm_ram;		// 1
  	slaves[nslaves++] = shared_ram;		// 2
    slaves[nslaves++] = tty;			// 3
    slaves[nslaves++] = tty0;			// 4
    slaves[nslaves++] = tty1;			// 5
   	slaves[nslaves++] = tg;				// 6
	slaves[nslaves++] = dummy;			// 7
    slaves[nslaves++] = sem;			// 8

	timer_device	*timers_qemu[1];
	int				ntimers_qemu = sizeof (timers_qemu) / sizeof (timer_device *);
	for (i = 0; i < ntimers_qemu; i ++)
	{
		char		buf[20];
		sprintf(buf, "timer_qemu-%d", i);
		timers_qemu[i] = new timer_device (buf);
		slaves[nslaves++] = timers_qemu[i];
	} 

	int							num_irqs_qemu = 2;
	int 						int_cpu_mask [] = {1, 1};
    sc_signal<bool>             *wires_irq_qemu = new sc_signal<bool>[num_irqs_qemu];
    timers_qemu[0]->irq (wires_irq_qemu[0]);
    tty ->irq_line (wires_irq_qemu[1]);

	// Create as many timers as many CPUs
    timer_device	* timers_kvm[kvm_num_cpus];
    int       ntimers_kvm = sizeof (timers_kvm) / sizeof (timer_device *);
    for (i = 0; i < ntimers_kvm; i ++)
    {
        char		buf[20];
        sprintf(buf, "timer_kvm-%d", i);
        timers_kvm[i] = new timer_device (buf);
        slaves[nslaves++] = timers_kvm[i]; // 9 + i  from 0xC1000000
    }

    int num_irqs_kvm = kvm_num_cpus;
    sc_signal<bool> * kvm_irq_wires = new sc_signal<bool>[num_irqs_kvm];

	// Connect IRQ ports to Timers
    for (i = 0; i < ntimers_kvm; i++)
        timers_kvm[i]->irq (kvm_irq_wires[i]);

    // Create the Interconnect Component
    onoc = new interconnect ("interconnect", total_masters + is.no_cpus, nslaves);

	// Connect All Slaves to Interconnect
    for (i = 0; i < nslaves; i++)
		onoc->connect_slave_64 (i, slaves[i]->get_port, slaves[i]->put_port);

    arm_load_kernel (&is);

    // Masters of QEMU
    qemu_wrapper                qemu1 ("QEMU1", 0, num_irqs_qemu, int_cpu_mask, is.no_cpus, 0, 
                                    is.cpu_family, is.cpu_model, is.ramsize);
    qemu1.add_map (0xA0000000, 0x10000000); // (base address, size)
    qemu1.set_base_address (QEMU_ADDR_BASE);
    for (j = 0; j < num_irqs_qemu; j++)
        qemu1.interrupt_ports[j] (wires_irq_qemu[j]);

    // Connect QEMU Master
    for (j = 0; j < is.no_cpus; j++)
        onoc->connect_master_64 (j, qemu1.get_cpu (j)->put_port, qemu1.get_cpu (j)->get_port);
	
    // Connect IRQs to KVM Wrapper
    for(i = 0; i < num_irqs_kvm; i++)
        kvm_wrapper.interrupt_ports[i] (kvm_irq_wires[i]);

	// Connect All CPU Masters
    for(i = 0; i < kvm_num_cpus; i++)
    {
        	onoc->connect_master_64 (i+j, kvm_wrapper.get_cpu(i)->put_port, kvm_wrapper.get_cpu(i)->get_port);
    }

	// Initialize the Debugger, if required.
    if(kvm_debug_port)
    {
	    kvm_wrapper.m_kvm_import_export.exp_gdb_srv_start_and_wait(kvm_wrapper.m_kvm_instance, kvm_debug_port);
    }

	// Start the Simulation
    cout << "Starting SystemC Hardware ... " << endl;

    sc_start();
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


