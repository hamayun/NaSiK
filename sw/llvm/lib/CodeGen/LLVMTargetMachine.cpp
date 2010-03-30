//===-- LLVMTargetMachine.cpp - Implement the LLVMTargetMachine class -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the LLVMTargetMachine class.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "llvm-target-machine"
#include "llvm/Support/Debug.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/GCStrategy.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetAsmInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetMachineRegistry.h"
#include "llvm/ModuleProvider.h"
#include <iostream>
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

static bool CompileModuleForTargetFlag = false;
static int  CompileModuleForTarget(Module *mod,bool Fast);

static cl::opt<bool> PrintLSR("print-lsr-output", cl::Hidden,
    cl::desc("Print LLVM IR produced by the loop-reduce pass"));
static cl::opt<bool> PrintISelInput("print-isel-input", cl::Hidden,
    cl::desc("Print LLVM IR input to isel pass"));
static cl::opt<bool> PrintEmittedAsm("print-emitted-asm", cl::Hidden,
    cl::desc("Dump emitter generated instructions as assembly"));
static cl::opt<bool> PrintGCInfo("print-gc", cl::Hidden,
    cl::desc("Dump garbage collector data"));

// Hidden options to help debugging
static cl::opt<bool>
EnableSinking("enable-sinking", cl::init(false), cl::Hidden,
              cl::desc("Perform sinking on machine code"));
static cl::opt<bool>
EnableLICM("machine-licm",
           cl::init(false), cl::Hidden,
           cl::desc("Perform loop-invariant code motion on machine code"));

// When this works it will be on by default.
static cl::opt<bool>
DisablePostRAScheduler("disable-post-RA-scheduler",
                       cl::desc("Disable scheduling after register allocation"),
                       cl::init(true));

FileModel::Model
LLVMTargetMachine::addPassesToEmitFile(PassManagerBase &PM,
                                       raw_ostream &Out,
                                       CodeGenFileType FileType,
                                       bool Fast,
                                       Module *mod) {
  DOUT << ">>>>>>>>>" << __func__ << ": Entering..., CompileModuleForTargetFlag = " << CompileModuleForTargetFlag << "\n";
  if((CompileModuleForTargetFlag == false) && (AnnotationArch != ""))
  {
    DOUT << ">>>>>>>>> " << __func__ << ": compile module for target " << AnnotationArch << "\n";
    assert( mod != NULL && "A module is needed for annotation !!!");
    DOUT << __func__ << ": compile module for target " << AnnotationArch << "\n";
    CompileModuleForTarget(mod,Fast);
  }

  // Standard LLVM-Level Passes.
  
  // Run loop strength reduction before anything else.
  if (!Fast) {
    PM.add(createLoopStrengthReducePass(getTargetLowering()));
    if (PrintLSR)
      PM.add(new PrintFunctionPass("\n\n*** Code after LSR ***\n", &cerr));
  }
  
  PM.add(createGCLoweringPass());

  if (!getTargetAsmInfo()->doesSupportExceptionHandling())
    PM.add(createLowerInvokePass(getTargetLowering()));

  // Make sure that no unreachable blocks are instruction selected.
  PM.add(createUnreachableBlockEliminationPass());

  if (!Fast)
    PM.add(createCodeGenPreparePass(getTargetLowering()));

  if (PrintISelInput)
    PM.add(new PrintFunctionPass("\n\n*** Final LLVM Code input to ISel ***\n",
                                 &cerr));
  
  // Ask the target for an isel.
  if (addInstSelector(PM, Fast))
    return FileModel::Error;

  if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("1_init"));

  // Print the instruction selected machine code...
  if (PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));

  if (EnableLICM)
  {
    PM.add(createMachineLICMPass());
    if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("2_LICM"));
  }

  if (EnableSinking)
  {
    PM.add(createMachineSinkingPass());
    if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("3_Sinking"));
  }

  // Run pre-ra passes.
  if (addPreRegAlloc(PM, Fast) && PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));

  if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("4_pre-ra"));

  // Perform register allocation to convert to a concrete x86 representation
  PM.add(createRegisterAllocator());
  if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("5_regalloc"));
  
  // Perform stack slot coloring.
  if (!Fast)
  {
    PM.add(createStackSlotColoringPass());
    if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("6_stackcolor"));
  }

  if (PrintMachineCode)  // Print the register-allocated code
    PM.add(createMachineFunctionPrinterPass(cerr));
  
  // Run post-ra passes.
  if (addPostRegAlloc(PM, Fast) && PrintMachineCode)
  {
    PM.add(createMachineFunctionPrinterPass(cerr));
    if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("7_post-ra"));
  }

  PM.add(createLowerSubregsPass());
  if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("8_lowersubreg"));
  
  if (PrintMachineCode)  // Print the subreg lowered code
    PM.add(createMachineFunctionPrinterPass(cerr));

  // Insert prolog/epilog code.  Eliminate abstract frame index references...
  PM.add(createPrologEpilogCodeInserter());
  if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("9_prolog-epilog"));
  
  if (PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));
  
  // Second pass scheduler.
  if (!Fast && !DisablePostRAScheduler)
  {
    PM.add(createPostRAScheduler());
    if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("10_post-ras"));
  }

  // Branch folding must be run after regalloc and prolog/epilog insertion.
  if (!Fast)
  {
    PM.add(createBranchFoldingPass(getEnableTailMergeDefault()));
    if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("11_branch-folding"));
  }

  PM.add(createGCMachineCodeAnalysisPass());
  if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("12_CG"));

  if (PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));
  
  if (PrintGCInfo)
    PM.add(createGCInfoPrinter(*cerr));
  
  // Fold redundant debug labels.
  PM.add(createDebugLabelFoldingPass());
  
  if (PrintMachineCode)  // Print the register-allocated code
    PM.add(createMachineFunctionPrinterPass(cerr));

  if (addPreEmitPass(PM, Fast) && PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));

  if (!Fast && !OptimizeForSize)
  {
    PM.add(createLoopAlignerPass());
    if(CompileModuleForTargetFlag) PM.add(createCFGPrinterPass("13_Loop-Alaigner"));
  }

  if(CompileModuleForTargetFlag == false)
  {
    switch (FileType) {
    default:
      break;
    case TargetMachine::AssemblyFile:
      if (addAssemblyEmitter(PM, Fast, Out))
        return FileModel::Error;
      return FileModel::AsmFile;
    case TargetMachine::ObjectFile:
      if (getMachOWriterInfo())
        return FileModel::MachOFile;
      else if (getELFWriterInfo())
        return FileModel::ElfFile;
    }
  }
  else
  {
    DOUT << __func__ << ": No file will be generated here ...\n";
    return FileModel::None;
  }

  return FileModel::Error;
}
 
