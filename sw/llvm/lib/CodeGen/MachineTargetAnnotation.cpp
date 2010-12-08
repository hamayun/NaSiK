/*************************************************************************************
 * File   : MachineTargetAnnotation.cpp,     
 *
 * Copyright (C) 2007 TIMA Laboratory
 * Author(s) :      Patrice, GERIN patrice.gerin@imag.fr
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

#define DEBUG_TYPE "annotation"
#include "llvm/CodeGen/Passes.h"
#include "llvm/Constants.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetAnnotationInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h" 
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <map>
#include <list>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cctype>
#include <fstream>

namespace llvm
{
  bool PrintDualCFG;
  bool BranchPenalty;
  bool PrintAnnotatedCFG;

  static cl::opt<bool, true> PrintDualCFGOpt("print-dual-cfg", 
	 cl::desc("Print LLVM and Target Machine CFG in the same graph."),
         cl::location(PrintDualCFG), cl::init(false));

  static cl::opt<bool, true> BranchPenaltyOpt("annotate-branch-penalty",
         cl::desc("Insert annotation for mispredicted branch penalty."),
         cl::location(BranchPenalty), cl::init(false));

  static cl::opt<bool, true> PrintAnnotatedCFGOpt("print-annotated-cfg",
	 cl::desc("Print superimposed LLVM and Machine CFG with annotation statistics."),
         cl::location(PrintAnnotatedCFG), cl::init(false));
}

using namespace llvm;

//#define PRINT_ANNOTATION_DETAILS
#ifdef PRINT_ANNOTATION_DETAILS
    #define ANNOUT if(1) std::cout
#else
    #define ANNOUT if(0) std::cout
#endif

#define DOUT                    if(0) std::cout
#define DDOUT			DOUT << __func__ << ": "

namespace {
  struct MachineTargetAnnotation : public MachineFunctionPass {
  static char ID;

  MachineTargetAnnotation(TargetMachine & T): MachineFunctionPass(ID), _Target(T) {}

  virtual const char *getPassName() const {
    return "Target Annotation pass";
  }

  bool runOnMachineFunction(MachineFunction & F);
  bool doInitialization(Module & M);
  bool doFinalization(Module & M);

  typedef std::list < BasicBlock* > llvmBB_list_t;
  typedef std::list < MachineBasicBlock* > machineBB_list_t;
  typedef std::map < BasicBlock*, machineBB_list_t*> llvmBasicBlockMap_t;

  private:
    unsigned int getInstructionCount(const MachineInstr & MI);

    llvmBasicBlockMap_t * buildLLVMBasicBlockMap(MachineFunction & MF);
    void cleanLLVMBasicBlockMap(llvmBasicBlockMap_t * map);

    bool annotateFunction(MachineFunction & MF);
    bool annotateLLVMBasicBlock(TargetAnnotationDB *MBBinfo, BasicBlock * BB);
    machineBB_list_t * getMBBSuccessors(machineBB_list_t * listMBB);
    llvmBB_list_t * insertAndAnnotateBB(const BasicBlock* pred, machineBB_list_t * succs);

    bool printAnnotatedCFG(MachineFunction & MF);

  private:
    Function		*_LLVM_F;
    Module              *_LLVM_M;
    Function            *_Fannotation;
    Function            *_Fannotation_Entry;
    Function            *_Fannotation_Return;
    TargetMachine       &_Target;
    bool                _plotCFG;
    bool                _entry_flag;
    bool                _return_flag;

    Function         *_Function;
    MachineFunction  *_MachineFunction;

    bool check(BasicBlock* BB, MachineBasicBlock *MBB, std::list< std::string > *lst);
    void printDualCFG();
    void formatDotLabel(std::string *str);
    void formatDotNode(std::string *str);
  };
  char MachineTargetAnnotation::ID = 0;
} // end of anonymous namespace

FunctionPass *llvm::createMachineTargetAnnotationPass(TargetMachine &T) {
  return new MachineTargetAnnotation(T);
}

/// runOnMachineFunction - 
bool MachineTargetAnnotation::doInitialization(Module &M) {
  _Target.InitializeMBBAnnotation();
  return (false);
}

bool MachineTargetAnnotation::doFinalization(Module &M) {
  return (false);
}

bool MachineTargetAnnotation::runOnMachineFunction(MachineFunction &MF) {
  _MachineFunction= &MF;
  _Function = (Function*)(MF.getFunction());

  // Get current function's parent module
  _LLVM_F = (Function*) MF.getFunction();
  assert(_LLVM_F && "Machine Function must have a Function to annotate!\n");
  _LLVM_M = _LLVM_F->getParent();
  assert(_LLVM_M && "Function must have a parent Module!\n");

  LLVMContext &Context = _LLVM_M->getContext();  // The current Module Context
  DDOUT << " run on function : " << _LLVM_F->getNameStr() << " (in " << _LLVM_M->getModuleIdentifier() << ")" << std::endl;

  /// Constant *Module::getOrInsertFunction(StringRef Name, const FunctionType *Ty);
  /// static FunctionType *get(const Type *Result, ///< The result type
  ///                          const std::vector<const Type*> &Params, ///< The types of the parameters
  ///                          bool isVarArg  ///< Whether this is a variable argument length function
  ///                          );

  _Fannotation = cast<Function>(_LLVM_M->getOrInsertFunction(StringRef("mbb_annotation"),
                                FunctionType::get(Type::getVoidTy(Context),
                                std::vector<const Type*> (1, PointerType::get(IntegerType::get(Context, 8),0)), false)));

  /*
  _Fannotation_Entry = cast<Function>(_LLVM_M->getOrInsertFunction(StringRef("annotation_entry"),
                                FunctionType::get(Type::getVoidTy(Context), false)));

  _Fannotation_Return = cast<Function>(_LLVM_M->getOrInsertFunction(StringRef("annotation_return"),
                                FunctionType::get(Type::getVoidTy(Context), false)));
  */
  
  _plotCFG = false;
  annotateFunction(MF);
  if (PrintDualCFG)
  {
    printDualCFG();
  }

  if (_plotCFG || PrintAnnotatedCFG)
  {
    // Call this Function to Create MBB BB and DOT Files for CFG Viewing
    printAnnotatedCFG(MF);
  }

  return (true);
}

