//===-- TargetAnnotationInfo.cpp - Target Annotation Database Info ---------------------==//

#include "llvm/Target/TargetAnnotationInfo.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include <string>
#include <fstream>
#include <unistd.h>


using namespace llvm;

//---------------------------------------------------------------------------
// TargetAnnotationDB Class
//

TargetAnnotationDB::TargetAnnotationDB()
{
	memset(_db._dump, 0x00, sizeof(_db));
        _db._struct._Type = MBB_DEFAULT;
}

TargetAnnotationDB::~TargetAnnotationDB() 
{}

unsigned int TargetAnnotationDB::DumpSize() {
	return((unsigned int)sizeof(_db));
}

char * TargetAnnotationDB::Dump() {
	return(_db._dump);
}

void TargetAnnotationDB::setInstructionCount(unsigned int value) {
	_db._struct._InstructionCount = value;
}

void TargetAnnotationDB::setCycleCount(unsigned int value) {
	_db._struct._CycleCount = value;
}

void TargetAnnotationDB::setLoadCount(unsigned int value) {
	_db._struct._LoadCount = value;
}

void TargetAnnotationDB::setStoreCount(unsigned int value) {
	_db._struct._StoreCount = value;
}

void TargetAnnotationDB::setType(unsigned int value) {
	_db._struct._Type = value;
}

unsigned int TargetAnnotationDB::getInstructionCount() {
	return (_db._struct._InstructionCount);
}

unsigned int TargetAnnotationDB::getCycleCount() {
	return (_db._struct._CycleCount);
}

unsigned int TargetAnnotationDB::getLoadCount() {
	return (_db._struct._LoadCount);
}

unsigned int TargetAnnotationDB::getStoreCount() {
	return (_db._struct._StoreCount);
}

unsigned int TargetAnnotationDB::getType() {
	return (_db._struct._Type);
}

