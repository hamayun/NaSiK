#!/bin/bash

if [ -z $PROJ_TOPDIR ] ; then
	echo "$PROJ_TOPDIR variable is Undefined; Please Run the Configure Script First !!!"
else
	echo "Cleaning TA Library ... "

    cd $PROJ_TOPDIR/hw/libta
    make clean -s
 
	echo "Cleaning Software Application ... "
	cd $APP_DIR/sw
    make clean -s

    echo "Cleaning Hardware Model ... "
    cd $APP_DIR/hw
    make clean -s

	cd $PROJ_TOPDIR

	rm -f `find . -name "*.bb"`
	rm -f `find . -name "*.mbb"`
	rm -f `find . -name "*.dot"`
	rm -f `find . -name "cachegrind.out.*"`

    echo "Environment Cleanup Done !!!"
fi