bool MachineTargetAnnotation::annotateFunction(MachineFunction &MF) {
  llvmBasicBlockMap_t *llvmBasicBlockMap;
  machineBB_list_t *listMachineBasicBlock;
  machineBB_list_t *succlistMachineBasicBlock;
  BasicBlock *llvmBasicBlock;
  MachineBasicBlock *machineBasicBlock, *succMachineBasicBlock;  
  TargetAnnotationDB *annotationDB, *branchAnnotationDB;

  llvmBasicBlockMap = buildLLVMBasicBlockMap(MF);
  assert(llvmBasicBlockMap && "No LLVM BasicBlock map to compute!\n");

  _entry_flag = false;
  _return_flag = false;

  for (llvmBasicBlockMap_t::const_iterator BBI = llvmBasicBlockMap->begin(), BBI_E = llvmBasicBlockMap->end(); BBI != BBI_E; ++BBI) {
    llvmBasicBlock = (*BBI).first;
    assert(llvmBasicBlock && "NULL pointer!\n");
    listMachineBasicBlock = (*llvmBasicBlockMap)[llvmBasicBlock];
    assert(listMachineBasicBlock && "NULL pointer!\n");
    if (listMachineBasicBlock->size() > 1) 
    {
      TargetAnnotationDB compositeDB;
      //_plotCFG = true;
      ANNOUT << llvmBasicBlock->getNameStr() << " have multiple childs! (" << listMachineBasicBlock->size() << ")\n";

      // Iterate through the Machine Basic Block List 
      for (machineBB_list_t::const_iterator MBBI = listMachineBasicBlock->begin(), MBBI_E = listMachineBasicBlock->end(); MBBI != MBBI_E; ++MBBI)
      {
        machineBasicBlock = (*MBBI);
        assert(machineBasicBlock && "NULL pointer!\n");

        annotationDB = (TargetAnnotationDB*) _Target.MachineBasicBlockAnnotation(*machineBasicBlock);
        if(annotationDB != NULL){
            compositeDB.addDB(annotationDB);
        }
      }

      // Handle the Entry/Return Types
      // I wonder if there exists a possibility of having a Basic Block in
      // (In Machine Independent LLVM) containing both Entry *AND* Return Types?
      // This sheme will work when we *ALWAYS* have two different Entry/Return LLVM BBs.
      if (llvmBasicBlock->getNameStr() == "entry")    compositeDB.setType(MBB_ENTRY);
      if (llvmBasicBlock->getNameStr() == "return")   compositeDB.setType(MBB_RETURN);

      // Set the Entry/Return flags for Checking Later on.
      if(compositeDB.getType() & MBB_ENTRY)   _entry_flag  = true;
      if(compositeDB.getType() & MBB_RETURN)  _return_flag = true;

      // Now we have the 'SUM' of all annotation DBs; We annotate the LLVM Basic Block with it.
      annotateLLVMBasicBlock(&compositeDB, llvmBasicBlock);
    }
    else if (listMachineBasicBlock->size() == 1) {
      machineBasicBlock = listMachineBasicBlock->front();
      assert(machineBasicBlock && "NULL pointer!\n");

      if(BranchPenalty)
      {
        // Find Successors of the Current Basic Block and Analyse Branch Penalty for Each Edge. 
        for(MachineBasicBlock::const_succ_iterator succMBB = machineBasicBlock->succ_begin(), succMBB_E = machineBasicBlock->succ_end(); 
            succMBB != succMBB_E; ++succMBB)
        //for(BasicBlock::_succ_iterator succBB = llvmBasicBlock->succ_begin(), succBB_E = llvmBasicBlock->succ_end(); succBB != succBB_E; ++succBB)
        {
            //succlistMachineBasicBlock = (*llvmBasicBlockMap)[*succBB];
            //succlistMachineBasicBlock = *succMBB;

            //if(succlistMachineBasicBlock->size() == 1) {
            //succMachineBasicBlock = listMachineBasicBlock->front(); 
            succMachineBasicBlock = *succMBB; 

            //DOUT << "MBB # "<< machineBasicBlock->getNumber() << "-->SuccMBB # "<< succMachineBasicBlock->getNumber() << std::endl;

            branchAnnotationDB = (TargetAnnotationDB*) _Target.MBBBranchPenaltyAnnotation(*machineBasicBlock, *succMachineBasicBlock);
            if(branchAnnotationDB != NULL)
            {
              ANNOUT << "Penalty = " << branchAnnotationDB->getCycleCount() << std::endl << std::endl;

              BasicBlock *BBdest = (BasicBlock*)(*succMachineBasicBlock).getBasicBlock();
              BasicBlock *BBsrc = BBdest->getSinglePredecessor();

              SmallVector<BasicBlock*, 8> PredsBlocks;
              PredsBlocks.push_back(BBsrc);

              if(BBsrc != 0)
              {
                BasicBlock *BBpenalty = SplitBlockPredecessors(BBdest,&PredsBlocks[0],PredsBlocks.size(),".branch_penalty");
                annotateLLVMBasicBlock(branchAnnotationDB, BBpenalty);

                // Do Something to Add it to the Other Annotation. 
              }
            }
        //}
        }
      }

      annotationDB = (TargetAnnotationDB*) _Target.MachineBasicBlockAnnotation(*machineBasicBlock);
      if(annotationDB != NULL){
        // Handle the Entry/Return Types
        // I wonder if there exists a possibility of having a Basic Block in
        // (In Machine Independent LLVM) containing both Entry *AND* Return Types?
        // This sheme will work when we *ALWAYS* have two different Entry/Return LLVM BBs.
        if (llvmBasicBlock->getNameStr() == "entry")    annotationDB->setType(MBB_ENTRY);
        if (llvmBasicBlock->getNameStr() == "return")   annotationDB->setType(MBB_RETURN);

        // Set the Entry/Return flags for Checking Later on.
        if(annotationDB->getType() & MBB_ENTRY)   _entry_flag  = true;
        if(annotationDB->getType() & MBB_RETURN)  _return_flag = true;

        // Call this function to Insert 'annotate' call with 'annotation db' to the current basic block.
        annotateLLVMBasicBlock(annotationDB, llvmBasicBlock);
      }
    }
  }

  cleanLLVMBasicBlockMap(llvmBasicBlockMap);

  // Normally Every Function *SHOULD* have one entry and one return basic block.
  // Except for the cases when a Function contains an Infinite Loop i.e. Does *Not Return* during the lifetime of the programe.
  // And Also for cases when the doesn't exist a correspondance between Machine Basic Blocks and LLVM Basic Blocks
  // In such cases we insert Dummy Entry/Return Annotation Function Calls. (See Below)
  if(_entry_flag == false || _return_flag == false){
    ANNOUT << "Warning: Function " << MF.getFunction()->getNameStr()
           << "  _entry_flag = " << _entry_flag << "  _return_flag = " << _return_flag << std::endl;
  }

  // Now go through all the LLVM Basic Blocks and Search for Entry/Return BBs.
  // If any of the Entry/Return BBs is missing in an Annoatation Call;
  // We Insert a dummy one with MBB_ENTRY/MBB_RETURN as Annotation Type.
  // This fix is neccessary for the online-analysis option of libta to get performance estimates.

  //std::cout << "($) Function: " << _Function->getNameStr() << std::endl;
  TargetAnnotationDB dummyEntryDB, dummyReturnDB;
  dummyEntryDB.setType(MBB_ENTRY); 
  dummyReturnDB.setType(MBB_RETURN);

  for (Function::iterator BBI = _Function->begin(), E = _Function->end(); BBI != E; ++BBI)
  {
      if(BBI->getAnnotationFlag() == false)
      {
        // We check the entry flag as well to make sure that there is only *ONE* entry BB
        if(BBI->getNameStr() == "entry" && (_entry_flag == false))
        {
            //std::cout << "Annotating with Dummy entry" << std::endl;
            annotateLLVMBasicBlock(&dummyEntryDB, &(*BBI));
        }
        // We check the return flag as well to make sure that there is only *ONE* return BB
        else if(BBI->getNameStr() == "return" && (_return_flag == false))
        {
            //std::cout << "Annotating with Dummy return" << std::endl;
            annotateLLVMBasicBlock(&dummyReturnDB, &(*BBI));
        }
      }
      //std::cout << "($ After ) BB Name: " << BBI->getNameStr() << " IsAnnotated: " << BBI->getAnnotationFlag()
      //          << "   Annotation Type: " << BBI->getAnnotationType() << std::endl;
  }

  return (false);
}

