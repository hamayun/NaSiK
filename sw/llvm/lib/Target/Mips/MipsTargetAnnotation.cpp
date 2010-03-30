/*************************************************************************************
 * File   : MipsTargetAnnotation.cpp,     
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

#include "Mips.h"
#include "MipsSubtarget.h"
#include "MipsInstrInfo.h"
#include "MipsTargetMachine.h"
#include "MipsMachineFunction.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Module.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/Target/TargetAsmInfo.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetAnnotationInfo.h"
#include "llvm/Support/Mangler.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#include <cctype>

using namespace llvm;

namespace MipsTargetAnnotation {

	unsigned int getInstructionCount(const MachineInstr &MI);

} // end of ARMTargetAnnotation namespace

const TargetAnnotationDB * MipsTargetMachine::MachineBasicBlockAnnotation(MachineBasicBlock &MBB)
{
	TargetAnnotationDB  * annotationDB;
	unsigned int          instCount;

	annotationDB = new TargetAnnotationDB();
	assert( annotationDB && "NULL pointer!\n" );

	instCount = 0;
	for (MachineBasicBlock::const_iterator II = MBB.begin(), II_E = MBB.end(); II != II_E; ++II)
	{
		instCount += MipsTargetAnnotation::getInstructionCount(*II);
	}
	annotationDB->setInstructionCount(instCount);

	return(annotationDB);

}

unsigned int MipsTargetAnnotation::getInstructionCount(const MachineInstr &MI)
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

