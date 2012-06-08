#!/bin/bash

print_step()
{
    echo "======================================================================"
    echo -en "\033[01;32m"
    echo " "$1
    echo -en "\033[00m"
    echo "======================================================================"
}

print_error()
{
    echo -en "\033[01;31m"
    echo "!--------------------------------------------------------------------!"
    echo "! "$1
    echo "!--------------------------------------------------------------------!"
    echo -en " \033[00m"
    cd $NASIK_HOME
}

if [ -z $NASIK_HOME ] ; then
    print_error "Environment Variables Undefined; Run the Configure Script First !!!"
    exit 1 
fi

print_step "Compiling Software Application ... ${APPLICATION}"
cd ${APP_DIR}
apes-compose
if [ $? != 0 ]; then
    print_error "Compilation Failed for Application"
    exit 3 
fi


