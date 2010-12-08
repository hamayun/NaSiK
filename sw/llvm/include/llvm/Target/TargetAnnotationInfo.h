//===-- llvm/Target/TargetAnnotationInfo.h - Target Information -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the TargetMachine and LLVMTargetMachine classes.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_TARGETANNOTATIONINFO_H
#define LLVM_TARGET_TARGETANNOTATIONINFO_H

namespace llvm {

class TargetAnnotationDB;

typedef enum {
    MBB_DEFAULT = 0,
    MBB_ENTRY = 1,
    MBB_RETURN = 2,
    MBB_BRANCHPENALTY=4
} MBB_TYPE;

class TargetAnnotationDB {
  TargetAnnotationDB(const TargetAnnotationDB &);   // DO NOT IMPLEMENT
  void operator=(const TargetAnnotationDB &);       // DO NOT IMPLEMENT

public:
  TargetAnnotationDB();
  ~TargetAnnotationDB();

  void setInstructionCount(unsigned int value);
  void setCycleCount(unsigned int value);
  void setLoadCount(unsigned int value);
  void setStoreCount(unsigned int value);
  void setType(unsigned int value);

  unsigned int getInstructionCount();
  unsigned int getCycleCount();
  unsigned int getLoadCount();
  unsigned int getStoreCount();
  unsigned int getType();

  TargetAnnotationDB * addDB(TargetAnnotationDB * pDB); 
  unsigned int DumpSize();
  char * Dump();

  typedef struct {
    unsigned int  _Type;
    unsigned int  _InstructionCount;
    unsigned int  _CycleCount;
    unsigned int  _LoadCount;
    unsigned int  _StoreCount;
  } db_struct;

private:
  union {
    db_struct     _struct;
    char          _dump[sizeof(db_struct)];
  } _db;
};

} // End llvm namespace
#endif



