#!/bin/bash

echo "[Setting-up Environment for Annotated NewLibC Build]"
export APES_LIBC_TOPDIR=$PWD

# This is the toolchain that will try to compile and annotate newlibc. 
export LLVM_NATIVE_TOOLCHAIN=/opt/toolchains/llvm-2.8

# This is the toolchain that will serve as backup if the above toolchain fails; 
# NB: This toolchain will be used a destination as well for installing the annotated newlibc. 
export APES_NATIVE_TOOLCHAIN=/opt/toolchains/apes-native-annotated

# Path to the newlib sources
export NEWLIB_SOURCE_DIR=${APES_LIBC_TOPDIR}/newlib-1.18.0

# Path to the sources of the newlibc anotator tool
export NEWLIB_ANNOTATOR=${APES_LIBC_TOPDIR}/newlib-annotator

# Where to install the annotator tool?
export ANNOTATOR_INSTALL_PATH=${APES_LIBC_TOPDIR}/installed-annotater

# Install Name for the annotator tool; we use the same name as compiler, so make will easier. 
export ANNOTATOR_INSTALL_NAME=i386-sls-dnaos-gcc

export NEWLIB_BUILD_DIR=${APES_LIBC_TOPDIR}/build-newlib
export NEWLIB_STAMPS_DIR=${APES_LIBC_TOPDIR}/stamps
export NEWLIB_LOGS_DIR=${APES_LIBC_TOPDIR}/logs

echo "=============== NEWLIBC BUILD ENVIRONMENT SETTINGS ==================="
echo "LLVM_NATIVE_TOOLCHAIN   = $LLVM_NATIVE_TOOLCHAIN"
echo "APES_NATIVE_TOOLCHAIN   = $APES_NATIVE_TOOLCHAIN"
echo "NEWLIB_SOURCE_DIR       = $NEWLIB_SOURCE_DIR"
echo "NEWLIB_ANNOTATOR        = $NEWLIB_ANNOTATOR"
echo "ANNOTATOR_INSTALL_PATH  = $ANNOTATOR_INSTALL_PATH"
echo "ANNOTATOR_INSTALL_NAME  = $ANNOTATOR_INSTALL_NAME"
echo "======================================================================"

export NEWLIB_ENV_SET="TRUE"

echo -en "\033[01;31m"
echo "WARNING: Contents of the following toolchain will be OVERWRITTEN."
echo "Toolchain Path: "$APES_NATIVE_TOOLCHAIN
echo "Make a Backup (if necessary) before Launching the Build Script !!!"
echo -en "\033[00m"

echo "[Done]"

