echo "[Setting-up Hardware Platform Environment (Native {libta})]"
PROJ_TOPDIR=$PWD
HW_PLATFORM=$PROJ_TOPDIR/hw
export SYSTEMC=/opt/libs/systemc-2.2.0
export NATIVE_HOME=/opt/libs/native
export LD_LIBRARY_PATH=$NATIVE_HOME/lib:/usr/local/lib:$LD_LIBRARY_PATH

echo "[Setting-up Software Platform Environment (APES+Toolchains)]"
export PRIMARY_TOOLCHAIN=/opt/toolchains/llvm-2.8
export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-native
export APES_ROOT=/opt/Apes
export APES_EXTRA_COMPS=$PROJ_TOPDIR/sw/apes-components
source $APES_ROOT/install.sh
export APES_PATH=$APES_PATH:$APES_EXTRA_COMPS

#APP_NAME=ParallelMjpeg
#APP_NAME=BlockDeviceTest
#APP_NAME=susan
#APP_NAME=qsort
APP_NAME=dijkstra
echo "[Setting-up Software Application Environment ($APP_NAME)]"
APP_DIR=$(find $PROJ_TOPDIR/examples -name "$APP_NAME")
cd $APP_DIR/sw
source install.sh 

# for tty_term
export PATH=$PROJ_TOPDIR:$PATH

cd $PROJ_TOPDIR

echo "======================= GENERAL ENVIRONMENT SETTINGS ========================="
echo "SYSTEMC                 = $SYSTEMC"
echo "NATIVE_HOME             = $NATIVE_HOME"
echo "APPLICATION             = $APP_DIR"
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

