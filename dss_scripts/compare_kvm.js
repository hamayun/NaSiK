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

var pad_str = new Array("", "0", "00", "000", "0000", "00000", "000000", "0000000", "0000000");

function printCcsRegisters(debugSession)
{
    var bank, reg;
    out_str = "";
    for(ibank = 'A'.charCodeAt(0); ibank < 'C'.charCodeAt(0); ibank++) {
        bank = String.fromCharCode(ibank);

        for(reg = 0; reg < 16; reg++) {
            reg_name = bank + new Integer(reg).toString();
            var reg_val = debugSession.memory.readRegister(reg_name);
            var pad_cnt = Math.floor((Long.numberOfLeadingZeros(reg_val) - 32) / 4);

            if(reg < 10) out_str += " ";
            out_str += "  " + reg_name + "=0x" + pad_str[pad_cnt] + Long.toHexString(reg_val);

            if(reg % 4 == 3)
            {
                script.traceWrite(out_str);
                out_str = "";
            }
        }
        out_str = "\n";
    }
    return;
}

function printCcsStack(debugSession, stack_start)
{
    var stack_ptr = debugSession.memory.readRegister("B15");
    var stack_val = null;
    var stack_size = 0;
    var n_page = null;

    stack_size = stack_start - stack_ptr + 4;
    script.traceWrite("Stack Start: 0x" + Long.toHexString(stack_start) +
                      " Pointer: 0x" + Long.toHexString(stack_ptr) +
                      " Size: 0x" + Long.toHexString(stack_size));

    while(stack_ptr <= stack_start)
    {
        n_page = debugSession.memory.getPage(0);
        stack_val = debugSession.memory.readWord(n_page, stack_ptr);
        script.traceWrite("[" + Long.toHexString(stack_ptr) + "] = 0x" + Long.toHexString(stack_val));

        stack_ptr += 4;
    }
    return;
}

function printCcsMemory(start_addr, size)
{
    var curr_addr = start_addr;
    var end_addr = start_addr + size;
    var mem_val = null;
    var n_page = 0x0;

    script.traceWrite("Memory Dump: From 0x" + Long.toHexString(start_addr) +
                              " To: 0x" +  Long.toHexString(end_addr));

    while(curr_addr < end_addr)
    {
        n_page = debugSession.memory.getPage(0);
        mem_val = debugSession.memory.readWord(n_page, curr_addr);

        script.traceWrite("[" + Long.toHexString(curr_addr) + "] = 0x" + Long.toHexString(mem_val));
        curr_addr += 4;
    }
    return;
}

function getFileLinesCount(filename)
{
    lineReader = new LineNumberReader(new FileReader(filename));
    var cnt = 0;
    lineRead = "";
    while ((lineRead = lineReader.readLine()) != null) {}
    cnt = lineReader.getLineNumber();
    lineReader.close();
    return cnt;
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
// script.setScriptTimeout(50000);
script.setScriptTimeout(-1);

// Log everything
script.traceSetConsoleLevel(TraceLevel.INFO);
script.traceSetFileLevel(TraceLevel.INFO);

// Get the Debug Server and start a Debug Session
debugServer = script.getServer("DebugServer.1");
//debugServer.setConfig("../C64/tisim_c64xple.ccxml");
debugServer.setConfig("/home/hamayun/TI/CCSTargetConfigurations/C64x_LE_CycleAccurate.ccxml");
debugSession = debugServer.openSession(".*");

// I don't want to loose time to search how to read command line arguments for the moment
//var c6x_coff_binary = "/home/hamayun/workspace_ccs/matmult/Debug/matmult.out";
//var c6x_coff_binary = "/home/hamayun/workspace_ccs/factorial/Debug/factorial.out";
var c6x_coff_binary = "/home/hamayun/workspace_ccs/IDCT/Debug/IDCT.out";
var kvm_trace_file  = "/home/hamayun/workspace/NaSiK/examples/platforms/tuzki/tty_debug_00";
var stop_on_first_err = true;
var mem_dump_flag = false;
var trace_lines_total = getFileLinesCount(kvm_trace_file);
var trace_progress = 0;

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
var ccs_stack_start = null;

if(trace == null) {
    script.traceWrite("No trace found in " + kvm_trace_file);
    script.traceEnd();
    java.lang.System.exit(1);
} else {
    script.traceWrite("First trace is 0x" + Integer.toHexString(trace));
}

setCssIntialState(debugSession);

//printCcsMemory(0x49C50, 0x1AC);

// Main verification loop
while(trace != null) {
    err_encountered = false;
    bp_id = debugSession.breakpoint.add(trace);

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

    if(!ccs_stack_start || (ccs_stack_start & 0x80000000))
    {
        ccs_stack_start = debugSession.memory.readRegister("B15");
        //script.traceWrite("Stack Start: 0x" + Long.toHexString(ccs_stack_start));
    }

    /*
    if(trace == 0x89ac || mem_dump_flag)
    {
        mem_dump_flag = true;
        printCcsRegisters(debugSession);
    }*/

    //var ccs_clock_cycles = debugSession.memory.readRegister("CLK");

    trace_progress = (cur_trace_line / trace_lines_total) * 100;
    script.traceWrite("[" + Integer(Math.floor(trace_progress)).toString() + "%]: TRACE PCE1 [" + Integer.toHexString(trace) + "]");

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

                //printCcsStack(debugSession, ccs_stack_start);
            }
        }
    }

    if(mem_dump_flag)
    {
        printCcsMemory(0x9E68, 36);
    }

    if(err_encountered && stop_on_first_err) {
        script.traceWrite("Differences found, quitting ...");
        script.traceEnd();
        java.lang.System.exit(1);
    }

    debugSession.breakpoint.remove(bp_id);
    trace = readNextEntry(reader);
}

script.traceWrite("[100%]: TRACE PCE1 [NULL]");

// All done
fin.close();
debugSession.terminate();
debugServer.stop();

script.traceSetConsoleLevel(TraceLevel.INFO);
script.traceWrite("TEST SUCCEEDED!");

// Stop logging and exit.
script.traceEnd();
java.lang.System.exit(0);

