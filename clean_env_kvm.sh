#!/bin/bash

print_step()
{
    echo "======================================================================"
    echo -en "\033[01;32m"
    echo " "$1
    echo -en "\033[00m"
    echo "======================================================================"
}

print_substep()
{
    echo "*** " $1
}

if [ -z $NASIK_HOME ] ; then
	echo "$NASIK_HOME variable is Undefined; Please Run the Configure Script First !!!"
    exit 1
fi

print_step "Cleaning the Build Environment"

print_substep "Cleaning LIBKVM ... "
cd ${LIBKVM_HOME}/libkvm 
make clean

print_substep "Cleaning bootstrap & dnastart.o ... "
cd ${LIBKVM_HOME}/user
make clean

print_substep "Cleaning the LIBSOCKVM ... "
cd ${LIBSOCKVM_HOME}
make clean 

print_substep "Removing Software Application Symlinks ..."
rm -f $(find $NASIK_HOME/examples/applications/kvmMiBench/ -name "interface.xmi")
rm -f $(find $NASIK_HOME/examples/applications/kvmMiBench/ -name "elf.lds")
rm -f $(find $NASIK_HOME/examples/applications/kvmMiBench/ -name "APPLICATION.X.bin")

print_substep "Cleaning Hardware Model ... "
cd ${PFORM_DIR}
make clean 

print_step "Environment Cleanup Done !!!"

