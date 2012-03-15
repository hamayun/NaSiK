#!/bin/bash
#------------------------------------------------------------------------------#
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
#------------------------------------------------------------------------------#
usage()
{
cat << EOF
usage: $0 options

This script Compiles, Translates and Simulates a Given Target Binary.

OPTIONS:
   -b      Compile Bootstraps
   -d      Run Decoder Only
   -f      Enable Function Level Optimization
   -g      Generated Code Level, 'EP' or 'BB'   [Default 'BB']
   -h      Show this help message
   -i      Enable Inline Code Generation
   -j      Enable Frequently Used Function Inline Code Generation
   -k      Compile KVM Libraries
   -m      Enable Module Level Optimization
   -p      Enable Special Optimizations (If Any)
   -s      Run Simulation
   -t      Target Binary Type, 'RAW' or 'COFF'  [Default 'COFF']
   -v      Verbose Mode
EOF
}

TARGET_BIN="COFF"
COMPILE_KVM=
DECODE_ONLY=
RUN_SIMULATION=
COMPILE_BOOTSTRAPS=
GEN_CODE_LEVEL="BB"
VERBOSE=
OUTPUT_TTY="/dev/null"
CODEGEN_OPT=""

while getopts “bdfg:hijkmpst:v” OPTION
do
     case $OPTION in
         b)
             COMPILE_BOOTSTRAPS=1
             ;;
         d)
             DECODE_ONLY=1
             ;;
         f)
             CODEGEN_OPT+="-fopt "
             ;;
         g)
             GEN_CODE_LEVEL=$OPTARG
             ;;
         h)
             usage
             exit 1
             ;;
         i)
             CODEGEN_OPT+="-inline "
             ;;
         j)
             CODEGEN_OPT+="-finline "
             ;;
         k)
             COMPILE_KVM=1
             ;;
         m)
             CODEGEN_OPT+="-mopt "
             ;;
         p)
             CODEGEN_OPT+="-sopt "
             ;;
         s)
             RUN_SIMULATION=1
             ;;
         t)
             TARGET_BIN=$OPTARG
             ;;
         v)
             VERBOSE=1
             OUTPUT_TTY="/dev/stdout"
             ;;
         "?")
             usage
             exit
             ;;
     esac
done
#------------------------------------------------------------------------------#
if [ $TARGET_BIN = "RAW" ]; then
    print_step "Compiling Target RAW Binary Writer ..."
    cd ${TARGET_BIN_WRITER}
    make clean                                                    >& $OUTPUT_TTY
    make                                                          >& $OUTPUT_TTY
    if [ $? != 0 ]; then
        print_error "Error! Compiling Target RAW Binary Writer ..."
        exit 1
    fi

    print_step "Writting RAW Binary ..."
    ./BFW.X                                                       >& $OUTPUT_TTY
    if [ $? != 0 ]; then
        print_error "Error! Writting RAW Binary ..."
        exit 1
    fi

    ln -sf ${TARGET_BIN_WRITER}/instructions.bin ${GENERATED_APP}/instructions.bin
elif [ $TARGET_BIN = "COFF" ]; then
    print_step "Compiling Target COFF Binary ... ${CCS_EXAMPLE_OUTFILE}"
    cd ${CCS_WORKSPACE_PATH}/${CCS_EXAMPLE_NAME}/${CCS_EXAMPLE_BUILD}
    gmake -k all                                                  >& $OUTPUT_TTY
    if [ $? != 0 ]; then
        print_error "Error! Compiling COFF Binary ..."
        exit 1
    fi
else
    print_error "Error! Un-supported Binary Format ... " $TARGET_BIN
    usage
    exit 1
fi
#------------------------------------------------------------------------------#
print_step "Building the C6x Decoder ... "
cd ${C6XDEC_BUILD}
make -s                                                           >& $OUTPUT_TTY
if [ $? != 0 ]; then
    print_error "Error! Building the C6x Decoder ..."
    exit 1
fi
#------------------------------------------------------------------------------#
print_step "Generating ISA Behavior ... "
cd ${C6XISA_BEHAVIOR}
make clean                                                        >& $OUTPUT_TTY
make -s                                                           >& $OUTPUT_TTY
if [ $? != 0 ]; then
    print_error "Error! Generating ISA Behavior ... "
    exit 1
