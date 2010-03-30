#!/bin/bash

if [ -z $APES_HOME ] ; then
	echo "[ERROR  ] The APES environment has not been installed"
	echo "[ERROR  ] Please execute the install.sh script in the APES directory"
else

	#
	# Cleaning up things a bit...
	#

	unset MAKEFILE_RULES

	unset TARGET_CAL
	unset TARGET_SYSTEM_KSP_OS
	unset TARGET_SYSTEM_KSP_TASK
	unset TARGET_SYSTEM_ASP_C
	unset TARGET_SYSTEM_ASP_M
	unset TARGET_SYSTEM_ASP_COM
	unset TARGET_SYSTEM_SSP_CAL
	unset TARGET_SYSTEM_SSP_PAL
	unset TARGET_SYSTEM_LIBS

	unset DNA_COMPONENTS
	unset DNA_CFLAGS
	unset DNA_MODULES
	unset DNA_CORE_SERVICES
	unset DNA_DRIVERS
	unset DNA_FILESYSTEMS

	unset TARGET_LLVMGCC
	unset TARGET_LLC
	unset TARGET_AS
	unset TARGET_CFLAGS
	unset TARGET_LD
	unset TARGET_LDFLAGS
	unset TARGET_AR
	unset TARGET_ARFLAGS
	unset TARGET_RANLIB
	unset ANNOTATION

	#
	# Including the configuration file
	#

	if [ -e "configurations/$1" ] ; then
		source configurations/$1
		
		export TARGET_SYSTEM_LIBS="${TARGET_SYSTEM_SSP_CAL} ${TARGET_SYSTEM_KSP_OS} ${TARGET_SYSTEM_ASP_COM} ${TARGET_SYSTEM_KSP_TASK} ${TARGET_SYSTEM_ASP_C} ${TARGET_SYSTEM_ASP_M}"  

		#
		# Print out the configuration
		#

		echo "[APES settings]"
		echo "	| Operating system       : ${TARGET_SYSTEM_KSP_OS}"
		echo "	| Task library           : ${TARGET_SYSTEM_KSP_TASK}"
		echo "	| C library              : ${TARGET_SYSTEM_ASP_C}"
		echo "	| Math library           : ${TARGET_SYSTEM_ASP_M}"
		echo "	| Communication library  : ${TARGET_SYSTEM_ASP_COM}"
		echo "	| CAL                    : ${TARGET_SYSTEM_SSP_CAL}"
		echo "	| PAL                    : ${TARGET_SYSTEM_SSP_PAL}"
		echo
		echo "[DNA settings]"
		echo "	| Components    : ${DNA_COMPONENTS}"
		echo "	| CFLAGS        : ${DNA_CFLAGS}"
		echo "	| Modules       : ${DNA_MODULES}"
		echo "	| Core services : ${DNA_CORE_SERVICES}"
		echo "	| Drivers       : ${DNA_DRIVERS}"
		echo "	| Filesystems   : ${DNA_FILESYSTEMS}"
		echo
		echo "[Tool Chain settings]"
		echo "	| LLVMGCC   : ${TARGET_LLVMGCC}"
		echo "	| LLC       : ${TARGET_LLC}"
		echo "	| AS        : ${TARGET_AS}"
		echo "	| CFLAGS    : ${TARGET_CFLAGS}"
		echo "	| LD        : ${TARGET_LD}"
		echo "	| LDFLAGS   : ${TARGET_LDFLAGS}"
		echo "	| AR        : ${TARGET_AR}"
		echo "	| ARFLAGS   : ${TARGET_ARFLAGS}"
		echo "	| RANLIB    : ${TARGET_RANLIB}"
		echo "	| ANNOTATION: ${ANNOTATION}" 
	else
		echo "[ERROR  ] The configuration file does not exist"
		echo "[ERROR  ] Please create one and restart the installation"
	fi
fi