MachineTargetAnnotation::llvmBasicBlockMap_t* MachineTargetAnnotation::buildLLVMBasicBlockMap(MachineFunction &MF) {
  llvmBasicBlockMap_t *llvmBasicBlockMap;
  machineBB_list_t *listMachineBasicBlock;
  BasicBlock *llvmBasicBlock;
  Function *llvmFunction;

  llvmBasicBlockMap = new llvmBasicBlockMap_t;
  assert(llvmBasicBlockMap && "NULL pointer!\n");

  listMachineBasicBlock = NULL;

  // Create LLVM Basic Block Map to contain corresponding Machine Basic Blocks List (For Each BB but currently Empty)
  llvmFunction = (Function*) MF.getFunction();
  for (Function::iterator BBI = llvmFunction->begin(), E = llvmFunction->end(); BBI != E; ++BBI) {
    assert((llvmBasicBlockMap->count(BBI) == 0) && "LLVM BasicBlock already in the map!");
    (*llvmBasicBlockMap)[BBI] = new machineBB_list_t;
    assert((*llvmBasicBlockMap)[BBI] && "NULL pointer!\n");
  }

  // Check How Many of Machine Basic Blocks Correspond to LLVM Basic Blocks
  unsigned int counter = 0;
  for (MachineFunction::iterator BBI = MF.begin(), E = MF.end(); BBI != E; ++BBI) {
    llvmBasicBlock = (BasicBlock*) BBI->getBasicBlock();
    if (llvmBasicBlock != NULL) {
      counter++;
    }
  }

  // Check If the Number of Basic Blocks (Expected as in the Map Above) and Machines Basic Blocks are Equal. 
  if (counter != llvmBasicBlockMap->size()){
    ANNOUT << "Number of BasicBlocks and MachineBasicBlocks don't match! [Function: " << MF.getFunction()->getNameStr() << "] "
           << "BBs: " <<llvmBasicBlockMap->size() << ", MBBs: " << counter << "\n";
    for (MachineFunction::iterator BBI = MF.begin(), E = MF.end(); BBI != E; ++BBI) {
        MachineBasicBlock * machineBasicBlock = BBI;
        llvmBasicBlock = (BasicBlock*) BBI->getBasicBlock();

        if(llvmBasicBlock)
            ANNOUT << "MBB # "    << machineBasicBlock->getNumber() << " --> "
                 << "BB: " << llvmBasicBlock->getName().str() << std::endl;
        else
            ANNOUT << "MBB # "    << machineBasicBlock->getNumber() << " --> "
                 << "NULL" << std::endl;
    }
  }

  // Now Actually Populate the List of Machine Basic Blocks inside the llvmBasicBlockMap
  for (MachineFunction::iterator MBBI = MF.begin(), E = MF.end(); MBBI != E; ++MBBI) {
    llvmBasicBlock = (BasicBlock*) MBBI->getBasicBlock();
    if (llvmBasicBlock != NULL) {
      // Here we get a pointer to the List of MBBs for the llvmBasicBlock (the corresponding BB for the current MBB)
      // Get the pointer to MBBs list from the Map.
      listMachineBasicBlock = (*llvmBasicBlockMap)[llvmBasicBlock];

      // And Insert (push_back) the current MBB into it.
      // This push_back may be called multiple times for the given llvmBasicBlock
      // if we have a one-to-many correspondance between BBs and MBBs.
      listMachineBasicBlock->push_back(MBBI);
    } else {
      // CONSTANT POOL are Machine BasicBlock with no LLVM parent BasicBlock
    }
  }

  return (llvmBasicBlockMap);
}

