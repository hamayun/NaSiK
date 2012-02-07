echo "[Setting-up Hardware Platform Environment (SystemC, LIBKVM)]"
export NASIK_HOME=$PWD
export SYSTEMC=/opt/libs/systemc-2.2.0
export LIBKVM_HOME=${NASIK_HOME}/hw/kvm-85
export LIBSOCKVM_HOME=${NASIK_HOME}/hw/soc_kvm
export LIBKVM_PREFIX=/opt/libs/libkvm
export LIBKVMTOOL_PREFIX=/home/hamayun/sandbox/linux-kvm/tools/kvm

echo "[Setting-up Software Platform Environment (APES+Toolchains)]"
export PRIMARY_TOOLCHAIN=/opt/toolchains/llvm-2.8-debug
export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-i386
#export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-i386-multifloat
#export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-i386-multifloat-llvm-newlib
#export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-i386-multifloat-annotated
export APES_ROOT=/opt/Apes
export APES_EXTRA_COMPS=$NASIK_HOME/sw/apes-components
source $APES_ROOT/install.sh
export APES_PATH=$APES_PATH:$APES_EXTRA_COMPS

APPLICATION=kvmParallelMjpeg
#APPLICATION=susan
#APPLICATION=qsort
#APPLICATION=dijkstra
#APPLICATION=patricia
#APPLICATION=blowfish
#APPLICATION=rijndael
#APPLICATION=sha
#APPLICATION=CRC32
#APPLICATION=bitcount
#APPLICATION=cjpeg
#APPLICATION=djpeg
#APPLICATION=stringsearch
#APPLICATION=kvmPi
#APPLICATION=selfProf
#APPLICATION=dynMemTest
#APPLICATION=printFloats
#APPLICATION=blockIOTest
#APPLICATION=printIOTest
#APPLICATION=kvmPhyMemTest
export APPLICATION
export APP_DIR=$(find $NASIK_HOME/examples/applications -name "$APPLICATION")

#export PLATFORM=tuzki
export PLATFORM=kroger
export PFORM_DIR=$NASIK_HOME/examples/platforms/$PLATFORM

echo "[Setting-up Software Application Environment ($APPLICATION)]"
cd $NASIK_HOME/examples/applications
source config-apps-kvm.sh

if [ -e ${APP_DIR}/app_specific_config.sh ]; then
	echo "Application Specific Configuration Found ... Sourcing it."
	cd ${APP_DIR}
	source app_specific_config.sh
fi

#Update Links in Application.
echo "Updating Application Specific Symlinks ..."
cd ${APP_DIR}

if [ ${APPLICATION} != "kvmParallelMjpeg" ]; then
	ln -sf $NASIK_HOME/examples/applications/ldscript_elf.kvm_mibench elf.lds
	ln -sf interface.xmi.kvm interface.xmi
fi

# for tty_terms
export PATH=$NASIK_HOME:$PATH
export PATH=${SECONDARY_TOOLCHAIN}/bin:$PATH
export PATH=${PRIMARY_TOOLCHAIN}/bin:$PATH
export LD_LIBRARY_PATH=${LIBKVMTOOL_PREFIX}:${LIBKVM_PREFIX}/lib

cd $NASIK_HOME

echo "======================= GENERAL ENVIRONMENT SETTINGS ========================="
echo "SYSTEMC                 = $SYSTEMC"
echo "LIBKVM_HOME             = $LIBKVM_HOME"
echo "LIBKVM_PREFIX           = $LIBKVM_PREFIX"
echo "APPLICATION             = $APP_DIR"
echo "PLATFORM                = $PFORM_DIR"
echo "APES_ROOT               = $APES_ROOT"
echo "APES EXTRA COMPONENTS   = $APES_EXTRA_COMPS"
echo "APES_PATH               = $APES_PATH"

if [ ${!APES_SPLIT_CC[@]} ]; then
	echo "======================= SPLIT COMPILATION TOOLCHAIN =========================="
	echo "PRIMARY_TOOLCHAIN       = $PRIMARY_TOOLCHAIN"
	echo "SECONDARY_TOOLCHAIN     = $SECONDARY_TOOLCHAIN"
	echo "APES_CC1                = $APES_CC1"
	echo "APES_CC2                = $APES_CC2"
	echo "APES_CC1_FLAGS          = $APES_CC1_FLAGS"
	echo "APES_CC2_FLAGS          = $APES_CC2_FLAGS"
else 
	echo "======================= APES COMPILATION TOOLCHAIN ==========================="
	echo "APES_COMPILER           = $APES_COMPILER"
	echo "APES_CC_FLAGS           = $APES_CC_FLAGS"
fi

echo "APES_CC_OPTIMIZATIONS   = $APES_CC_OPTIMIZATIONS"
echo "APES_LINKER             = $APES_LINKER"
echo "APES_LINKER_FLAGS       = $APES_LINKER_FLAGS"
echo "APES_LINKER_TRAIL_FLAGS = $APES_LINKER_TRAIL_FLAGS"
echo "=============================================================================="

