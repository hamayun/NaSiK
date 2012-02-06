#!/bin/bash

export CUSTOM_INCLUDES="-I${SECONDARY_TOOLCHAIN}/arm-sls-dnaos/include/ -I${SECONDARY_TOOLCHAIN}/lib/gcc/arm-sls-dnaos/4.4.2/include/"

# Try and Do the Split Compilation using the Primary Compiler. 
#export APES_SPLIT_CC=""
export APES_CC1="llvm-gcc"
export APES_CC2="llc"
export APES_CC1_FLAGS="-Wall -Wno-format -std=c99 -U__linux__ --emit-llvm -nostdinc -DMEASURE_ACCURACY -DPLATFORM_QEMU $CUSTOM_INCLUDES"
export APES_CC2_FLAGS="-march=arm"
#export APES_CC2_FLAGS="-soft-float -march=arm"

# On Failure Use the default APES_COMPILER
export APES_COMPILER="arm-sls-dnaos-gcc"
export APES_CC_FLAGS="-Wall -Wno-format -std=c99 -mlittle-endian -O3 -mfpu=fpa -march=armv6 $CUSTOM_INCLUDES"

export APES_ASSEMBLER="arm-sls-dnaos-as"
#export APES_ASSEMBLER_FLAGS="-mfloat-abi=soft"
export APES_LINKER="arm-sls-dnaos-gcc"
export APES_LINKER_FLAGS="-mfpu=fpa -mlittle-endian -march=armv6 -Wl,-T,elf.lds"
export APES_LINKER_TRAIL_FLAGS="-L${SECONDARY_TOOLCHAIN}/arm-sls-dnaos/lib -lc -lgcc -lm"

APES_CC_OPTIMIZATIONS="-g"
#APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=INFO_LEVEL"
#APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=VERBOSE_LEVEL"
export APES_CC_OPTIMIZATIONS

# export DNACORE_CC_FLAGS="-DDNA_ENABLE_LOG=VERBOSE_LEVEL"
unset DNACORE_CC_FLAGS

export MJPEG_CC_FLAGS="-DNB_DECODER=1 -DDISPATCH_TIME -DINFO -DSHOW_TIME"

