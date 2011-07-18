#!/bin/bash

export APES_COMPILER="arm-sls-dnaos-gcc"
export APES_CC_FLAGS="-Wall -Wno-format -std=c99"

export APES_ASSEMBLER="arm-sls-dnaos-as"

export APES_LINKER="arm-sls-dnaos-gcc"
export APES_LINKER_FLAGS="-mfpu=fpa -mlittle-endian -march=armv6 -Wl,-T,elf.lds"
export APES_LINKER_TRAIL_FLAGS="-L${SECONDARY_TOOLCHAIN}/i386-sls-dnaos/lib -lc -lgcc -lm"

APES_CC_OPTIMIZATIONS="-g -mlittle-endian -O3 -mfpu=fpa -march=armv6"
#APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=INFO_LEVEL"
#APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=VERBOSE_LEVEL"
export APES_CC_OPTIMIZATIONS

# export DNACORE_CC_FLAGS="-DDNA_ENABLE_LOG=VERBOSE_LEVEL"
unset DNACORE_CC_FLAGS

export MJPEG_CC_FLAGS="-DNB_DECODER=1 -DDISPATCH_TIME -DINFO -DSHOW_TIME"

