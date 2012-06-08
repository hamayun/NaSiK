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
rm -f $(find $NASIK_HOME/examples/applications/kvmMiBench/ -name "compose.log")
rm -f $NASIK_HOME/examples/applications/kvmMiBench/automotive/bitcount/BITCOUNT
rm -f $NASIK_HOME/examples/applications/kvmMiBench/automotive/qsort/QSORT
rm -f $NASIK_HOME/examples/applications/kvmMiBench/automotive/susan/SUSAN
rm -f $NASIK_HOME/examples/applications/kvmMiBench/consumer/cjpeg/CJPEG
rm -f $NASIK_HOME/examples/applications/kvmMiBench/consumer/djpeg/DJPEG
rm -f $NASIK_HOME/examples/applications/kvmMiBench/network/dijkstra/DIJKSTRA
rm -f $NASIK_HOME/examples/applications/kvmMiBench/network/patricia/PATRICIA
rm -f $NASIK_HOME/examples/applications/kvmMiBench/office/stringsearch/STRINGSEARCH
rm -f $NASIK_HOME/examples/applications/kvmMiBench/security/blowfish/BLOWFISH
rm -f $NASIK_HOME/examples/applications/kvmMiBench/security/rijndael/RIJNDAEL
rm -f $NASIK_HOME/examples/applications/kvmMiBench/security/sha/SHA
rm -f $NASIK_HOME/examples/applications/kvmMiBench/telecomm/CRC32/CRC32APP

print_substep "Cleaning Hardware Model ... "
cd ${PFORM_DIR}
make clean 

rm -f $NASIK_HOME/tools/fbviewer $NASIK_HOME/tools/tty_term_rw

print_step "Environment Cleanup Done !!!"

