#!/bin/bash

if [ -z $PROJ_TOPDIR ] ; then
	echo "Environment Variables are Undefined; Please Run the Configure Script First !!!"
else
	echo "Compiling and Installing TA Library ... "
	cd $PROJ_TOPDIR/hw/native 
	make all install -s 
	if [ $? != 0 ]; then
		echo "Compiling and/or Installation of TA Library Failed"
		echo "Exiting ... !!!"
		cd $PROJ_TOPDIR
	else 
		echo "Compiling Software Application ... "
		cd $APP_DIR/sw 
		apes-compose  
		if [ $? != 0 ]; then
			echo "Compilation Failed for Application"
			echo "Exiting ... !!!"
			cd $PROJ_TOPDIR
		else
			echo "Compiling Hardware Model ... "
			cd $APP_DIR/hw
			make -s 
			if [ $? != 0 ]; then
				echo "Compilation Failed for Hardware Model"
				echo "Exiting ... !!!"
				cd $PROJ_TOPDIR
			else 
				echo "Simulation Compiled Successfully !!!" 
				echo "Use run.sh for Simulation. " 
			fi
		fi 	
	fi
fi