/// addPassesToEmitFileFinish - If the passes to emit the specified file had to
/// be split up (e.g., to add an object writer pass), this method can be used to
/// finish up adding passes to emit the file, if necessary.
bool LLVMTargetMachine::addPassesToEmitFileFinish(PassManagerBase &PM,
                                                  MachineCodeEmitter *MCE,
                                                  bool Fast) {
  if (MCE)
    addSimpleCodeEmitter(PM, Fast, PrintEmittedAsm, *MCE);
    
  if(CompileModuleForTargetFlag == true)
  {
    DOUT << __func__ << ": add target annotation pass ...\n";
    if (targetSupportAnnotation()) {
      PM.add(createMachineTargetAnnotationPass(*this));
    } else {
      cerr << "Warning: target annotation not supported!!!\n";
    }
  }


  PM.add(createGCInfoDeleter());

  // Delete machine code for this function
  PM.add(createMachineCodeDeleter());

  return false; // success!
}

/// addPassesToEmitMachineCode - Add passes to the specified pass manager to
/// get machine code emitted.  This uses a MachineCodeEmitter object to handle
/// actually outputting the machine code and resolving things like the address
/// of functions.  This method should returns true if machine code emission is
/// not supported.
///
bool LLVMTargetMachine::addPassesToEmitMachineCode(PassManagerBase &PM,
                                                   MachineCodeEmitter &MCE,
                                                   bool Fast) {
  // Standard LLVM-Level Passes.
  
  // Run loop strength reduction before anything else.
  if (!Fast) {
    PM.add(createLoopStrengthReducePass(getTargetLowering()));
    if (PrintLSR)
      PM.add(new PrintFunctionPass("\n\n*** Code after LSR ***\n", &cerr));
  }
  
  PM.add(createGCLoweringPass());
  
  if (!getTargetAsmInfo()->doesSupportExceptionHandling())
    PM.add(createLowerInvokePass(getTargetLowering()));
  
  // Make sure that no unreachable blocks are instruction selected.
  PM.add(createUnreachableBlockEliminationPass());

  if (!Fast)
    PM.add(createCodeGenPreparePass(getTargetLowering()));

  if (PrintISelInput)
    PM.add(new PrintFunctionPass("\n\n*** Final LLVM Code input to ISel ***\n",
                                 &cerr));

  // Ask the target for an isel.
  if (addInstSelector(PM, Fast))
    return true;

  // Print the instruction selected machine code...
  if (PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));

  if (EnableLICM)
    PM.add(createMachineLICMPass());
  
  if (EnableSinking)
    PM.add(createMachineSinkingPass());

  // Run pre-ra passes.
  if (addPreRegAlloc(PM, Fast) && PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));

  // Perform register allocation.
  PM.add(createRegisterAllocator());

  // Perform stack slot coloring.
  if (!Fast)
    PM.add(createStackSlotColoringPass());

  if (PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));
    
  // Run post-ra passes.
  if (addPostRegAlloc(PM, Fast) && PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));

  if (PrintMachineCode)  // Print the register-allocated code
    PM.add(createMachineFunctionPrinterPass(cerr));
  
  PM.add(createLowerSubregsPass());
  
  if (PrintMachineCode)  // Print the subreg lowered code
    PM.add(createMachineFunctionPrinterPass(cerr));

  // Insert prolog/epilog code.  Eliminate abstract frame index references...
  PM.add(createPrologEpilogCodeInserter());
  
  if (PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));
  
  // Second pass scheduler.
  if (!Fast)
    PM.add(createPostRAScheduler());

  // Branch folding must be run after regalloc and prolog/epilog insertion.
  if (!Fast)
    PM.add(createBranchFoldingPass(getEnableTailMergeDefault()));

  PM.add(createGCMachineCodeAnalysisPass());

  if (PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));
  
  if (PrintGCInfo)
    PM.add(createGCInfoPrinter(*cerr));
  
  if (addPreEmitPass(PM, Fast) && PrintMachineCode)
    PM.add(createMachineFunctionPrinterPass(cerr));

  addCodeEmitter(PM, Fast, PrintEmittedAsm, MCE);
  
  PM.add(createGCInfoDeleter());
  
  // Delete machine code for this function
  PM.add(createMachineCodeDeleter());
  
  return false; // success!
}

