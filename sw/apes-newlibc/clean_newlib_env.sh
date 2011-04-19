#!/bin/bash

print_substep()
{
    echo "*** " $1
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

check_newlib_env()
{
  if [ -z $NEWLIB_ENV_SET ] ; then
    libc_env_error
  fi
}

#
# Main
#

# Check if Newlib Environment is Set? 
check_newlib_env

print_substep "Cleaning Newlib Build Env"

cd ${NEWLIB_ANNOTATOR}
make clean 
cd ${APES_LIBC_TOPDIR}

rm -rf ${ANNOTATOR_INSTALL_PATH}
rm -rf ${NEWLIB_BUILD_DIR}
rm -rf ${NEWLIB_STAMPS_DIR}
rm -rf ${NEWLIB_LOGS_DIR}

print_substep "Done."

