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

print_step "Compiling Binary File Writer ... "
cd ${TARGET_BIN_WRITER}
make clean
make -s
if [ $? != 0 ]; then
    print_error "Error! Compiling Binary File Writer ..."
    exit 1
fi

print_step "Generating Target Binary ... "
./BFW.X
if [ $? != 0 ]; then
    print_error "Error! Generating Target Binary ..."
    exit 1
fi

print_step "Building the C6x Decoder ... "
cd ${C6XDEC_BUILD}
make -s
if [ $? != 0 ]; then
    print_error "Error! Building the C6x Decoder ..."
    exit 1
fi

print_step "Generating ISA Behavior ... "
cd ${C6XISA_BEHAVIOR}
make clean
make -s
if [ $? != 0 ]; then
    print_error "Error! Generating ISA Behavior ... "
    exit 1
fi

if [ -f "${C6XDEC_BUILD}/Debug+Asserts/bin/c6x-decoder" ]
then
    echo "  Debug Build of c6x-decoder Found ... "
    ln -sf ${C6XDEC_BUILD}/Debug+Asserts/bin/c6x-decoder ${GENERATED_APP}/c6x-decoder
else
    echo "  Creating Link for Release Build of c6x-decoder ... "
    ln -sf ${C6XDEC_BUILD}/Release+Asserts/bin/c6x-decoder ${GENERATED_APP}/c6x-decoder
fi

ln -sf ${TARGET_BIN_WRITER}/instructions.bin ${GENERATED_APP}/instructions.bin
ln -sf /home/hamayun/examples/matmult/matmult.coff ${GENERATED_APP}/matmult.coff

if [ "$1" = "kvm" ]; then
print_step "Building Bootstrap(s) ... "
cd ${LIBKVM_HOME}/user
make 
if [ $? != 0 ]; then
    print_error "Error! Building the Bootstrap ... "
    exit 1
fi
fi

print_step "Decoding Target Binary ... "
cd ${GENERATED_APP}
./c6x-decoder instructions.bin instructions.asm
#./c6x-decoder matmult.coff matmult.asm
if [ $? != 0 ]; then
    print_error "Error! Decoding Target Binary ... "
    exit 1
fi

print_step "Generating Simulator ... "
make clean 
make -s
if [ $? != 0 ]; then
    print_error "Error! Generating Simulator ... "
    exit 1
fi

if [ "$1" = "kvm" ]; then
print_step "Compiling the LIBKVM"
cd ${LIBKVM_HOME}
./configure --arch=i386 --prefix=${LIBKVM_PREFIX}
cd ${LIBKVM_HOME}/libkvm 
make && make install-libkvm
if [ $? != 0 ]; then
    print_error "Error: Compiling the LIBKVM"
    exit 1 
fi

print_step "Compiling the LIBSOCKVM"
cd ${LIBSOCKVM_HOME}
make && make install 
if [ $? != 0 ]; then
    print_error "Error! Compiling the LIBSOCKVM"
    exit 1 
fi
fi

print_step "Building the Simulation Platform ... "
cd ${PFORM_DIR}
make -sj3
if [ $? != 0 ]; then
    print_error "Error! Building the Simulation Platform ... "
    exit 1
fi

print_step "Updating Links ... "
ln -sf ${NASIK_HOME}/hw/kvm-85/user/test/x86/kvm_c6x_bootstrap ${PFORM_DIR}/kvm_c6x_bootstrap
ln -sf ${GENERATED_APP}/instructions.bin.text ${PFORM_DIR}/target_binary_text

print_step "Running the Simulation ... "
rm -f tty100
export PATH=~/workspace/Rabbits-sls/rabbits/tools:$PATH
./arch.x kvm_c6x_bootstrap APPLICATION.X
cat tty100 | less