void MachineTargetAnnotation::cleanLLVMBasicBlockMap(llvmBasicBlockMap_t* map) {
  for (llvmBasicBlockMap_t::const_iterator BBI = map->begin(), E = map->end(); BBI != E; ++BBI) {
    if ((*map)[(*BBI).first])
      delete (*map)[(*BBI).first];
  }
  delete map;
}

bool MachineTargetAnnotation::annotateLLVMBasicBlock(TargetAnnotationDB *MBBinfo, BasicBlock *BB) {
  LLVMContext &Context = _LLVM_M->getContext();  // The current Module Context
  ANNOUT << "Annotating " << BB->getNameStr() << std::endl;

  assert(MBBinfo && "NULL pointer!\n");
  assert(BB && "NULL pointer!\n");

  std::vector < Constant * > DumpValues;
  char * dump = MBBinfo->Dump();                // Get the AnnotationDB as a Dump

  for( unsigned int i = 0 ; i < MBBinfo->DumpSize() ; i++){
    DumpValues.push_back(ConstantInt::get(IntegerType::get(Context, 8), dump[i]));      // Push AnnotationDB Dump to DumpValues Vector
  }

  std::string VarName = BB->getNameStr() + "_db";

  // Here we create a Global Variable in the current module and Dump Annotation Info for the Current BB into it.
  GlobalVariable * DumpVar = new GlobalVariable(*_LLVM_M,
                                                ArrayType::get(IntegerType::get(Context, 8), MBBinfo->DumpSize()),
                                                true,
                                                GlobalValue::InternalLinkage,
                                                ConstantArray::get(ArrayType::get(IntegerType::get(Context, 8), MBBinfo->DumpSize()), DumpValues),
                                                VarName.data());

  std::vector<Constant*> const_ptr_7_indices;
  Constant* const_int32_8 = Constant::getNullValue(IntegerType::get(Context, 32));
  const_ptr_7_indices.push_back(const_int32_8);
  const_ptr_7_indices.push_back(const_int32_8);
  Constant* GEP = ConstantExpr::getGetElementPtr(DumpVar, &const_ptr_7_indices[0], const_ptr_7_indices.size() );

  // Get the first valide instruction for the insertion of the call; Skip the Phi Instructions
  // A phi instruction is used to select appropriate version of a given variable in SSA form.
  // And usually this is done using grouping those variables togather and telling to compiler to use
  // the same register for storing them.
  BasicBlock::iterator BBI = ((BasicBlock*) BB)->begin();
  while (isa<PHINode > (BBI)) BBI++;

  // Create the Call Instruction for annotate function.
  CallInst*  annotationCall= CallInst::Create(_Fannotation, GEP, "");       
  assert(annotationCall	&& "NULL pointer!\n");

  // Here we Insert the Annotation Function Call to Basic Block
  BB->getInstList().insert(BBI, annotationCall);

  // Set the Annoatation Flag so that we can check it later on if required.
  BB->setAnnotationFlag(true);

  //std::cout << "Setting Annotation Type to: " << MBBinfo->getType() << std::endl;
  BB->setAnnotationType(MBBinfo->getType()); 

  /* MMH: Un-comment this to have different calls to Entry/Return BBs */
  // Effectively we don't need to have different function calls
  // as now we are dealing with the BB type in H/W analyzer component
  //if(pred_begin(BB) == pred_end(BB))
  //{
  //  CallInst*  annotationEntryCall= CallInst::Create(_Fannotation_Entry, "");
  //  assert(annotationEntryCall	&& "NULL pointer!\n");
  //  BB->getInstList().insert(BBI, annotationEntryCall);
  //  _entry_flag = true;
  //}

  //if(succ_begin(BB) == succ_end(BB))
  //{
  //  CallInst*  annotationReturnCall= CallInst::Create(_Fannotation_Return, "");
  //  assert(annotationReturnCall	&& "NULL pointer!\n");
  //  BB->getInstList().insert(BB->getTerminator(), annotationReturnCall);
  //  _return_flag = true;
  //}

  return (true);
}

