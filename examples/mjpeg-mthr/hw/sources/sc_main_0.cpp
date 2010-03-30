/* Vendor :vendor*/
/* Libray :library*/
/* Name :DesignTest*/
/* Version :1.0*/


/*A simple example*/


#include <systemc.h>
#include <stdio.h>
#include "signal.h"

#include "execution_spy.h"
#include "eu/dna/proc/dna_eu.h"
#include "generic_noc.h"
#include "tty_pool.h"
#include "debug.h"
#include "linker/dna/dna_linker.h"
#include "memory.h"
#include "soclib_fb.h"
#include "soclib_timer.h"
#include "bridgefs.h"

static bool quit = false;

extern int sc_configure(sc_simcontext * simcontext, char *filename);

void simulation_stop(int signo)
{
	cout << "************************************************************"
		<< endl;
	cout << "              USER REQUEST END OF SIMULATION                "
		<< endl;
	cout << " Simulation time : " << sc_time_stamp() << endl;;
	
  libta::annotation::ExecutionSpy::close();
  sc_stop();

	quit = true;
}

using namespace std;
using namespace libta;

int sc_main(int argc, char *argv[])
{

  signal(SIGINT,simulation_stop);

  argc = libta::parseCommandLine(argv);

  if (argc < 2) {
	cerr << "usage : " << argv[0] << " <conf_file(s)> <libta_options>" << endl;
	return (!EXIT_SUCCESS);
  }

  dna_eu            eu_inst_0("eu_inst_0");
  tty_pool           tty_pool_inst_0("tty_pool_inst_0");
  generic_noc     generic_noc_inst_0("generic_noc_inst_0");
  dna_linker	   linker_dna("linker_dna");
  memory	   ram0("ram0");
  soclib_fb  	   fb("fb");
  soclib_timer     timer("timer");
  bridgefs           fdaccess("fdaccess");

  // Devices Channels
#ifdef CHANNELS
  dev_channel    eu0_channel("eu0_channel");
  dev_channel    tty_pool_channel("tty_pool_channel");
  dev_channel    ram0_channel("ram0_channel");
  dev_channel    fb_channel("fb_channel");
  dev_channel    timer_channel("timer_channel");
  dev_channel    fdaccess_channel_mst("fdaccess_channel_mst");
  dev_channel    fdaccess_channel_slv("fdaccess_channel_slv");
#endif

  // Slave connection
#ifdef CHANNELS
	generic_noc_inst_0.p_io(tty_pool_channel.exp_io);
	generic_noc_inst_0.p_io(ram0_channel.exp_io);
	generic_noc_inst_0.p_io(fb_channel.exp_io);
	generic_noc_inst_0.p_io(timer_channel.exp_io);
	generic_noc_inst_0.p_io(fdaccess_channel_slv.exp_io);
#else
	generic_noc_inst_0.p_io(tty_pool_inst_0.exp_io);
	generic_noc_inst_0.p_io(ram0.exp_io);
	generic_noc_inst_0.p_io(fb.exp_io);
	generic_noc_inst_0.p_io(timer.exp_io);
	generic_noc_inst_0.p_io(fdaccess.exp_io);
#endif

#ifdef CHANNELS
	tty_pool_channel.p_io(tty_pool_inst_0.exp_io);
	ram0_channel.p_io(ram0.exp_io);
	fb_channel.p_io(fb.exp_io);
	timer_channel.p_io(timer.exp_io);
	fdaccess_channel_slv.p_io(fdaccess.exp_io);
#endif

  //
#ifdef CHANNELS
  eu_inst_0.p_io(eu0_channel.exp_io);
  fdaccess.p_io(fdaccess_channel_mst.exp_io);
	eu0_channel.p_io(generic_noc_inst_0.exp_io);
  fdaccess_channel_mst.p_io(generic_noc_inst_0.exp_io);
#else
  eu_inst_0.p_io(generic_noc_inst_0.exp_io);
  fdaccess.p_io(generic_noc_inst_0.exp_io);
#endif

	eu_inst_0.p_linker_loader(linker_dna.exp_linker_loader);
	ram0.p_linker_loader(linker_dna.exp_linker_loader);

  // Connect linker to component
	linker_dna.p_linker(tty_pool_inst_0.exp_linker);
	linker_dna.p_linker(fb.exp_linker);
	linker_dna.p_linker(timer.exp_linker);
	linker_dna.p_linker(fdaccess.exp_linker);

  cout << "Load configuration files ..." << endl;
  for(int i = 1; i < argc ; i++)
  {
    cout << " + " << argv[i] << endl;
    if(sc_configure(sc_get_curr_simcontext(), argv[i]) == -1)
      return(EXIT_FAILURE);
  }

#ifdef CHANNELS
  sc_trace_file * trace_file;
  ofstream        vcd_config_file;
  trace_file = sc_create_vcd_trace_file("waveforms");
  vcd_config_file.open("waveforms.sav");

  sc_trace(trace_file, eu0_channel, &vcd_config_file);
  sc_trace(trace_file, tty_pool_channel, &vcd_config_file);
  sc_trace(trace_file, ram0_channel, &vcd_config_file);
  sc_trace(trace_file, fb_channel, &vcd_config_file);
#endif
  sc_start();

#ifdef CHANNELS
  sc_close_vcd_trace_file(trace_file);
  vcd_config_file.close();
#endif

  return (EXIT_SUCCESS);
}
