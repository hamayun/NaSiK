// Courtesy of Luc MICHEL ;)
// Adapted for Analyzing KVM Trace Logs
// Many Thanks -- Hamayun

// Import the DSS packages into our namespace to save on typing
importPackage(Packages.com.ti.debug.engine.scripting);
importPackage(Packages.com.ti.ccstudio.scripting.environment);
importPackage(Packages.java.lang);
importPackage(Packages.java.io);

var kvm_regs_val = new Array();
var cur_trace_line = 0;

function getNextTrace(reader) 
{
    var str = reader.readLine();
    var addr = null;

    cur_trace_line++;
    while(str != null && !str.matches("^TRACE PCE1 .*")) {
        str = reader.readLine();
        cur_trace_line++;
    }

    if(str != null) {
        addr = Long.parseLong(str.replaceFirst("^TRACE PCE1 \\[([0-9a-f]+)\\].*", "$1"), 16);
    }

    return addr;
}

function getNextRegister(reader, mod) 
{
    var str = null;
    var reg_val = null;
    var traceline_pos_save = cur_trace_line;
    var read_ahead_limit = 4096;

    if(mod < 3){
        // Mark the present position in the stream.
        // Subsequent calls to reset() will attempt to reposition the stream to this point.
        reader.mark(read_ahead_limit);
    }

    str = reader.readLine();
    cur_trace_line++;

    while(str != null && !str.matches("( *[AB][0-9]+=0x[0-9a-fA-F]{8}){4}")) {
        str = reader.readLine();
        cur_trace_line++;
    }

    if(str != null) {
        regex="^ *[AB][0-9]+=0x([0-9a-fA-F]{8}) +[AB][0-9]+=0x([0-9a-fA-F]{8}) +[AB][0-9]+=0x([0-9a-fA-F]{8}) +[AB][0-9]+=0x([0-9a-fA-F]{8})";
        reg_val = Long.parseLong(str.replaceFirst(regex, "$" + new Integer(mod+1).toString()), 16);
    }

    if(mod < 3 && str != null) {
        reader.reset();
        cur_trace_line = traceline_pos_save;
    }

    return reg_val;

}

function readNextEntry(reader) 
{
    for(ibank = 'A'.charCodeAt(0); ibank < 'C'.charCodeAt(0); ibank++) {
        bank = String.fromCharCode(ibank);

        for(reg = 0; reg < 16; reg++) {
            reg_name = bank + new Integer(reg).toString();

            kvm_regs_val[reg_name] = getNextRegister(reader, reg % 4);
        }
    }

    return getNextTrace(reader);
}

/* Zeros all GPR of the simulator */
function setCssIntialState(debugSession)
{
    for(ibank = 'A'.charCodeAt(0); ibank < 'C'.charCodeAt(0); ibank++) {
        bank = String.fromCharCode(ibank);

        for(reg = 0; reg < 16; reg++) {
            reg_name = bank + new Integer(reg).toString();
            debugSession.memory.writeRegister(reg_name, 0);
        }
    }
}

// Create our scripting environment object - which is the main entry point into any script and
// the factory for creating other Scriptable ervers and Sessions
var script = ScriptingEnvironment.instance();

// Create a log file in the current directory to log script execution
script.traceBegin("BreakpointsTestLog.xml", "DefaultStylesheet.xsl");

// Set our TimeOut
script.setScriptTimeout(15000);

// Log everything
script.traceSetConsoleLevel(TraceLevel.INFO);
script.traceSetFileLevel(TraceLevel.INFO);

// Get the Debug Server and start a Debug Session
debugServer = script.getServer("DebugServer.1");
debugServer.setConfig("../C64/tisim_c64xple.ccxml");
debugSession = debugServer.openSession(".*");