bool MachineTargetAnnotation::printAnnotatedCFG(MachineFunction &MF)
{
  std::string fileName;
  std::string name1;
  BasicBlock* BB;
  MachineBasicBlock* MBB;
  machineBB_list_t* listMachineBasicBlock;
  llvmBasicBlockMap_t *llvmBasicBlockMap;
  TargetAnnotationDB  * annotationDB;

  assert(_LLVM_F && "Machine Function must have a Function to annotate!\n");

  fileName = _LLVM_F->getNameStr() + "_annotated_CFG" + ".dot";
  std::string CFGErrorInfo;
  raw_fd_ostream fileCFG(fileName.c_str(), CFGErrorInfo);

  fileName = _LLVM_F->getNameStr() + ".bb";
  std::string BBErrorInfo;
  raw_fd_ostream fileBB(fileName.c_str(), BBErrorInfo);

  fileName = _LLVM_F->getNameStr() + ".mbb";
  std::string MBBErrorInfo;
  raw_fd_ostream fileMBB(fileName.c_str(), MBBErrorInfo);

  fileCFG << "digraph module_graph {\n";

  llvmBasicBlockMap = buildLLVMBasicBlockMap(MF);
  for (llvmBasicBlockMap_t::const_iterator BBI = llvmBasicBlockMap->begin(), E = llvmBasicBlockMap->end(); BBI != E; ++BBI)
  {
    BB = (*BBI).first;
    name1 = BB->getNameStr();
    size_t pos = -1;
    while ((pos = name1.find(".", pos + 1)) != std::string::npos)
        name1.replace(pos, 1, "_");

    listMachineBasicBlock = (*llvmBasicBlockMap)[(*BBI).first];
    if (listMachineBasicBlock->size() >= 1)
    {
      fileCFG << "\tsubgraph cluster_" << name1 << " {\n";
      fileCFG << "\t\tlabel = \"" << name1 << "\";\n";
      fileCFG << "\t\tnode [style=filled];\n";
      if (listMachineBasicBlock->size() > 1)
      {
        fileCFG << "\t\tstyle=filled;\n";
        fileCFG << "\t\tcolor=red;\n";
      }
      
      while (!listMachineBasicBlock->empty())
      {
        MBB = listMachineBasicBlock->front();
        listMachineBasicBlock->pop_front();
        annotationDB = (TargetAnnotationDB*) _Target.MachineBasicBlockAnnotation(*MBB);
        if( MBB->succ_begin() ==  MBB->succ_end() )
        {
          fileCFG << "\t\tnode " << MBB->getNumber() << " [label = \"return_#" << MBB->getNumber() 
                  << "(I:" << ((annotationDB != NULL) ? annotationDB->getInstructionCount() : -1)
                  << ",C:" << ((annotationDB != NULL) ? annotationDB->getCycleCount() : -1) << ")" <<"\"];\n";
        }
        else if( MBB->pred_begin() ==  MBB->pred_end() )
        {
          fileCFG << "\t\tnode " << MBB->getNumber() << " [label = \"entry_#" << MBB->getNumber() 
                  << "(I:" << ((annotationDB != NULL) ? annotationDB->getInstructionCount() : -1)
                  << ",C:" << ((annotationDB != NULL) ? annotationDB->getCycleCount() : -1) << ")" <<"\"];\n";
        }
        else
        {
          fileCFG << "\t\tnode " << MBB->getNumber() << " [label = \"_#" << MBB->getNumber() 
                  << "(I:" << ((annotationDB != NULL) ? annotationDB->getInstructionCount() : -1)
                  << ",C:" << ((annotationDB != NULL) ? annotationDB->getCycleCount() : -1) << ")" <<"\"];\n";
        }
      }
      fileCFG << "\t}\n";
    }
  }
  cleanLLVMBasicBlockMap(llvmBasicBlockMap);

  // Print the Machine Basic Blocks
  for (MachineFunction::iterator MBBI = MF.begin(), MBBI_E = MF.end(); MBBI != MBBI_E; ++MBBI) {
    fileMBB << *MBBI;
    for (MachineBasicBlock::succ_iterator SI = MBBI->succ_begin(), SI_E = MBBI->succ_end(); SI != SI_E; ++SI) {
      fileCFG << "\t" << MBBI->getNumber() << " -> " << (*SI)->getNumber() << ";\n";
    }
  }

  // Print the LLVM Basic Blocks
  for (Function::iterator BBI = _LLVM_F->begin(), BBI_E = _LLVM_F->end(); BBI != BBI_E; ++BBI) {
    fileBB << *BBI;
    name1 = BBI->getNameStr();
    size_t pos = -1;
    while ((pos = name1.find(".", pos + 1)) != std::string::npos) name1.replace(pos, 1, "_");
    fileCFG << "\tsubgraph cluster_" << name1 << ";\n";
  }
  fileCFG << "}\n";

  fileCFG.close();
  fileBB.close();
  fileMBB.close();

  return (true);
}

