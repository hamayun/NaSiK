#!/bin/bash

export CRT0="${SECONDARY_TOOLCHAIN}/i386-sls-dnaos/lib/crt0.o"
export DNASTART="${LIBKVM_HOME}/user/test/x86/dnastart.o"
CUSTOM_INCLUDES="-I${SECONDARY_TOOLCHAIN}/i386-sls-dnaos/include/ -I${SECONDARY_TOOLCHAIN}/lib/gcc/i386-sls-dnaos/4.4.2/include/"

# Try and Do the Split Compilation using the Primary Compiler. 
#export APES_SPLIT_CC=""
export APES_CC1="llvm-gcc"
export APES_CC2="llc"
export APES_CC1_FLAGS="-Wall -Werror -Wno-format -std=c99 -U__linux__ --emit-llvm -nostdinc $CUSTOM_INCLUDES"
export APES_CC2_FLAGS="-annotate=arm -soft-float"
# "-march=arm --print-dual-cfg --print-annotated-cfg

# On Failure Use the default APES_COMPILER
export APES_COMPILER="i386-sls-dnaos-gcc"
export APES_CC_FLAGS="-D__i386__ -D__KERNEL__ -Wall -Werror -Wno-format -std=c99 -O3 -nostdinc $CUSTOM_INCLUDES"
export APES_ASSEMBLER="i386-sls-dnaos-as"

export APES_LINKER="i386-sls-dnaos-gcc"
export APES_LINKER_FLAGS="${DNASTART} ${CRT0} -D__i386__ -D__KERNEL__ -Wl,-T,elf.lds"
export APES_LINKER_TRAIL_FLAGS="-L${SECONDARY_TOOLCHAIN}/i386-sls-dnaos/lib -lc -lgcc -lm"

APES_CC_OPTIMIZATIONS="-g"
APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=INFO_LEVEL"
#APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=VERBOSE_LEVEL"
export APES_CC_OPTIMIZATIONS

