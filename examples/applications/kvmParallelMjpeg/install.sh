#!/bin/bash

export TOOLCHAINS="/home/shenhao/tmp/test_native/opt"
export CRT0="/home/shenhao/workspace/vm/kvm/userspace/kvm-compare/kvm-85/user/test/x86/dnastart.o"

export APES_COMPILER="i386-sls-dnaos-gcc"
export APES_LINKER="i386-sls-dnaos-gcc"

export APES_CC_FLAGS="-D__i386__ -D__KERNEL__ -Wall -Werror -Wno-format -std=c99 -I/home/shenhao/workspace/apes.git/apps/kvmValidationFAT/Sources"
export APES_LINKER_FLAGS="${CRT0} -D__i386__ -D__KERNEL__ -Wl,-T,elf.lds -L${TOOLCHAINS}/i386-sls-dnaos/lib"

APES_CC_OPTIMIZATIONS="-g"
export APES_CC_OPTIMIZATIONS

export DNAVIRTUALFILESYSTEM_CC_FLAGS="-DDNA_ENABLE_LOG=VERBOSE_LEVEL"
export MJPEG_CC_FLAGS="-DNB_DECODER=1 -DDISPATCH_TIME -DINFO -DSHOW_TIME"
