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
#include "soclib_fb.h"
#include "soclib_timer.h"
#include "bridgefs.h"
#include "it_gen.h"
#include "bridge.h"

static bool quit = false;
unsigned long no_frames_to_simulate = 0;
void * PLATFORM_AICU_BASE = NULL; 
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
	cerr << "usage : " << name << " <nb_cpu> <trace_on|trace_off> <conf_file(s)> <native_options>" << endl;
	exit(EXIT_FAILURE);
}

using namespace std;
using namespace native;

int sc_main(int argc, char *argv[])
{
	unsigned int nb_cpu;
	bool         trace_on = false;

        /*
        PLATFORM_AICU_BASE = malloc(1024);
        if(!PLATFORM_AICU_BASE)
        {
            cerr << "Error Allocating Memory for PLATFORM_AICU_BASE" << std::endl;
            return (-1); 
        }
        */

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
	if(strcmp(argv[2],"trace_on") == 0)
	{
		trace_on = true;
		cerr << "Trace enabled\n";
	}

	dna_eu                      *eu_inst[16];
	tty_pool                    tty_pool_inst_0("tty_pool_inst_0");
	bridge						bridge_noc_0_1("bridge_noc_0_1");
	generic_noc                 generic_noc_inst_0("generic_noc_inst_0");
	generic_noc                 generic_noc_inst_1("generic_noc_inst_1");
	dna_linker                  linker_dna("linker_dna");
	memory                      ram0("ram0");
	soclib_fb                   fb("fb");
	soclib_timer                timer("timer");
	bridgefs                    fdaccess("fdaccess");
	it_gen                      it_generator("it_generator");

	for(unsigned int i = 0 ; i < nb_cpu ; i++)
	{
		char name[16];
		sprintf(name,"eu_inst_%d",i);
		eu_inst[i] = new dna_eu((char *)strdup(name));
	}

	// Devices Channels
	dev_channel    *eu_channel[16];
	dev_channel    *tty_pool_channel;
	dev_channel    *ram0_channel;
	dev_channel    *fb_channel;
	dev_channel    *timer_channel;
	dev_channel    *fdaccess_channel_mst;
	dev_channel    *fdaccess_channel_slv;
	dev_channel    *bridge_channel_mst;
	dev_channel    *bridge_channel_slv;
	dev_channel    *it_generator_channel;

	if(trace_on)
	{
		tty_pool_channel = new dev_channel("tty_pool_channel");
		ram0_channel = new dev_channel("ram0_channel");
		fb_channel = new dev_channel("fb_channel");
		timer_channel = new dev_channel("timer_channel");
		fdaccess_channel_mst = new dev_channel("fdaccess_channel_mst");
		fdaccess_channel_slv = new dev_channel("fdaccess_channel_slv");
		bridge_channel_mst = new dev_channel("bridge_channel_mst");
		bridge_channel_slv = new dev_channel("bridge_channel_slv");
		it_generator_channel = new dev_channel("it_generator_channel");

		for(unsigned int i = 0 ; i < nb_cpu ; i++)
		{
			char name[16];
			sprintf(name,"eu_channel_%d",i);
			eu_channel[i] = new dev_channel((char *)strdup(name));
    	}
	}

	// Slave connection
	if(trace_on)  {
		// NoC 0
		//generic_noc_inst_0.p_io(tty_pool_channel->exp_io);
		generic_noc_inst_0.p_io(bridge_channel_slv->exp_io);
		generic_noc_inst_0.p_io(ram0_channel->exp_io);
		generic_noc_inst_0.p_io(fb_channel->exp_io);
		generic_noc_inst_0.p_io(timer_channel->exp_io);
		generic_noc_inst_0.p_io(fdaccess_channel_slv->exp_io);
		generic_noc_inst_0.p_io(it_generator_channel->exp_io);

		// NoC 1 for bridge
		bridge_noc_0_1.p_io(bridge_channel_mst->exp_io);
		generic_noc_inst_1.p_io(tty_pool_channel->exp_io);
	}  else  {
		// NoC 0
		//generic_noc_inst_0.p_io(tty_pool_inst_0.exp_io);
		generic_noc_inst_0.p_io(bridge_noc_0_1.exp_io);
		generic_noc_inst_0.p_io(ram0.exp_io);
		generic_noc_inst_0.p_io(fb.exp_io);
		generic_noc_inst_0.p_io(timer.exp_io);
		generic_noc_inst_0.p_io(fdaccess.exp_io);
		generic_noc_inst_0.p_io(it_generator.exp_io);

		// NoC 1 for bridge
		bridge_noc_0_1.p_io(generic_noc_inst_1.exp_io);
		generic_noc_inst_1.p_io(tty_pool_inst_0.exp_io);
	}

	if(trace_on){
		// NoC 0
		//tty_pool_channel->p_io(tty_pool_inst_0.exp_io);
		bridge_channel_slv->p_io(bridge_noc_0_1.exp_io);
		ram0_channel->p_io(ram0.exp_io);
		fb_channel->p_io(fb.exp_io);
		timer_channel->p_io(timer.exp_io);
		fdaccess_channel_slv->p_io(fdaccess.exp_io);
		it_generator_channel->p_io(it_generator.exp_io);

		// NoC 1
		bridge_channel_mst->p_io(generic_noc_inst_1.exp_io);
		tty_pool_channel->p_io(tty_pool_inst_0.exp_io);
	}

	//
	if(trace_on){
		for(unsigned int i = 0 ; i < nb_cpu ; i++)
			eu_inst[i]->p_io(eu_channel[i]->exp_io);
		fdaccess.p_io(fdaccess_channel_mst->exp_io);
		for(unsigned int i = 0 ; i < nb_cpu ; i++)
			eu_channel[i]->p_io(generic_noc_inst_0.exp_io);
		fdaccess_channel_mst->p_io(generic_noc_inst_0.exp_io);
	} else {
		for(unsigned int i = 0 ; i < nb_cpu ; i++)
			eu_inst[i]->p_io(generic_noc_inst_0.exp_io);
		fdaccess.p_io(generic_noc_inst_0.exp_io);
	}

	for(unsigned int i = 0 ; i < nb_cpu ; i++)
		eu_inst[i]->p_linker_loader(linker_dna.exp_linker_loader);
	ram0.p_linker_loader(linker_dna.exp_linker_loader);

	// Connect linker to component
	linker_dna.p_linker(bridge_noc_0_1.exp_linker);
	linker_dna.p_linker(tty_pool_inst_0.exp_linker);
	linker_dna.p_linker(fb.exp_linker);
	linker_dna.p_linker(timer.exp_linker);
	linker_dna.p_linker(fdaccess.exp_linker);
	linker_dna.p_linker(it_generator.exp_linker);

	// interrupt connection
	it_generator.p_it(eu_inst[0]->exp_it);

	cout << "Load configuration files ..." << endl;
	for(int i = 3; i < argc ; i++)
	{
		cout << " + " << argv[i] << endl;
		if(sc_configure(sc_get_curr_simcontext(), argv[i]) == -1)
			return(EXIT_FAILURE);
	}

	sc_trace_file * trace_file;
	ofstream        vcd_config_file;
	if(trace_on) {
		trace_file = sc_create_vcd_trace_file("waveforms");
		vcd_config_file.open("waveforms.sav");

		for(unsigned int i = 0 ; i < nb_cpu ; i++)
			sc_trace(trace_file, *(eu_channel[i]), &vcd_config_file);
		sc_trace(trace_file, *tty_pool_channel, &vcd_config_file);
		sc_trace(trace_file, *ram0_channel, &vcd_config_file);
		sc_trace(trace_file, *fb_channel, &vcd_config_file);
		sc_trace(trace_file, *fdaccess_channel_slv, &vcd_config_file);
		sc_trace(trace_file, *fdaccess_channel_mst, &vcd_config_file);
		for(unsigned int i = 0 ; i < nb_cpu ; i++)
			sc_trace(trace_file, *(eu_inst[i]), &vcd_config_file);
		vcd_config_file.close();
	}
	sc_start();

	if(trace_on) {
		sc_close_vcd_trace_file(trace_file);
	}

        /*
        if(PLATFORM_AICU_BASE)
        {
            free(PLATFORM_AICU_BASE);
            PLATFORM_AICU_BASE = NULL;
        }
        */
	return (EXIT_SUCCESS);
}
