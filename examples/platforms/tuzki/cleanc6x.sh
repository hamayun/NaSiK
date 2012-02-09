#!/bin/bash

print_step()
{
    echo "----------------------------------------------------------------------"
    echo -en "\033[01;32m"
    echo " "$1
    echo -en "\033[00m"
    echo "----------------------------------------------------------------------"
}

print_error()
{
    echo -en "\033[01;31m"
    echo "!--------------------------------------------------------------------!"
    echo "! "$1
    echo "!--------------------------------------------------------------------!"
    echo -en " \033[00m"
}

if [ -z $NASIK_HOME ] ; then
    print_error "Environment Variables Undefined; Run the Configure Script First !!!"
    exit 1 
fi

print_step "Cleaning Binary File Writer ... "
cd ${TARGET_BIN_WRITER}
make clean

print_step "Cleaning the C6x Decoder ... "
cd ${C6XDEC_BUILD}
make clean

print_step "Cleaning ISA Behavior ... "
cd ${C6XISA_BEHAVIOR}
make clean

print_step "Cleaning Bootstrap(s) ... "
cd ${LIBKVM_HOME}/user
make clean

print_step "Cleaning Generated Application ... "
cd ${GENERATED_APP}
make clean 

print_step "Cleaning the LIBKVM"
cd ${LIBKVM_HOME}/libkvm 
make clean

print_step "Cleaning the LIBSOCKVM"
cd ${LIBSOCKVM_HOME}
make clean

print_step "Cleaning the Simulation Platform ... "
cd ${PFORM_DIR}
make clean

print_step "Removing Temporary Links ... "
rm -f ${PFORM_DIR}/kvm_c6x_bootstrap
rm -f ${PFORM_DIR}/target_binary_text
rm -f ${GENERATED_APP}/c6x-decoder
rm -f ${GENERATED_APP}/instructions.bin


