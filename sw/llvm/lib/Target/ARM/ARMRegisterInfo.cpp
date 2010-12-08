//===- ARMRegisterInfo.cpp - ARM Register Information -----------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the ARM implementation of the TargetRegisterInfo class.
//
//===----------------------------------------------------------------------===//

#include "ARM.h"
#include "ARMAddressingModes.h"
#include "ARMBaseInstrInfo.h"
#include "ARMInstrInfo.h"
#include "ARMMachineFunctionInfo.h"
#include "ARMRegisterInfo.h"
#include "ARMSubtarget.h"
#include "llvm/Constants.h"
#include "llvm/DerivedTypes.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineLocation.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterScavenging.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Target/TargetFrameInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/SmallVector.h"
using namespace llvm;

ARMRegisterInfo::ARMRegisterInfo(const ARMBaseInstrInfo &tii,
                                 const ARMSubtarget &sti)
  : ARMBaseRegisterInfo(tii, sti) {
}

unsigned ARMRegisterInfo::getRegisterNumbering(unsigned RegEnum) {
  using namespace ARM;
  switch (RegEnum) {
  case R0:  case S0:  case D0:  return 0;
  case R1:  case S1:  case D1:  return 1;
  case R2:  case S2:  case D2:  return 2;
  case R3:  case S3:  case D3:  return 3;
  case R4:  case S4:  case D4:  return 4;
  case R5:  case S5:  case D5:  return 5;
  case R6:  case S6:  case D6:  return 6;
  case R7:  case S7:  case D7:  return 7;
  case R8:  case S8:  case D8:  return 8;
  case R9:  case S9:  case D9:  return 9;
  case R10: case S10: case D10: return 10;
  case R11: case S11: case D11: return 11;
  case R12: case S12: case D12: return 12;
  case SP:  case S13: case D13: return 13;
  case LR:  case S14: case D14: return 14;
  case PC:  case S15: case D15: return 15;
  case S16: return 16;
  case S17: return 17;
  case S18: return 18;
  case S19: return 19;
  case S20: return 20;
  case S21: return 21;
  case S22: return 22;
  case S23: return 23;
  case S24: return 24;
  case S25: return 25;
  case S26: return 26;
  case S27: return 27;
  case S28: return 28;
  case S29: return 29;
  case S30: return 30;
  case S31: return 31;
  default:
    assert(0 && "Unknown ARM register!");
    abort();
  }
}
