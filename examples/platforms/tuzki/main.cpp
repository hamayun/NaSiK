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

#include <systemc.h>
#include <abstract_noc.h>
#include <interconnect_master.h>

#include <kvm_cpu_wrapper.h>
#include <interconnect.h>
#include <tg_device.h>
#include <sl_tg_device.h>
#include <timer_device.h>
#include <sem_device.h>
#include <sl_tty_device.h>
#include <framebuffer_device.h>
#include <sl_block_device.h>
#include <mem_device.h>


using namespace std;

extern "C" {

#include <soc_kvm.h>
    extern soc_kvm_data soc_kvm_init_data;
    int soc_kvm_init(char *bootstrap, char *elf_file);

    void *p_kvm_cpu_adaptor[MAX_VCPUS] = {NULL};
}


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
interconnect        *onoc = NULL;
slave_device        *slaves[50];
int                 nslaves = 0;

int sc_main (int argc, char ** argv)
{
    int         i, retVal;
    int         kvm_nb_cpu;

    if(argc < 3) usage_and_exit(argv[0]);
    signal(SIGINT,simulation_stop);

    fb_reset_t    fb_res_stat;
    {
      fb_res_stat.fb_start  = 1;
      fb_res_stat.fb_w      = 256;
      fb_res_stat.fb_h      = 144;
      fb_res_stat.fb_mode   = YV16;
      fb_res_stat.fb_display_on_wrap = 1;
    }

    /* Initialize the kvm. */
    retVal = soc_kvm_init(argv[1], argv[2]);
    if(retVal)
    {
        std::cout << "soc_kvm_init failed, retVal = " << retVal << std::endl;
        return(EXIT_FAILURE);
    }

    kvm_nb_cpu = soc_kvm_init_data.ncpus;

    /* Initialize the adaptor, also pass the application name to execute*/
    kvm_cpu_wrapper_t * kvm_cpu_adaptor[MAX_VCPUS] = {NULL};
    for (i = 0; i < kvm_nb_cpu; i++)
    {
        std::stringstream cpu_id; cpu_id << i;
        string kvm_cpu_name = "kvm_cpu_" + cpu_id.str();
        kvm_cpu_adaptor[i] = new kvm_cpu_wrapper_t(kvm_cpu_name.c_str(), argv[2], (uintptr_t)soc_kvm_init_data.vm_mem, i);
        p_kvm_cpu_adaptor[i] = (void *) kvm_cpu_adaptor[i];

        std::cout << "p_kvm_cpu_adaptor[" << i << "] = " << p_kvm_cpu_adaptor[i] << std::endl;
    }

    sl_block_device   *bl   = new sl_block_device("block", 1, "input_data", 1);
    sl_block_device   *bl2  = new sl_block_device("block2", 2, "input_data", 1);
    sl_block_device   *bl3  = new sl_block_device("block3", 3, "output_data", 1);

    mem_device         *ram = new mem_device("ram", soc_kvm_init_data.memory_size, soc_kvm_init_data.vm_mem);
    mem_device  *shared_ram = new mem_device("shared_ram", 0x10000);
    sl_tty_device     *tty0 = new sl_tty_device ("tty1", 1);
    sl_tg_device        *tg = new sl_tg_device ("tg", "fdaccess.0.0");
    fb_device           *fb = new fb_device("framebuffer", 0, &fb_res_stat);
    sem_device         *sem = new sem_device("sem", 0x100000);

    slaves[nslaves++] = ram;			 // 0	0x00000000 - 0x08000000
    slaves[nslaves++] = shared_ram;		 // 1	0xAF000000 - 0xAFF00000
    slaves[nslaves++] = tty0;			 // 2	0xC0000000 - 0xC0000040
    slaves[nslaves++] = tg;			 // 3	0xC3000000 - 0xC3001000
    slaves[nslaves++] = fb->get_slave();         // 4	0xC4000000 - 0xC4100000 /* Important: In Application ldscript the base address should be 0XC4001000 */
    slaves[nslaves++] = sem;			 // 5	0xC5000000 - 0xC5100000
    slaves[nslaves++] = bl->get_slave();         // 6	0xC6000000 - 0xC6100000
    slaves[nslaves++] = bl2->get_slave();        // 7	0xC6500000 - 0xC6600000
    slaves[nslaves++] = bl3->get_slave();        // 8	0xC6A00000 - 0xC6B00000

    timer_device	*timers[3 + kvm_nb_cpu];
    int       ntimers = sizeof (timers) / sizeof (timer_device *);   // Why we divide by pointer size here ???
    for (i = 0; i < ntimers; i ++)
    {
        char		buf[20];
        sprintf(buf, "timer_%d", i);
        timers[i] = new timer_device (buf);
        slaves[nslaves++] = timers[i]; // 7 + i  from 0xC1000000
    }
    int                     no_irqs = ntimers + 4;
    sc_signal<bool> *wires_irq_qemu = new sc_signal<bool>[no_irqs];

    for (i = 0; i < ntimers; i++)
        timers[i]->irq (wires_irq_qemu[i]);

    bl->irq (wires_irq_qemu[no_irqs-4]);
    bl2->irq (wires_irq_qemu[no_irqs-3]);
    bl3->irq (wires_irq_qemu[no_irqs-2]);
    fb->irq (wires_irq_qemu[no_irqs-1]);

    //interconnect
    onoc = new interconnect ("interconnect", (kvm_nb_cpu + 4), nslaves);
    for (i = 0; i < nslaves; i++)
        onoc->connect_slave_64 (i, slaves[i]->get_port, slaves[i]->put_port);

    for (i = 0; i < kvm_nb_cpu; i++)
        onoc->connect_master_64 (i, kvm_cpu_adaptor[i]->put_port, kvm_cpu_adaptor[i]->get_port);

    // connect block device
    onoc->connect_master_64(i,
                            bl->get_master()->put_port,
                            bl->get_master()->get_port);
    onoc->connect_master_64(i+1,
                            bl2->get_master()->put_port,
                            bl2->get_master()->get_port);
    onoc->connect_master_64(i+2,
                            bl3->get_master()->put_port,
                            bl3->get_master()->get_port);
    onoc->connect_master_64(i+3,
                            fb->get_master()->put_port,
                            fb->get_master()->get_port);

    sc_start();
    return (EXIT_SUCCESS);
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
