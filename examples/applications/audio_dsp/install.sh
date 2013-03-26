#!/bin/bash

export TOOLCHAINS="/opt/toolchains/apes-i386/"
export CRT0="/home/hamayun/workspace/Rabbits-hao/rabbits/kvm-85/user/test/x86/dnastart.o"

export APES_COMPILER="i386-sls-dnaos-gcc"
export APES_LINKER="i386-sls-dnaos-gcc"

export APES_CC_FLAGS="-D__i386__ -D__KERNEL__ -Wall -Wno-format -std=c99"
export APES_LINKER_FLAGS="-D__i386__ -Wl,-T,elf.lds"
export APES_LINKER_TRAIL_FLAGS="-L${TOOLCHAIN}/i386-sls-dnaos/lib -lc -lgcc -lm"

APES_CC_OPTIMIZATIONS="-g"
export APES_CC_OPTIMIZATIONS
