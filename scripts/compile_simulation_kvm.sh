#!/bin/bash

HERE=`pwd`
NB_PROC=`cat /proc/cpuinfo | grep "processor" | wc -l`
JOPT=-j${NB_PROC}

print_step()
{
    echo "======================================================================"
    echo -en "\033[01;32m"
    echo " "$1
    echo -en "\033[00m"
    echo "======================================================================"
}

print_error()
{
    echo -en "\033[01;31m"
    echo "!--------------------------------------------------------------------!"
    echo "! "$1
    echo "!--------------------------------------------------------------------!"
    echo -en " \033[00m"
    cd $NASIK_HOME
}

if [ -z $NASIK_HOME ] ; then
    print_error "Environment Variables Undefined; Run the Configure Script First !!!"
    exit 1 
fi

# Platform is tuzki, so we need libKVM and DNA Start Files
if [ ${PLATFORM} == "tuzki" ]; then
	print_step "Compiling the LibKVM Library"
	cd ${LIBKVM_HOME}
	./configure --arch=i386 --prefix=${LIBKVM_PREFIX}
	cd ${LIBKVM_HOME}/libkvm 
	make && make install-libkvm
	if [ $? != 0 ]; then
	    print_error "Error: LIBKVM compilation failed"
    	exit 2 
	fi

	print_step "Compiling Bootstrap and DNAStart Objects ..."
	cd ${LIBKVM_HOME}/user
	make  

	print_step "Compiling the LIBSOCKVM"
	cd ${LIBSOCKVM_HOME}
	make && make install 
	if [ $? != 0 ]; then
	    print_error "Compilation Failed for LIBSOCKVM"
    	exit 3 
	fi
# Platform is other than tuzki; more recent ones are based on LibKVMTool Library
else
	print_step "Compiling the LibKVMTool Library"
	cd ${LIBKVMTOOL_PREFIX}
	make ${JOPT}

	print_step "Compiling the BootLoader"
	cd ${APES_EXTRA_COMPS}/KVMx86BootLoader/
	make
fi

print_step "Compiling Software Application ... ${APPLICATION}"
cd ${APP_DIR}
apes-compose
if [ $? != 0 ]; then
    print_error "Compilation Failed for Application"
    exit 3 
fi

print_step "Compiling Tools ... "
cd ${NASIK_HOME}/tools
make
if [ $? != 0 ]; then
    print_error "Compilation Failed for tools"
    exit 4 
fi

print_step "Compiling Platform Model ... ${PLATFORM}"
cd ${PFORM_DIR}
make ${JOPT}

if [ $? != 0 ]; then
    print_error "Compilation Failed for Hardware Model"
    exit 5
fi

print_step "Simulation Compiled Successfully !!!" 
cd ${HERE}