// I don't want to loose time to search how to read command line arguments for the moment
var c6x_coff_binary = "/home/hamayun/workspace_ccs/factorial/Debug/factorial.out";
var kvm_trace_file  = "/home/hamayun/workspace/NaSiK/examples/platforms/tuzki/tty100";
var stop_on_first_err = true;

var fstream;
var fin;
var reader;

// Stuff for reading into the kvm traces... Bloody language; Original Comment from Luc
try {
    fstream = new FileInputStream(kvm_trace_file);
    fin     = new DataInputStream(fstream);
    reader  = new BufferedReader(new InputStreamReader(fin)); /* Yeah... me neither, I don't beleve it */
} catch(e) {
    script.traceWrite("Error while opening trace file");
    script.traceEnd();
    java.lang.System.exit(1);
}

if(!reader.markSupported()) {
    script.traceWrite("Does not support mark -_-'");
    script.traceEnd();
    java.lang.System.exit(1);
}

try {
    debugSession.memory.loadProgram(c6x_coff_binary);
} catch(e) {
    script.traceWrite("Error while opening c6x_coff_binary");
    script.traceEnd();
    java.lang.System.exit(1);
}

// Remove the breakpoints put by the feaukt conf of the simulator
debugSession.breakpoint.removeAll();

// Get the first trace addr
var trace = readNextEntry(reader);
var is_started = false;

if(trace == null) {
    script.traceWrite("No trace found in " + kvm_trace_file);
    script.traceEnd();
    java.lang.System.exit(1);
} else {
    script.traceWrite("First trace is 0x" + Integer.toHexString(trace));
}

setCssIntialState(debugSession);

// Main verification loop
while(trace != null) {
    err_encountered = false;
    bp_id = debugSession.breakpoint.add(trace);

    script.traceWrite("Comparing .... 0x" + Integer.toHexString(trace));

    if(is_started)
        debugSession.target.run();
    else {
        debugSession.target.restart();
        is_started = true;
    }

    // Check that we stopped at the trace addr.
    // If we not, it's probably that KVM's gone wild
    var css_stop_addr = debugSession.expression.evaluate("PC");
    if(css_stop_addr != trace) {
        script.traceWrite("CCS did not stop at 0x" + Integer.toHexString(trace) + " but at 0x" + 
                          Integer.toHexString(css_stop_addr) + "!");
        script.traceEnd();
        java.lang.System.exit(1);
    }

    // Registers value comparison
    var bank, reg;
    for(ibank = 'A'.charCodeAt(0); ibank < 'C'.charCodeAt(0); ibank++) {
        bank = String.fromCharCode(ibank);

        for(reg = 0; reg < 16; reg++) {
            reg_name = bank + new Integer(reg).toString();
            if(kvm_regs_val[reg_name] == null) {
                script.traceWrite("kvm_regs_val was null for " + reg_name);
                script.traceEnd();
                java.lang.System.exit(1);
            }

            var ccs_reg_val = debugSession.memory.readRegister(reg_name);

            if(kvm_regs_val[reg_name] != ccs_reg_val) {
                err_encountered = true;
                script.traceWrite("Trace 0x" + Long.toHexString(trace) +
                                  " (" + kvm_trace_file + ":" + new Integer(cur_trace_line).toString() + ")" +
                                  ": kvm " + reg_name + "=" + Long.toHexString(kvm_regs_val[reg_name]) +
                                  ", ccs " + reg_name + "=" + Long.toHexString(ccs_reg_val));
            }
        }
    }

    if(err_encountered && stop_on_first_err) {
        script.traceWrite("Differences found, quitting ...");
        script.traceEnd();
        java.lang.System.exit(1);
    }

    debugSession.breakpoint.remove(bp_id);
    trace = readNextEntry(reader);
}

// All done
fin.close();
debugSession.terminate();
debugServer.stop();

script.traceSetConsoleLevel(TraceLevel.INFO);
script.traceWrite("TEST SUCCEEDED!");

// Stop logging and exit.
script.traceEnd();
java.lang.System.exit(0);

