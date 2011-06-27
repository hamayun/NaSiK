#!/bin/bash

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

print_step "Compiling the LIBKVM"
cd ${LIBKVM_HOME}
./configure --arch=i386 --prefix=${LIBKVM_PREFIX}
cd ${LIBKVM_HOME}/libkvm 
make && make install-libkvm
if [ $? != 0 ]; then
    print_error "Error: LIBKVM compilation failed"
    exit 2 
fi

print_step "Compiling bootstrap & dnastart.o"
cd ${LIBKVM_HOME}/user
make  

print_step "Compiling the LIBSOCKVM"
cd ${LIBSOCKVM_HOME}
make && make install 
if [ $? != 0 ]; then
    print_error "Compilation Failed for LIBSOCKVM"
    exit 3 
fi

print_step "Compiling Software Application ... ${APPLICATION}"
cd ${APP_DIR}
apes-compose
if [ $? != 0 ]; then
    print_error "Compilation Failed for Application"
    exit 3 
fi

print_step "Compiling Platform Model ... ${PLATFORM}"
cd ${PFORM_DIR}
make -j2
if [ $? != 0 ]; then
    print_error "Compilation Failed for Hardware Model"
    exit 4 
fi

print_step "Simulation Compiled Successfully !!!" 

