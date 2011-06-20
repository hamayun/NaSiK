echo "[Setting-up Hardware Platform Environment (SystemC, LIBKVM)]"
export NASIK_HOME=$PWD
export SYSTEMC=/opt/libs/systemc-2.2.0
export LIBKVM_HOME=${NASIK_HOME}/hw/kvm-85
export LIBSOCKVM_HOME=${NASIK_HOME}/hw/soc_kvm
export LIBKVM_PREFIX=/opt/libs/libkvm

echo "[Setting-up Software Platform Environment (APES+Toolchains)]"
export PRIMARY_TOOLCHAIN=/opt/toolchains/llvm-2.8
export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-i386
#export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-i386-annotated
export APES_ROOT=/opt/Apes
export APES_EXTRA_COMPS=$NASIK_HOME/sw/apes-components
source $APES_ROOT/install.sh
export APES_PATH=$APES_PATH:$APES_EXTRA_COMPS

#export APPLICATION=kvmParallelMjpeg
#export APPLICATION=qsort
export APPLICATION=pi
export PLATFORM=tuzki
echo "[Setting-up Software Application Environment ($APPLICATION)]"
export APP_DIR=$(find $NASIK_HOME/examples/applications -name "$APPLICATION")
cd $APP_DIR
source install.sh 

export PLATFORM_DIR=$(find $NASIK_HOME/examples/platforms -name "$PLATFORM")

# for tty_terms
export PATH=$NASIK_HOME:$PATH

# add toolchain paths
export PATH=${PRIMARY_TOOLCHAIN}/bin:${SECONDARY_TOOLCHAIN}/bin:$PATH
export LD_LIBRARY_PATH=${LIBKVM_PREFIX}/lib:$LD_LIBRARY_PATH

cd $NASIK_HOME

echo "======================= GENERAL ENVIRONMENT SETTINGS ========================="
echo "SYSTEMC                 = $SYSTEMC"
echo "LIBKVM_HOME             = $LIBKVM_HOME"
echo "LIBKVM_PREFIX           = $LIBKVM_PREFIX"
echo "APPLICATION             = $APP_DIR"
echo "PLATFORM                = $PLATFORM_DIR"
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

