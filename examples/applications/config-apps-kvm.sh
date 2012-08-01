#!/bin/bash

export CRT0="${SECONDARY_TOOLCHAIN}/i386-sls-dnaos/lib/crt0.o"
export CUSTOM_INCLUDES="-I${SECONDARY_TOOLCHAIN}/i386-sls-dnaos/include/ -I${SECONDARY_TOOLCHAIN}/lib/gcc/i386-sls-dnaos/4.4.2/include/"
export DNASTART="${LIBKVM_HOME}/user/test/x86/dnastart.o"

# Try and Do the Split Compilation using the Primary Compiler. 
#export APES_SPLIT_CC=""
export APES_CC1="llvm-gcc"
export APES_CC2="llc"
export APES_CC1_FLAGS="-Wall -Wno-format -std=c99 -U__linux__ --emit-llvm -nostdinc -DMEASURE_ACCURACY $CUSTOM_INCLUDES"
export APES_CC2_FLAGS="-soft-float -annotate=arm"
#export APES_CC2_FLAGS="-soft-float"
# "-march=arm --print-dual-cfg --print-annotated-cfg"

# On Failure Use the default APES_COMPILER
export APES_COMPILER="i386-sls-dnaos-gcc"
export APES_CC_FLAGS="-Wall -Wno-format -std=c99 -O0 ${CUSTOM_PARAM} $CUSTOM_INCLUDES"
export APES_ASSEMBLER="i386-sls-dnaos-as"

export APES_LINKER="i386-sls-dnaos-gcc"
#export APES_LINKER_FLAGS="${CRT0} ${DNASTART} -Wl,-T,elf.lds"
export APES_LINKER_FLAGS="${CRT0} -Wl,-T,elf.lds"
export APES_LINKER_TRAIL_FLAGS="-L${SECONDARY_TOOLCHAIN}/i386-sls-dnaos/lib -lc -lgcc -lm"

APES_CC_OPTIMIZATIONS="-g"
APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=INFO_LEVEL"
#APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=VERBOSE_LEVEL"
export APES_CC_OPTIMIZATIONS

export MJPEGKVM_CC_FLAGS="-DNB_DECODER=8 -DDISPATCH_TIME -DINFO -DSHOW_TIME"