void MachineTargetAnnotation::formatDotNode(std::string *str)
{
  size_t pos = -1;
  while ((pos = str->find(".", pos + 1)) != std::string::npos)
  {
    str->erase(pos,1);
    str->insert(pos, "_");
  }
}

void MachineTargetAnnotation::formatDotLabel(std::string *str)
{
  std::string _str;

  size_t pos = -1;
  size_t prev_pos = 0;
  while ((pos = str->find("\n", pos + 1)) != std::string::npos)
  {
    if((pos-prev_pos) <= 40)
      _str.append(str->substr(prev_pos,(pos-prev_pos)));
    else
      _str.append(str->substr(prev_pos,40));
    _str.append("\\n");
    prev_pos = pos + 1;
  }

  *str = _str;
}

void MachineTargetAnnotation::printDualCFG() {
  std::fstream              fileCFG;
  std::string               fileName;
  std::string               LabelNameSource, LabelNameSucc;
  std::string               NodeNameSource, NodeNameSucc;
  //std::stringstream         *ASMostr;
  std::string               ASMstr;
  BasicBlock                *SuccBB;
  std::map< const BasicBlock*, std::string > basicblockMap;
  const TerminatorInst      *Term;

  fileName = _Function->getNameStr() + "_dual_CFG" + ".dot";
  fileCFG.open(fileName.data(), std::fstream::out);

  fileCFG << "digraph module_graph {\n";
  fileCFG << "subgraph cluster_Machine {\n";
  fileCFG << "\tlabel = \"cluster_Machine\";\n";
  fileCFG << "\tnode [style=filled];\n";

  // Print All Nodes in Machine Function
  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); BBI != E; ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
      NodeNameSource = BBI->getBasicBlock()->getNameStr();
    else
      NodeNameSource = "noname_" + BBI->getNumber();

    LabelNameSource = NodeNameSource;
    formatDotNode(&NodeNameSource);

    //    ASMostr = new std::stringstream();
    //    *ASMostr << *BBI;
    //    ASMstr = ASMostr->str();
    //    formatDotLabel(&ASMstr);
    fileCFG << "\t node M_" << NodeNameSource << "[shape=record, labeljust=l , label=\"{" << LabelNameSource <</* "|" << ASMstr <<*/ "}\"];\n";
    //    delete ASMostr;
  }
  fileCFG << "}\n";


  fileCFG << "subgraph cluster_LLVM {\n";
  fileCFG << "\tlabel = \"cluster_LLVM\";\n";
  fileCFG << "\tnode [style=filled];\n";
  // Print All Nodes in LLVM Function
  for (Function::const_iterator BBI = _Function->begin(), E = _Function->end(); BBI != E; ++BBI)
  {
    LabelNameSource = BBI->getNameStr();
    NodeNameSource = LabelNameSource;
    formatDotNode(&NodeNameSource);
    //    ASMostr = new std::stringstream();
    //    *ASMostr << *BBI;
    //    ASMstr = ASMostr->str();
    //    formatDotLabel(&ASMstr);
    fileCFG << "\t node " << NodeNameSource << "[shape=record, labeljust=l , label=\"{" << LabelNameSource <</* "|" << ASMstr <<*/ "}\"];\n";
    //    delete ASMostr;
    basicblockMap[&(*BBI)] = NodeNameSource;
  }

  fileCFG << "}\n";

  // Print Relationships between LLVM Basic Blocks
  for (Function::const_iterator BBI = _Function->begin(), E = _Function->end(); BBI != E; ++BBI)
  {
    NodeNameSource = BBI->getNameStr();
    formatDotNode(&NodeNameSource);
    Term = BBI->getTerminator();
    for(unsigned int i = 0 ; i < Term->getNumSuccessors() ; i++)
    {
      SuccBB = Term->getSuccessor(i);
      NodeNameSucc = SuccBB->getNameStr();
      formatDotNode(&NodeNameSucc);

      fileCFG << "\t" << NodeNameSource << " -> " << NodeNameSucc << ";\n";
    }
  }

  // Print Relationships between Machine Basic Blocks
  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); BBI != E; ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
      NodeNameSource = BBI->getBasicBlock()->getNameStr();
    else
      NodeNameSource = "noname_" + BBI->getNumber();

    formatDotNode(&NodeNameSource);

    for(MachineBasicBlock::const_succ_iterator SI = BBI->succ_begin(), SI_E = BBI->succ_end(); SI != SI_E; ++SI)
    {
      if((*SI)->getBasicBlock() != NULL)
        NodeNameSucc = (*SI)->getBasicBlock()->getNameStr();
      else
        NodeNameSucc = "noname_" + (*SI)->getNumber();

      formatDotNode(&NodeNameSucc);
      fileCFG << "\tM_" << NodeNameSource << " -> M_" << NodeNameSucc << ";\n";
    }
  }

  // Mapping between LLVM Basic Blocks and Machine Basic Blocks
  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); BBI != E; ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
      NodeNameSource = BBI->getBasicBlock()->getNameStr();
    else
      NodeNameSource = "noname_" + BBI->getNumber();
    
    formatDotNode(&NodeNameSource);
    basicblockMap.erase(BBI->getBasicBlock());

    fileCFG << "\t" << NodeNameSource << " -> M_" << NodeNameSource << " [color=gray,arrowtail=dot, arrowhead=dot];\n";
  }

  // Branch Penalty Related ... Needs Elaboration !!!
  for(std::map<const BasicBlock*,std::string>::const_iterator BBI = basicblockMap.begin(), E = basicblockMap.end() ; BBI != E ; ++BBI)
  {
    if( (*BBI).second.find("penalty") != std::string::npos ) {
      fileCFG << "\tnode " << (*BBI).second << "[color=green];\n";
    }
    else {
      fileCFG << "\tnode " << (*BBI).second << "[color=red];\n";
    }
  }

  // For the Cases where we don't find a match between BBs and MBBs.
  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); BBI != E; ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
    {
      std::list< std::string > _lst;
      std::string              _name;
      if(!check((BasicBlock*)BBI->getBasicBlock(),BBI, &_lst))
      {
        fileCFG << "\tnode " << BBI->getBasicBlock()->getNameStr() << "[color=red];\n";
        while(_lst.empty() == false)
        {
          _name=_lst.front();
          _lst.pop_front();
          fileCFG << "\t" << BBI->getBasicBlock()->getNameStr() << " -> " << _name << " [color=red];\n";
        }
      }
    }
  }


  fileCFG << "subgraph cluster_LLVM;\n";
  fileCFG << "subgraph cluster_Machine;\n";
  fileCFG << "}\n";
  fileCFG.close();
}

