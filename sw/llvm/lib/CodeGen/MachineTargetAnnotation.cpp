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
#include "llvm/CodeGen/DwarfWriter.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/Target/TargetAsmInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetAnnotationInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h" 
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Mangler.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CFG.h"
#include "llvm/Instructions.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <map>
#include <list>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <cctype>

namespace llvm
{
  bool PrintDualCFG;
  bool BranchPenalty;

  static cl::opt<bool, true> PrintDualCFGOpt("print-dual-cfg", 
	 cl::desc("Print LLVM and Target Machine CFG in the same graph."),
         cl::location(PrintDualCFG), cl::init(false));

  static cl::opt<bool, true> BranchPenaltyOpt("annotate-branch-penalty",
         cl::desc("Insert annotation for mispredicted branch penalty."),
         cl::location(BranchPenalty), cl::init(false));
}

using namespace llvm;

//STATISTIC(EmittedInsts, "Number of machine instrs printed");

#define DDOUT			DOUT << __func__ << ": "

namespace {
  struct VISIBILITY_HIDDEN MachineTargetAnnotation : public MachineFunctionPass {
  static char ID;

  MachineTargetAnnotation(TargetMachine & T): MachineFunctionPass((intptr_t) & ID), _Target(T) {}

  virtual const char *getPassName() const {
    return "Target Annotation pass";
  }

  bool runOnMachineFunction(MachineFunction & F);
  bool doInitialization(Module & M);
  bool doFinalization(Module & M);

  typedef std::list < BasicBlock* > llvmBB_list_t;
  typedef std::list < MachineBasicBlock* > machineBB_list_t;
  typedef std::map < BasicBlock*, machineBB_list_t* > llvmBasicBlockMap_t;

  private:
    unsigned int getInstructionCount(const MachineInstr & MI);

    llvmBasicBlockMap_t * buildLLVMBasicBlockMap(MachineFunction & MF);
    void cleanLLVMBasicBlockMap(llvmBasicBlockMap_t * map);

    bool annotateFunction(MachineFunction & MF);
    bool annotateLLVMBasicBlock(TargetAnnotationDB *MBBinfo, BasicBlock * BB);
    machineBB_list_t * getMBBSuccessors(machineBB_list_t * listMBB);
    llvmBB_list_t * insertAndAnnotateBB(const BasicBlock* pred, machineBB_list_t * succs);

    bool plotGraph(MachineFunction & MF);

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

    std::string      _File_name;
    Function         *_Function;
    MachineFunction  *_MachineFunction;

    bool check(BasicBlock* BB, MachineBasicBlock *MBB, std::list< std::string > *lst);
    void printBoth(); 
    void formatDotLabel(std::string *str);
    void formatDotNode(std::string *str);
  };
  char MachineTargetAnnotation::ID = 0;
} // end of anonymous namespace

FunctionPass *llvm::createMachineTargetAnnotationPass(TargetMachine &T) {
  return new MachineTargetAnnotation(T);
}

/// runOnMachineFunction - 
///
bool MachineTargetAnnotation::doInitialization(Module &M) {
  _Target.InitializeMBBAnnotation();
  return (false);
}

bool MachineTargetAnnotation::doFinalization(Module &M) {
  return (false);
}

