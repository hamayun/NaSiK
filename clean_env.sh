#!/bin/bash

if [ -z $PROJ_TOPDIR ] ; then
	echo "$PROJ_TOPDIR variable is Undefined; Please Run the Configure Script First !!!"
else
	echo "Cleaning Native Library (libta) ... "

    cd $PROJ_TOPDIR/hw/native
    make clean -s
 
    echo "Cleaning Software Application ... "
    cd $APP_DIR/sw
    apes-compose -c

    echo "Cleaning Hardware Model ... "
    cd $APP_DIR/hw
    make clean -s

	cd $PROJ_TOPDIR

  echo "Environment Cleanup Done !!!"
fi

