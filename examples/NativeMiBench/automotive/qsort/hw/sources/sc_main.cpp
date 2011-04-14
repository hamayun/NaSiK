/* Vendor :vendor*/
/* Libray :library*/
/* Name :DesignTest*/
/* Version :1.0*/

/*A simple example*/

#include <systemc.h>
#include <stdio.h>
#include "signal.h"

#include "annotation/execution_spy.h"
#include "eu/dna/proc/dna_eu.h"
#include "generic_noc.h"
#include "tty_pool.h"
#include "debug.h"
#include "linker/dna/dna_linker.h"
#include "memory.h"
#include "soclib_timer.h"
#include "bridgefs.h"
#include "it_gen.h"
#include "bridge.h"
#include "soclib_blockdevice.h"
#include "hosttime.h"

static bool quit = false;

extern int sc_configure(sc_simcontext * simcontext, char *filename);

void simulation_stop(int signo)
{
	cout << "************************************************************"
		<< endl;
	cout << "              USER REQUEST END OF SIMULATION                "
		<< endl;
	cout << " Simulation time : " << sc_time_stamp() << endl;;
	
	native::annotation::ExecutionSpy::close();
	sc_stop();

	quit = true;
}

void usage_and_exit(char * name)
{
	cerr << "usage : " << name << " <nb_cpu> <conf_file(s)> <native_options>" << endl;
	exit(EXIT_FAILURE);
}

using namespace std;
using namespace native;

int sc_main(int argc, char *argv[])
{
    unsigned int nb_cpu;
    sc_signal <bool> input_irq_bkdev0;
    sc_signal <bool> input_irq_bkdev1;
    sc_signal <bool> input_irq_bkdev2;

    signal(SIGINT,simulation_stop);

    argc = parseCommandLine(argv);

    if (argc < 4) {
        usage_and_exit(argv[0]);
    }

    /* The first argument must be the number of processors */
    for( unsigned int i = 0; argv[1][i] != '\0' ; i++)
    {
        if(isdigit(argv[1][i]) == false) usage_and_exit(argv[0]);
    }

    nb_cpu = atoi(argv[1]);

    dna_eu                      *eu_inst[16];
    tty_pool                    tty_pool_inst_0("tty_pool_inst_0");
    bridge                      bridge_noc_0_1("bridge_noc_0_1");
    generic_noc                 generic_noc_inst_0("generic_noc_inst_0");
    generic_noc                 generic_noc_inst_1("generic_noc_inst_1");
    dna_linker                  linker_dna("linker_dna");
    memory                      ram0("ram0");
    soclib_timer                timer("timer");
    it_gen                      it_generator("it_generator");
    soclib_blockdevice          blockdevice_0("blockdevice_0", 0, "input_small.dat", 1);
    soclib_blockdevice          blockdevice_1("blockdevice_1", 1, "dummy-disk", 1);
    soclib_blockdevice          blockdevice_2("blockdevice_2", 2, "output_small.txt", 1);
    hosttime                    hosttime_0("hosttime", "qsort_host_time.txt"); 

    for(unsigned int i = 0 ; i < nb_cpu ; i++)
    {
        char name[16];
        sprintf(name,"eu_inst_%d",i);
        eu_inst[i] = new dna_eu((char *)strdup(name));
    }

    // Slave connection
    // NoC 0
    //generic_noc_inst_0.p_io(tty_pool_inst_0.exp_io);
    generic_noc_inst_0.p_io(bridge_noc_0_1.exp_io);
    generic_noc_inst_0.p_io(ram0.exp_io);
    generic_noc_inst_0.p_io(timer.exp_io);
    generic_noc_inst_0.p_io(it_generator.exp_io);
    generic_noc_inst_0.p_io(hosttime_0.exp_io);

    // Connect Master and Slave Ports of BlockDevice to the NOC 0
    generic_noc_inst_0.p_io(blockdevice_0.exp_io);
    blockdevice_0.p_io(generic_noc_inst_0.exp_io);
    blockdevice_0.irq(input_irq_bkdev0);

    generic_noc_inst_0.p_io(blockdevice_1.exp_io);
    blockdevice_1.p_io(generic_noc_inst_0.exp_io);
    blockdevice_1.irq(input_irq_bkdev1);

    generic_noc_inst_0.p_io(blockdevice_2.exp_io);
    blockdevice_2.p_io(generic_noc_inst_0.exp_io);
    blockdevice_2.irq(input_irq_bkdev2);

    // NoC 1 for bridge
    bridge_noc_0_1.p_io(generic_noc_inst_1.exp_io);
    generic_noc_inst_1.p_io(tty_pool_inst_0.exp_io);

    for(unsigned int i = 0 ; i < nb_cpu ; i++)
        eu_inst[i]->p_io(generic_noc_inst_0.exp_io);

    for(unsigned int i = 0 ; i < nb_cpu ; i++)
        eu_inst[i]->p_linker_loader(linker_dna.exp_linker_loader);
    ram0.p_linker_loader(linker_dna.exp_linker_loader);

    // Connect linker to component
    linker_dna.p_linker(bridge_noc_0_1.exp_linker);
    linker_dna.p_linker(tty_pool_inst_0.exp_linker);
    linker_dna.p_linker(timer.exp_linker);
    linker_dna.p_linker(it_generator.exp_linker);
    linker_dna.p_linker(hosttime_0.exp_linker);
    linker_dna.p_linker(blockdevice_0.exp_linker);
    linker_dna.p_linker(blockdevice_1.exp_linker);
    linker_dna.p_linker(blockdevice_2.exp_linker);

    // interrupt connection
    it_generator.p_it(eu_inst[0]->exp_it);

    cout << "Load configuration files ..." << endl;
    for(int i = 3; i < argc ; i++)
    {
        cout << " + " << argv[i] << endl;
        if(sc_configure(sc_get_curr_simcontext(), argv[i]) == -1)
            return(EXIT_FAILURE);
    }

    sc_start();
    return (EXIT_SUCCESS);
}
