
// Import the DSS packages into our namespace to save on typing
importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.ccstudio.scripting.environment);
importPackage(Packages.java.lang);
importPackage(Packages.java.io);

var c6x_coff_binary;
var use_C64xp = false;
var use_C64x_custom = false;

function usage() {
    script.traceWrite("Usage: ccs_simulate [-p] app_dir_path");
    script.traceWrite("app_dir_path : Path to the Directory Containing C6x Executables");
    script.traceWrite("          -p : Use C64x+ Simulator (Slower)");
}

function read_args(args)
{
    if(args.length < 1) {
        usage();
        java.lang.System.exit(1);
    }

    for(i = 0; i < args.length; i++) 
    {
        switch(args[i])
        {
            case "-p":
                use_C64xp = true;
                break;

            case "-q":
                use_C64x_custom = true;
                break;

            default:
                c6x_coff_binary = args[i];
                break;
        }
    }
}

// Create our scripting environment object - which is the main entry point into any script and
// the factory for creating other Scriptable ervers and Sessions
var script = ScriptingEnvironment.instance();

// Create a log file in the current directory to log script execution
script.traceBegin("BreakpointsTestLog.xml", "DefaultStylesheet.xsl");

// Set our TimeOut
script.setScriptTimeout(-1);        // In milliseconds or -1 for Infinite.

// Log everything
script.traceSetConsoleLevel(TraceLevel.INFO);
script.traceSetFileLevel(TraceLevel.OFF);

c6x_coff_binary = ""
read_args(arguments);

// Get the Debug Server and start a Debug Session
debugServer = script.getServer("DebugServer.1");
if(use_C64xp)
{
    debugServer.setConfig("../C64/tisim_c64xple.ccxml");
    script.traceWrite("Using C64x+ Simulator (Slower)");
}
else if(use_C64x_custom)
{
    debugServer.setConfig("/home/hamayun/TI/CCSTargetConfigurations/C64xx_LE_CA_CPU_Only.ccxml");
    script.traceWrite("Using C64x Simulator (Custom)");
}
else
{
    debugServer.setConfig("/home/hamayun/TI/CCSTargetConfigurations/C6416_LE_CA_DeviceFunctional.ccxml");
    script.traceWrite("Using C64x Simulator (Faster)");
}

debugSession = debugServer.openSession(".*");

//script.traceWrite("Target Binary :" + c6x_coff_binary);

try {
    debugSession.memory.loadProgram(c6x_coff_binary);
} catch(e) {
    script.traceWrite("Error while opening c6x_coff_binary");
    script.traceEnd();
    java.lang.System.exit(1);
}

debugSession.breakpoint.removeAll();
//script.traceWrite("Going to Run ... ");
var start = new Date().getTime();

debugSession.target.run();

// Make sure the target is halted
// while (debugSession.target.isHalted() == false){}

var elapsed = new Date().getTime() - start;

debugSession.terminate();
debugServer.stop();

script.traceWrite("Time Used : " + elapsed/1000 + " Seconds [" + elapsed + " ms]");

// Stop logging and exit.
script.traceEnd();
java.lang.System.exit(0);

