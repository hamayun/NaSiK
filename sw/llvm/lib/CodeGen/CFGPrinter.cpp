//===--  CFGPrinter.cpp - print llvm and machine basic block CFG  ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "cfgprinter"
#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineJumpTableInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Function.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/BasicBlock.h"
#include "llvm/Instructions.h"
#include <algorithm>
#include <iosfwd>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
using namespace llvm;

static cl::opt<bool>
EnableCFGPrinter("enable-cfg-printer",
    cl::init(false), cl::Hidden);

namespace {
  struct CFGPrinter : public MachineFunctionPass {
    static char ID;

    CFGPrinter(const std::string &Name) :
      MachineFunctionPass(ID), _File_name(Name)
    {}

    virtual bool runOnMachineFunction(MachineFunction &MF);
    virtual const char *getPassName() const { return "CFG Printer"; }

    private:
    std::string      _File_name;
    Function         *_Function;
    MachineFunction  *_MachineFunction;


    private:
    void printBoth(); 
    void formatDotLabel(std::string *str);
    void formatDotNode(std::string *str);

  };
  char CFGPrinter::ID = 0;

}

FunctionPass *llvm::createCFGPrinterPass(const std::string &Name) {
  return new CFGPrinter(Name); 
}

bool CFGPrinter::runOnMachineFunction(MachineFunction &MF) {

  _MachineFunction= &MF;
  _Function = (Function*)(MF.getFunction());
  assert(_Function && "NULL pointer!\n");

  if(EnableCFGPrinter)
  {
    printBoth(); 
  }

  return false;
}

void CFGPrinter::formatDotNode(std::string *str)
{
  size_t pos = -1;
  while ((pos = str->find(".", pos + 1)) != std::string::npos)
  {
    str->erase(pos,1);
    str->insert(pos, "_");
  }
}

void CFGPrinter::formatDotLabel(std::string *str)
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

void CFGPrinter::printBoth() {
  std::fstream              fileCFG;
  std::string               fileName;
  std::string               LabelNameSource, LabelNameSucc;
  std::string               NodeNameSource, NodeNameSucc;
  std::stringstream         *ASMostr;
  std::string               ASMstr;
  const TerminatorInst      *Term;
  BasicBlock                *SuccBB;
  std::map< const BasicBlock*, std::string > basicblockMap;

  fileName = "CFG_" + _Function->getNameStr() + "_" + _File_name + ".dot";
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
      fileCFG << "\tnode " << (*BBI).second << "[color=blue];\n";
    }
  }

  fileCFG << "subgraph cluster_LLVM;\n";
  fileCFG << "subgraph cluster_Machine;\n";
  fileCFG << "}\n";
  fileCFG.close();
}
