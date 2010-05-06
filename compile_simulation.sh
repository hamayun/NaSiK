#!/bin/bash

if [ -z $PROJ_TOPDIR ] ; then
	echo "$PROJ_TOPDIR variable is Undefined; Please Run the Configure Script First !!!"
else
	echo "Compiling and Installing TA Library ... "
	cd $PROJ_TOPDIR/hw/libta 
	make all install -s 

	echo "Compiling Software Application ... "
	cd $APP_DIR/sw 
	make -s 

	echo "Compiling Hardware Model ... "
	cd $APP_DIR/hw
	make -s 

	echo "Compilation Finished Successfully !!!"
fi

