
// Import the DSS packages into our namespace to save on typing
importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.ccstudio.scripting.environment);
importPackage(Packages.java.lang);
importPackage(Packages.java.io);

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

// Get the Debug Server and start a Debug Session
debugServer = script.getServer("DebugServer.1");
//debugServer.setConfig("../C64/tisim_c64xple.ccxml");
debugServer.setConfig("/home/hamayun/TI/CCSTargetConfigurations/C64x_LE_CycleAccurate.ccxml");
debugSession = debugServer.openSession(".*");

var c6x_coff_binary = "/home/hamayun/workspace_ccs/factorial/Debug/factorial.out";

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

