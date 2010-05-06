#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=
AS=

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_CONF=Default
CND_DISTDIR=dist

# Include project Makefile
include platform-proj-Makefile.mk

# Object Directory
OBJECTDIR=build/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES=

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	cd ../../examples/mjpegmthr/hw && ${MAKE} -f Makefile -s

# Subprojects
.build-subprojects:
	cd ../libta-proj && ${MAKE}  -f libta-proj-Makefile.mk CONF=Default
	cd ../mjpeg-proj && ${MAKE}  -f mjpeg-proj-Makefile.mk CONF=Default

# Clean Targets
.clean-conf:
	cd ../../examples/mjpegmthr/hw && ${MAKE} -f Makefile clean -s

# Subprojects
.clean-subprojects:
	cd ../libta-proj && ${MAKE}  -f libta-proj-Makefile.mk CONF=Default clean
	cd ../mjpeg-proj && ${MAKE}  -f mjpeg-proj-Makefile.mk CONF=Default clean
