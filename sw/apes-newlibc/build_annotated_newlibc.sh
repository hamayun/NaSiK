#!/bin/bash
#
# NewLib compilation script                     
# Copyright (C) 2011, TIMA Laboratory                         
#                                                                      
# This program is free software: you can redistribute it and/or modify 
# it under the terms of the GNU General Public License as published by 
# the Free Software Foundation, either version 3 of the License, or    
# (at your option) any later version.                                  
#                                                                      
# This program is distributed in the hope that it will be useful,      
# but WITHOUT ANY WARRANTY; without even the implied warranty of       
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        
# GNU General Public License for more details.                         
#                                                                      
# You should have received a copy of the GNU General Public License    
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

print_greetings()
{
    echo "See logs directory in case of Build Failure."
}

print_step()
{
    echo "==================================="
    echo -en "\033[01;32m"
    echo " "$1
    echo -en "\033[00m"
    echo "==================================="
}

print_substep()
{
    echo "*** " $1 "..."
}

print_error()
{
    echo -en "\033[01;31m"
    echo "!-----------------------------------------------------------!"
    echo "! "$1
    echo "!-----------------------------------------------------------!"
    echo -en " \033[00m"
}

libc_env_error(){
    print_error "Error: Please Set the Newlib Build Environment First !!!"
    exit
}

build_error(){
    print_error "Error: During build process ... "
    exit
}

check_newlib_env()
{
  if [ -z $NEWLIB_ENV_SET ] ; then
    libc_env_error
  fi
}

build_annotator()
{
  print_step "Building the Annotator Tool"
  cd ${NEWLIB_ANNOTATOR}

  print_substep "Make" 
  make all 			>& ${NEWLIB_LOGS_DIR}/annotator_build.log    || return

  print_substep "Install"
  make install 		>& ${NEWLIB_LOGS_DIR}/annotator_install.log	 || return

  touch ${NEWLIB_STAMPS_DIR}/annotator_built
}

configure_newlib()
{
  print_step "Configuring the Newlib"
  
  # Make sure that we use the apes-compliler toolchain for configure	
  export PATH=${APES_NATIVE_TOOLCHAIN}/bin:$PATH

  rm -rf ${NEWLIB_BUILD_DIR}
  mkdir -p ${NEWLIB_BUILD_DIR}
  cd ${NEWLIB_BUILD_DIR}

  print_substep "Configuring" 

  ${NEWLIB_SOURCE_DIR}/configure 											\
  --prefix=${APES_NATIVE_TOOLCHAIN}				 							\
  --target=i386-sls-dnaos													\
  --disable-nls 															\
  --enable-multilib 														\
  --enable-interwork 														\
  --enable-newlib-io-long-long            									\
  --enable-newlib-io-long-double          									\
  --enable-newlib-io-c99-formats          									\
  --disable-newlib-may-supply-syscalls 										\
  --enable-newlib-multithread 												\
  --disable-newlib-supplied-syscalls 										\
  >& ${NEWLIB_LOGS_DIR}/newlib_configure.log || return
  
  touch ${NEWLIB_STAMPS_DIR}/newlib_configured
}

build_newlib()
{
  print_step "Building the Newlib"

  # In case this function is invoked independently. 
  export PATH=${APES_NATIVE_TOOLCHAIN}/bin:$PATH

  # Now Change the paths; for using annotator tool and LLVM-GCC compiler; for Annotations
  export PATH=${LLVM_NATIVE_TOOLCHAIN}/bin:$PATH 
  export PATH=${ANNOTATOR_INSTALL_PATH}/bin:$PATH 

  # Envrionment Variables used by llvm-cross-gcc tool. 
  export GNU_TOOLCHAINS=${APES_NATIVE_TOOLCHAIN}
  export ANNOTATED_ARCH=arm

  cd ${NEWLIB_BUILD_DIR}

  print_substep "Clean" 
  make clean 	>& ${NEWLIB_LOGS_DIR}/newlib_clean.log  	 || return

  print_substep "Make" 
  make all 		>& ${NEWLIB_LOGS_DIR}/newlib_build.log		 || return

  print_substep "Install" 
  make install  >& ${NEWLIB_LOGS_DIR}/newlib_install.log	 || return
 
  touch ${NEWLIB_STAMPS_DIR}/newlib_built
}

#
# Main
#

# Check if Newlib Environment is Set? 
check_newlib_env

# Now print the Environment Settings
print_greetings

mkdir -p ${NEWLIB_STAMPS_DIR}
mkdir -p ${NEWLIB_LOGS_DIR}

# Build and Install the Annotator Tool. 
[ -e ${NEWLIB_STAMPS_DIR}/annotator_built ] || \
    build_annotator || build_error

# Configure the Newlib using the Native Toolchain.
[ -e ${NEWLIB_STAMPS_DIR}/newlib_configured ] || \
    configure_newlib || build_error

# Now Build and Install Newlib using the LLVM Toolchain.
[ -e ${NEWLIB_STAMPS_DIR}/newlib_built ] || \
    build_newlib || build_error

print_step "Done."