bool MachineTargetAnnotation::check(BasicBlock* BB, MachineBasicBlock *MBB, std::list< std::string > *lst) {
  bool                   result = true;
  BasicBlock                *SuccBB;
  BasicBlock                *ptrBB;
  std::map< BasicBlock*, BasicBlock* > bbMapSucc;
  std::map< BasicBlock*, BasicBlock* > mbbMapSucc;
  std::map< BasicBlock*, BasicBlock* >::iterator bbMapSuccIt;
  const TerminatorInst      *Term;

  DOUT <<  _Function->getNameStr() << ": BB = " << BB->getNameStr() << "\n";

  Term = BB->getTerminator();
  if(Term == NULL) return(true);
  for(unsigned int i = 0 ; i < Term->getNumSuccessors() ; i++)
  {
    SuccBB = Term->getSuccessor(i);
    if( SuccBB->getNameStr().find("penalty") != std::string::npos ) {
      // Skip penalty branch
      SuccBB = SuccBB->getTerminator()->getSuccessor(0);
    }
    bbMapSucc[SuccBB] = SuccBB;
    DOUT << "    - " << SuccBB->getNameStr() << "\n";
  }

  DOUT << "    ---------------- " << "\n";

  for(MachineBasicBlock::succ_iterator SI = MBB->succ_begin(), SI_E = MBB->succ_end();
      SI != SI_E;
      ++SI)
  {
    if((*SI)->getBasicBlock() != NULL)
    {
      SuccBB = (BasicBlock*)(*SI)->getBasicBlock();
      if( SuccBB->getNameStr().find("penalty") != std::string::npos ) {
        // Skip penalty branch
        SuccBB = SuccBB->getTerminator()->getSuccessor(0);
      }
      mbbMapSucc[SuccBB] = SuccBB;
      DOUT << "    - " << SuccBB->getNameStr() << "\n";
    }
  }

  if(bbMapSucc.size() != mbbMapSucc.size()) result = false;
  for(std::map< BasicBlock*, BasicBlock* >::iterator BBI = mbbMapSucc.begin(), E = mbbMapSucc.end() ; BBI != E ; ++BBI)
  {
    ptrBB= (*BBI).second;
    bbMapSuccIt = bbMapSucc.find(ptrBB);
    if( bbMapSuccIt ==  bbMapSucc.end()) 
      result = false;
    else
      bbMapSucc.erase(bbMapSuccIt);
  }

  if(bbMapSucc.size() != 0) 
  {
    for(std::map< BasicBlock*, BasicBlock* >::iterator BBI = bbMapSucc.begin(), E = bbMapSucc.end() ; BBI != E ; ++BBI)
    {
      ptrBB= (*BBI).second;
      lst->push_back(ptrBB->getNameStr());
    }
    result = false;;
  }

  return(result);
}
