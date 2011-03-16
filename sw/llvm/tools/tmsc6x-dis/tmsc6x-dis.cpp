/*************************************************************************************
 * File   : tmsc6x-dis.cpp,
 *
 * Copyright (C)    2011 TIMA Laboratory
 * Author(s) :      Mian Muhammad, HAMAYUN mian-muhammad.hamayun@imag.fr
 * Bug Fixer(s) :
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *************************************************************************************/

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Type.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Assembly/AssemblyAnnotationWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/System/Signals.h"

#include <fstream>
#include "block_packets.h"

using namespace llvm;
using namespace std;

#if 0
static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input .coff file>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Override output filename"),
               cl::value_desc("filename"));

static cl::opt<bool>
Force("f", cl::desc("Enable binary output on terminals"));

static cl::opt<bool>
DontPrint("disable-output", cl::desc("Don't output the .ll file"), cl::Hidden);

static cl::opt<bool>
ShowAnnotations("show-annotations",
                cl::desc("Add informational comments to the .ll file"));

namespace {
    class CommentWriter : public AssemblyAnnotationWriter {
    public:
      void emitFunctionAnnot(const Function *F,
                             formatted_raw_ostream &OS) {
        OS << "; [#uses=" << F->getNumUses() << ']';  // Output # uses
        OS << '\n';
      }
      void printInfoComment(const Value &V, formatted_raw_ostream &OS) {
        if (V.getType()->isVoidTy()) return;

        OS.PadToColumn(50);
        OS << "; [#uses=" << V.getNumUses() << ']';  // Output # uses
      }
    };
} // end anonymous namespace
#endif


void print_usage(char **argv)
{
    cout << argv[0] << " <input .coff filename>" << " <output .ll filename>" << endl;
    return; 
}

int main(int argc, char **argv) {
    // Print a stack trace if we signal out.
    sys::PrintStackTraceOnErrorSignal();
    PrettyStackTraceProgram X(argc, argv);

    //LLVMContext &Context = getGlobalContext();
    llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

    //cl::ParseCommandLineOptions(argc, argv, "TMSC6X binary (.coff) -> LLVM IR (.ll) disassembler\n");

    if(argc != 3)
    {
        print_usage(argv);
        return (EXIT_FAILURE);
    }

    string in_filename = argv[1];
    string out_filename = argv[2];
    
    cout << "Input .coff file: " << in_filename << endl;
    cout << "Output .ll  file: " << out_filename << endl;

    ofstream * pout_stream = new ofstream(out_filename.c_str(), ios::out);
    if(!pout_stream->is_open()){
        DOUT << "Error: Opening Output File for " << out_filename << endl;
        return (EXIT_FAILURE);
    }
    
    std::string ErrorMessage;
    std::auto_ptr<Module> module;

    coff_reader         reader(in_filename);        // Read the Input File into reader object
    fetch_packet_list   fplist(&reader);            // Make the Fetch Packet List
    execute_packet_list eplist(&fplist);            // Decode and Make the Execute Packet List
    basic_block_list    bblist(&eplist);            // Now Make the Basic Block List


    //eplist.print(pout_stream, PRINT_MODE_BOTH);
    bblist.print(pout_stream, PRINT_MODE_BOTH);

    //basic_block        *pbb = bblist.get_basic_block(292);
    //pbb->print(&cout, PRINT_MODE_BOTH);

    //cout << "Transcoded Basic Block" << endl;
    //pbb->transcode();
    
    // Start the Real Instruction Processing
    // Also Check Valgrind Output for Memory Leaks.

    return (EXIT_SUCCESS);

  /*
  if (MemoryBuffer *Buffer
         = MemoryBuffer::getFileOrSTDIN(InputFilename, &ErrorMessage)) {
    M.reset(ParseBitcodeFile(Buffer, Context, &ErrorMessage));
    delete Buffer;
  }

  if (M.get() == 0) {
    errs() << argv[0] << ": ";
    if (ErrorMessage.size())
      errs() << ErrorMessage << "\n";
    else
      errs() << "bitcode didn't read correctly.\n";
    return 1;
  }
  
  // Just use stdout.  We won't actually print anything on it.
  if (DontPrint)
    OutputFilename = "-";
  
  if (OutputFilename.empty()) { // Unspecified output, infer it.
    if (InputFilename == "-") {
      OutputFilename = "-";
    } else {
      const std::string &IFN = InputFilename;
      int Len = IFN.length();
      // If the source ends in .bc, strip it off.
      if (IFN[Len-3] == '.' && IFN[Len-2] == 'b' && IFN[Len-1] == 'c')
        OutputFilename = std::string(IFN.begin(), IFN.end()-3)+".ll";
      else
        OutputFilename = IFN+".ll";
    }
  }

  std::string ErrorInfo;
  OwningPtr<tool_output_file> 
  Out(new tool_output_file(OutputFilename.c_str(), ErrorInfo,
                           raw_fd_ostream::F_Binary));
  if (!ErrorInfo.empty()) {
    errs() << ErrorInfo << '\n';
    return 1;
  }

  OwningPtr<AssemblyAnnotationWriter> Annotator;
  if (ShowAnnotations)
    Annotator.reset(new CommentWriter());
  
  // All that llvm-dis does is write the assembly to a file.
  if (!DontPrint)
    M->print(Out->os(), Annotator.get());

  // Declare success.
  Out->keep();
	*/

//  return 0;
}