bool MachineTargetAnnotation::runOnMachineFunction(MachineFunction &MF) {
  _File_name  = "annotate";
  _MachineFunction= &MF;
  _Function = (Function*)(MF.getFunction());

  // Get current function parent module 
  _LLVM_F = (Function*) MF.getFunction();
  assert(_LLVM_F && "Machine Function must have a Function to annotate!\n");
  _LLVM_M = _LLVM_F->getParent();
  assert(_LLVM_M && "Function must have a parent Module!\n");

  DDOUT << " run on function : " << _LLVM_F -> getName() << " (in " << _LLVM_M -> getModuleIdentifier() << ")" << std::endl;

  //  Constant * c = _LLVM_M->getOrInsertFunction("mbb_annotation",
  //      Type::VoidTy,
  //      PointerType::get(IntegerType::get(8),0),
  //      NULL);

  _Fannotation = cast<Function>( _LLVM_M->getOrInsertFunction("mbb_annotation", Type::VoidTy, PointerType::get(IntegerType::get(8),0), NULL));
  _Fannotation_Entry = cast<Function>( _LLVM_M->getOrInsertFunction("annotation_entry", Type::VoidTy, NULL));
  _Fannotation_Return = cast<Function>( _LLVM_M->getOrInsertFunction("annotation_return", Type::VoidTy, NULL));

  _plotCFG = false;
  annotateFunction(MF);
  if (PrintDualCFG)
  {
    printBoth();
  }

  //if (_plotCFG)
  {
    plotGraph(MF);
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
    if (listMachineBasicBlock->size() > 1) {
      _plotCFG = true;
#if 0
      DDOUT << llvmBasicBlock->getName() << " have multiple childs! (" << listMachineBasicBlock->size() << ")\n";
      DDOUT << *llvmBasicBlock;

//    cerr << "Annotation warning: " << llvmBasicBlock->getName() << " LLVM BasicBlock have multiple corresponding MachineBasicBlock "
//         << "(CFG and LLVM files created)\n";
      _plotCFG = true;

      for (machineBB_list_t::const_iterator MBBI = listMachineBasicBlock->begin(), MBBI_E = listMachineBasicBlock->end(); MBBI != MBBI_E; ++MBBI) {
        machineBasicBlock = (*MBBI);
        assert(machineBasicBlock && "NULL pointer!\n");

        annotationDB = (TargetAnnotationDB*) _Target.MachineBasicBlockAnnotation(*machineBasicBlock);
        if(annotationDB != NULL)
          annotateLLVMBasicBlock(annotationDB, llvmBasicBlock);
      }
#endif
    } else if (listMachineBasicBlock->size() == 1) {
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

            //cout << "MBB # "<< machineBasicBlock->getNumber() << "-->SuccMBB # "<< succMachineBasicBlock->getNumber() << std::endl;

            branchAnnotationDB = (TargetAnnotationDB*) _Target.MBBBranchPenaltyAnnotation(*machineBasicBlock, *succMachineBasicBlock);
            if(branchAnnotationDB != NULL)
            {
              cout  << "Penalty = " << branchAnnotationDB->getCycleCount() << std::endl << std::endl;

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
      if(annotationDB != NULL)
        annotateLLVMBasicBlock(annotationDB, llvmBasicBlock);
    }
  }

  cleanLLVMBasicBlockMap(llvmBasicBlockMap);

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

  // Create LLVM function basic block map
  llvmFunction = (Function*) MF.getFunction();
  for (Function::iterator BBI = llvmFunction->begin(), E = llvmFunction->end(); BBI != E; ++BBI) {
    assert((llvmBasicBlockMap->count(BBI) == 0) && "LLVM BasicBlock already in the map!");
    (*llvmBasicBlockMap)[BBI] = new machineBB_list_t;
    assert((*llvmBasicBlockMap)[BBI] && "NULL pointer!\n");
  }

  unsigned int counter = 0;
  for (MachineFunction::iterator BBI = MF.begin(), E = MF.end(); BBI != E; ++BBI) {
    llvmBasicBlock = (BasicBlock*) BBI->getBasicBlock();
    if (llvmBasicBlock != NULL) {
      counter++;
    }
  }
  if (counter != llvmBasicBlockMap->size()) 
    DDOUT << "Number of BasicBlock and MachineMachineBlock doesn't match! " << llvmBasicBlockMap->size() << ":" << counter << "\n";

  // Create Machine function basic block map
  for (MachineFunction::iterator BBI = MF.begin(), E = MF.end(); BBI != E; ++BBI) {
    llvmBasicBlock = (BasicBlock*) BBI->getBasicBlock();
    if (llvmBasicBlock != NULL) {
      listMachineBasicBlock = (*llvmBasicBlockMap)[llvmBasicBlock];
      listMachineBasicBlock->push_back(BBI);
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
  //Value *mBBValue;

  assert(MBBinfo && "NULL pointer!\n");
  assert(BB && "NULL pointer!\n");

  std::vector < Constant * > DumpValues;
  char * dump = MBBinfo->Dump();

  for( unsigned int i = 0 ; i < MBBinfo->DumpSize() ; i++){
    DumpValues.push_back(ConstantInt::get(IntegerType::get(8), dump[i]));
  }

  std::string  VarName = BB->getName() + "_db";
  GlobalVariable * DumpVar = new GlobalVariable(ArrayType::get(IntegerType::get(8),MBBinfo->DumpSize()),
      true,
      GlobalValue::InternalLinkage,
      ConstantArray::get(ArrayType::get(IntegerType::get(8),MBBinfo->DumpSize()), DumpValues),
      VarName.data(),
      _LLVM_M
      );

  std::vector<Constant*> const_ptr_7_indices;
  Constant* const_int32_8 = Constant::getNullValue(IntegerType::get(32));
  const_ptr_7_indices.push_back(const_int32_8);
  const_ptr_7_indices.push_back(const_int32_8);
  Constant* GEP = ConstantExpr::getGetElementPtr(DumpVar, &const_ptr_7_indices[0], const_ptr_7_indices.size() );

  // Get the first valide instruction for the insertion of the call
  BasicBlock::iterator BBI = ((BasicBlock*) BB)->begin();
  while (isa<PHINode > (BBI)) BBI++;

  /* MMH: Comment this to have different calls to Entry/Return BBs */
  CallInst*  annotationCall= CallInst::Create(_Fannotation, GEP, "");
  assert(annotationCall	&& "NULL pointer!\n");
  BB->getInstList().insert(BBI, annotationCall);
  
  /* MMH: Un-comment this to have different calls to Entry/Return BBs */
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

bool MachineTargetAnnotation::plotGraph(MachineFunction &MF) {
  std::fstream fileCFG;
  std::fstream fileBB;
  std::fstream fileMBB;
  std::string fileName;
  std::string name1, name2;
  BasicBlock* BB;
  MachineBasicBlock* MBB;
  machineBB_list_t* listMachineBasicBlock;
  llvmBasicBlockMap_t *llvmBasicBlockMap;
  TargetAnnotationDB  * annotationDB;

  assert(_LLVM_F && "Machine Function must have a Function to annotate!\n");

  fileName = "CFG_" + _LLVM_F->getName() + ".dot";
  fileCFG.open(fileName.data(), std::fstream::out);
  fileName = "BB_" + _LLVM_F->getName() + ".bb";
  fileBB.open(fileName.data(), std::fstream::out);
  fileName = "MBB_" + _LLVM_F->getName() + ".mbb";
  fileMBB.open(fileName.data(), std::fstream::out);

  fileCFG << "digraph module_graph {\n";

  llvmBasicBlockMap = buildLLVMBasicBlockMap(MF);
  for (llvmBasicBlockMap_t::const_iterator BBI = llvmBasicBlockMap->begin(), E = llvmBasicBlockMap->end(); BBI != E; ++BBI) {
    BB = (*BBI).first;
    name1 = BB->getName();
    size_t pos = -1;
    while ((pos = name1.find(".", pos + 1)) != std::string::npos) name1.replace(pos, 1, "_");

    listMachineBasicBlock = (*llvmBasicBlockMap)[(*BBI).first];
    if (listMachineBasicBlock->size() >= 1) {

      fileCFG << "\tsubgraph cluster_" << name1 << " {\n";
      fileCFG << "\t\tlabel = \"" << name1 << "\";\n";
      fileCFG << "\t\tnode [style=filled];\n";
      if (listMachineBasicBlock->size() > 1) {
        fileCFG << "\t\tstyle=filled;\n";
        fileCFG << "\t\tcolor=red;\n";
      }
      while (!listMachineBasicBlock->empty()) {
        MBB = listMachineBasicBlock->front();
        listMachineBasicBlock->pop_front();
        annotationDB = (TargetAnnotationDB*) _Target.MachineBasicBlockAnnotation(*MBB);
        if( MBB->succ_begin() ==  MBB->succ_end() )
        {
          fileCFG << "\t\tnode " << MBB->getNumber() << " [label = \"return_#" << MBB->getNumber() << "(" << ((annotationDB != NULL) ? annotationDB->getInstructionCount() : -1) << ")" <<"\"];\n";
        }
        else if( MBB->pred_begin() ==  MBB->pred_end() )
        {
          fileCFG << "\t\tnode " << MBB->getNumber() << " [label = \"entry_#" << MBB->getNumber() << "(" << ((annotationDB != NULL) ? annotationDB->getInstructionCount() : -1) << ")" << "\"];\n";
        }
        else
        {
          fileCFG << "\t\tnode " << MBB->getNumber() << " [label = \"_#" << MBB->getNumber() << "(" << ((annotationDB != NULL) ? annotationDB->getInstructionCount() : -1) << ")" << "\"];\n";
        }
      }
      fileCFG << "\t}\n";
    }
  }
  cleanLLVMBasicBlockMap(llvmBasicBlockMap);

  for (MachineFunction::iterator MBBI = MF.begin(), MBBI_E = MF.end(); MBBI != MBBI_E; ++MBBI) {
    fileMBB << *MBBI;
    for (MachineBasicBlock::succ_iterator SI = MBBI->succ_begin(), SI_E = MBBI->succ_end(); SI != SI_E; ++SI) {
      fileCFG << "\t" << MBBI->getNumber() << " -> " << (*SI)->getNumber() << ";\n";
    }
  }

  for (Function::iterator BBI = _LLVM_F->begin(), BBI_E = _LLVM_F->end(); BBI != BBI_E; ++BBI) {
    fileBB << *BBI;
    name1 = BBI->getName();
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

void MachineTargetAnnotation::printBoth() {
  std::fstream              fileCFG;
  std::string               fileName;
  std::string               LabelNameSource, LabelNameSucc;
  std::string               NodeNameSource, NodeNameSucc;
  //std::stringstream         *ASMostr;
  std::string               ASMstr;
  BasicBlock                *SuccBB;
  std::map< const BasicBlock*, std::string > basicblockMap;
  const TerminatorInst      *Term;

  fileName = "CFG_" + _Function->getName() + "_" + _File_name + ".dot";
  fileCFG.open(fileName.data(), std::fstream::out);

  fileCFG << "digraph module_graph {\n";

  fileCFG << "subgraph cluster_Machine {\n";
  fileCFG << "\tlabel = \"cluster_Machine\";\n";
  fileCFG << "\tnode [style=filled];\n";

  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); 
      BBI != E; 
      ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
      NodeNameSource = BBI->getBasicBlock()->getName();
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
  for (Function::const_iterator BBI = _Function->begin(), E = _Function->end();
      BBI != E; 
      ++BBI) 
  {
    LabelNameSource = BBI->getName();
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

  for (Function::const_iterator BBI = _Function->begin(), E = _Function->end();
      BBI != E; 
      ++BBI) 
  {
    NodeNameSource = BBI->getName();
    formatDotNode(&NodeNameSource);
    Term = BBI->getTerminator();
    for(unsigned int i = 0 ; i < Term->getNumSuccessors() ; i++)
    {
      SuccBB = Term->getSuccessor(i);
      NodeNameSucc = SuccBB->getName();
      formatDotNode(&NodeNameSucc);

      fileCFG << "\t" << NodeNameSource << " -> " << NodeNameSucc << ";\n";
    }
  }

  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); 
      BBI != E; 
      ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
      NodeNameSource = BBI->getBasicBlock()->getName();
    else
      NodeNameSource = "noname_" + BBI->getNumber();
    formatDotNode(&NodeNameSource);

    for(MachineBasicBlock::const_succ_iterator SI = BBI->succ_begin(), SI_E = BBI->succ_end();
        SI != SI_E;
        ++SI)
    {
      if((*SI)->getBasicBlock() != NULL)
        NodeNameSucc = (*SI)->getBasicBlock()->getName();
      else
        NodeNameSucc = "noname_" + (*SI)->getNumber();

      formatDotNode(&NodeNameSucc);

      fileCFG << "\tM_" << NodeNameSource << " -> M_" << NodeNameSucc << ";\n";
    }
  }

  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); 
      BBI != E; 
      ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
      NodeNameSource = BBI->getBasicBlock()->getName();
    else
      NodeNameSource = "noname_" + BBI->getNumber();
    formatDotNode(&NodeNameSource);
    basicblockMap.erase(BBI->getBasicBlock());

    fileCFG << "\t" << NodeNameSource << " -> M_" << NodeNameSource << " [color=gray,arrowtail=dot, arrowhead=dot];\n";
  }

  for(std::map<const BasicBlock*,std::string>::const_iterator BBI = basicblockMap.begin(), E = basicblockMap.end() ; BBI != E ; ++BBI)
  {
    if( (*BBI).second.find("penalty") != std::string::npos ) {
      fileCFG << "\tnode " << (*BBI).second << "[color=green];\n";
    }
    else {
      fileCFG << "\tnode " << (*BBI).second << "[color=red];\n";
    }
  }

  for (MachineFunction::iterator BBI = _MachineFunction->begin(), E = _MachineFunction->end(); 
      BBI != E; 
      ++BBI) 
  {
    if(BBI->getBasicBlock() != NULL)
    {
      std::list< std::string > _lst;
      std::string              _name;
      if(!check((BasicBlock*)BBI->getBasicBlock(),BBI, &_lst))
      {
        fileCFG << "\tnode " << BBI->getBasicBlock()->getName() << "[color=red];\n";
        while(_lst.empty() == false)
        {
          _name=_lst.front();
          _lst.pop_front();
          fileCFG << "\t" << BBI->getBasicBlock()->getName() << " -> " << _name << " [color=red];\n";
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

  cout <<  _Function->getName() << ": BB = " << BB->getName() << "\n";

  Term = BB->getTerminator();
  if(Term == NULL) return(true);
  for(unsigned int i = 0 ; i < Term->getNumSuccessors() ; i++)
  {
    SuccBB = Term->getSuccessor(i);
    if( SuccBB->getName().find("penalty") != std::string::npos ) {
      // Skip penalty branch
      SuccBB = SuccBB->getTerminator()->getSuccessor(0);
    }
    bbMapSucc[SuccBB] = SuccBB;
    cout << "    - " << SuccBB->getName() << "\n";
  }

  cout << "    ---------------- " << "\n";

  for(MachineBasicBlock::succ_iterator SI = MBB->succ_begin(), SI_E = MBB->succ_end();
      SI != SI_E;
      ++SI)
  {
    if((*SI)->getBasicBlock() != NULL)
    {
      SuccBB = (BasicBlock*)(*SI)->getBasicBlock();
      if( SuccBB->getName().find("penalty") != std::string::npos ) {
        // Skip penalty branch
        SuccBB = SuccBB->getTerminator()->getSuccessor(0);
      }
      mbbMapSucc[SuccBB] = SuccBB;
      cout << "    - " << SuccBB->getName() << "\n";
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
      lst->push_back(ptrBB->getName());
    }
    result = false;;
  }

  return(result);
}
