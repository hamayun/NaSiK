#!/bin/bash

export TOOLCHAINS="/opt/toolchains/apes-native"
export CRT0="${TOOLCHAINS}/i386-sls-dnaos/lib/crt0.o"

export APES_COMPILER="i386-sls-dnaos-gcc"
export APES_LINKER="i386-sls-dnaos-gcc"

export APES_CC_FLAGS="-Wall -Werror -Wno-format -std=c99"
export APES_LINKER_FLAGS="-Wl,-shared ${CRT0} -L${TOOLCHAINS}/i386-sls-dnaos/lib"

APES_CC_OPTIMIZATIONS="-g"
APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=INFO_LEVEL"
#APES_CC_OPTIMIZATIONS+=" -DDNA_ENABLE_LOG=VERBOSE_LEVEL"

export APES_CC_OPTIMIZATIONS

export MJPEG_CC_FLAGS="-DNB_DECODER=1 -DDISPATCH_TIME -DINFO"

export PATH=${TOOLCHAINS}/bin:$PATH