fi
#------------------------------------------------------------------------------#
if [ -f "${C6XDEC_BUILD}/Debug+Asserts/bin/c6x-decoder" ]
then
    echo "  Debug Build of c6x-decoder Found ... "                >& $OUTPUT_TTY
    ln -sf ${C6XDEC_BUILD}/Debug+Asserts/bin/c6x-decoder ${GENERATED_APP}/c6x-decoder
else
    echo "  Creating Link for Release Build of c6x-decoder ... "  >& $OUTPUT_TTY
    ln -sf ${C6XDEC_BUILD}/Release+Asserts/bin/c6x-decoder ${GENERATED_APP}/c6x-decoder
fi
#------------------------------------------------------------------------------#
if [ -n "$COMPILE_BOOTSTRAPS" ]; then
    print_step "Building Bootstrap(s) ... "
    cd ${LIBKVM_HOME}/user
    make                                                          >& $OUTPUT_TTY
    if [ $? != 0 ]; then
        print_error "Error! Building the Bootstrap ... "
        exit 1
    fi

    ln -sf ${NASIK_HOME}/hw/kvm-85/user/test/x86/kvm_c6x_bootstrap ${PFORM_DIR}/kvm_c6x_bootstrap
fi
#------------------------------------------------------------------------------#
print_step "Decoding Target Binary ... "
cd ${GENERATED_APP}
rm -f gen_*

DEC_CMD_LINE=""

if [ $TARGET_BIN = "RAW" ]; then
    DEC_CMD_LINE+="instructions.bin -o instructions.asm -ibf=RAW"
else
    DEC_CMD_LINE+="${CCS_EXAMPLE_PATH} -o ${CCS_EXAMPLE_PATH}.asm -ibf=COFF"
fi

DEC_CMD_LINE+=" -$GEN_CODE_LEVEL -isa=C62xISABehavior_v2.bc $CODEGEN_OPT"

./c6x-decoder $DEC_CMD_LINE                                       >& $OUTPUT_TTY
if [ $? != 0 ]; then
    print_error "Error! Decoding Target Binary ... "
    exit 1
fi

if [ -n "$DECODE_ONLY" ]; then
	exit 2
fi
#------------------------------------------------------------------------------#
print_step "Generating Simulator ... "
make clean                                                        >& $OUTPUT_TTY
make -s                                                           >& $OUTPUT_TTY
if [ $? != 0 ]; then
    print_error "Error! Generating Simulator ... "
    exit 1
fi
#------------------------------------------------------------------------------#
if [ -n "$COMPILE_KVM" ]; then
    print_step "Compiling the LIBKVM"
    cd ${LIBKVM_HOME}
    ./configure --arch=i386 --prefix=${LIBKVM_PREFIX}             >& $OUTPUT_TTY
    cd ${LIBKVM_HOME}/libkvm
    make && make install-libkvm                                   >& $OUTPUT_TTY
    if [ $? != 0 ]; then
        print_error "Error: Compiling the LIBKVM"
        exit 1
    fi

    print_step "Compiling the LIBSOCKVM"
    cd ${LIBSOCKVM_HOME}
    make && make install                                          >& $OUTPUT_TTY
    if [ $? != 0 ]; then
        print_error "Error! Compiling the LIBSOCKVM"
        exit 1
    fi
fi
#------------------------------------------------------------------------------#
if [ -n "$RUN_SIMULATION" ]; then
    print_step "Building the Simulation Platform ... "
    cd ${PFORM_DIR}
    make -sj3                                                     >& $OUTPUT_TTY
    if [ $? != 0 ]; then
        print_error "Error! Building the Simulation Platform ... "
        exit 1
    fi

    print_step "Running the Simulation ... "
    rm -f tty_console_00
    rm -f tty_debug_00
    export PATH=~/workspace/Rabbits-sls/rabbits/tools:$PATH

    if [ $TARGET_BIN = "RAW" ]; then
        ./arch.x kvm_c6x_bootstrap APPLICATION.X
    else
        ./arch.x kvm_c6x_bootstrap APPLICATION.X ${CCS_EXAMPLE_PATH}
    fi
fi
#------------------------------------------------------------------------------#
