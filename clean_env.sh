#!/bin/bash

if [ -z $NASIK_HOME ] ; then
	echo "$NASIK_HOME variable is Undefined; Please Run the Configure Script First !!!"
else
	echo "Cleaning Native Library (libta) ... "
  cd $NASIK_HOME/hw/native
  make clean -s
 
  echo "Cleaning Software Application ... "
  cd $APP_DIR/sw
  apes-compose -c

  echo "Cleaning Hardware Model ... "
  cd $APP_DIR/hw
  make clean -s

	cd $NASIK_HOME
  echo "Environment Cleanup Done !!!"
fi

