//===-- SparcTargetAnnotation.cpp - Sparc LLVM assembly writer ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains a printer that converts from our internal representation
// of machine-dependent LLVM code to GAS-format SPARC assembly language.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "asm-printer"
#include "Sparc.h"
#include "SparcInstrInfo.h"
#include "SparcTargetMachine.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Target/TargetAsmInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetAnnotationInfo.h"
#include "llvm/Support/Mangler.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#include <cctype>
#include <cstring>
#include <iostream>
#include <map>

using namespace llvm;

namespace SparcTargetAnnotation {

	unsigned int getInstructionCount(const MachineInstr &MI);

} // end of ARMTargetAnnotation namespace

const TargetAnnotationDB * SparcTargetMachine::MachineBasicBlockAnnotation(MachineBasicBlock &MBB)
{
	TargetAnnotationDB  * annotationDB = NULL;
	unsigned int          instCount;

	instCount = 0;
	for (MachineBasicBlock::const_iterator II = MBB.begin(), II_E = MBB.end(); II != II_E; ++II)
	{
		instCount += SparcTargetAnnotation::getInstructionCount(*II);
	}

  if( instCount > 0)
  {
    annotationDB = new TargetAnnotationDB();
    assert( annotationDB && "NULL pointer!\n" );
    annotationDB->setType( (MBB.pred_empty() ? MBB_ENTRY:0) | (MBB.succ_empty() ? MBB_RETURN:0) );
    annotationDB->setInstructionCount(instCount);
  }

  return(annotationDB);

}

unsigned int SparcTargetAnnotation::getInstructionCount(const MachineInstr &MI)
{
  unsigned int instCount = 0;

  switch(MI.getOpcode())
  {
    default:
      instCount = 1;
      break;
  }
  return(instCount);
}