static int  CompileModuleForTarget(Module *mod,bool Fast) {
  std::string Err;

  DOUT << __func__ << ": Entering...\n";

  CompileModuleForTargetFlag = true;

  // Package up features to be passed to target/subtarget
  std::string FeaturesStr;

  const TargetMachineRegistryEntry  *selectedTarget = NULL;

  for (Registry<TargetMachine>::iterator I = Registry<TargetMachine>::begin(), E = Registry<TargetMachine>::end(); I != E; ++I)
  {
    if(I->Name == AnnotationArch)
    {
      selectedTarget = &(*I);
      break;
    }
  }
  assert(selectedTarget != NULL && "Target annotated architecture not found !!!");

  std::auto_ptr<TargetMachine> target(selectedTarget->CtorFn(*mod, FeaturesStr));
  assert(target.get() && "Could not allocate target machine!");
  TargetMachine &Target = *target.get();

  // Build up all of the passes that we want to do to the module.
  DOUT << __func__ << ": Build up all of the passes...\n";
  FunctionPassManager Passes(new ExistingModuleProvider(mod));
  Passes.add(new TargetData(*Target.getTargetData()));

  Target.addPassesToEmitFile(Passes, outs(), TargetMachine::AssemblyFile, Fast);
  Target.addPassesToEmitFileFinish(Passes, NULL, Fast);

  Passes.doInitialization();

  // Run our queue of passes all at once now, efficiently.
  // TODO: this could lazily stream functions out of the module.
  for (Module::iterator I = mod->begin(), E = mod->end(); I != E; ++I)
    if (!I->isDeclaration())
    {
      DOUT << __func__ << ": run " << (*I).getName() << std::endl;
      Passes.run(*I);
    }

  Passes.doFinalization();

  CompileModuleForTargetFlag = false;

  return (0);
}

