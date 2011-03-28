echo "[Setting-up Hardware Platform Environment (Native {libta})]"
PROJ_TOPDIR=$PWD
HW_PLATFORM=$PROJ_TOPDIR/hw
export SYSTEMC=/opt/libs/systemc-2.2.0
export NATIVE_HOME=/opt/libs/native
export LD_LIBRARY_PATH=$NATIVE_HOME/lib:/usr/local/lib:$LD_LIBRARY_PATH

echo "[Setting-up Software Platform Environment (APES)]"
export APES_ROOT=/opt/Apes
APES_EXTRA_COMPS=$PROJ_TOPDIR/sw/apes-components
source $APES_ROOT/install.sh
export APES_PATH=$APES_PATH:$APES_EXTRA_COMPS

APP_NAME=ParallelMjpeg
echo "[Setting-up Software Application Environment ($APP_NAME)]"
APP_DIR=$PROJ_TOPDIR/examples/$APP_NAME
cd $APP_DIR/sw
source install.sh 

# for tty_term
export PATH=$PROJ_TOPDIR:$PATH

cd $PROJ_TOPDIR

echo "======================= GENERAL ENVIRONMENT SETTINGS ========================="
echo "SYSTEMC               = $SYSTEMC"
echo "NATIVE_HOME           = $NATIVE_HOME"
echo "APPLICATION           = $APP_DIR"

echo "======================= APES TOOLCHAIN SETTINGS =============================="
echo "APES_ROOT             = $APES_ROOT"
echo "APES_COMPILER         = $APES_COMPILER"
echo "APES_CC_FLAGS         = $APES_CC_FLAGS"
echo "APES_CC_OPTIMIZATIONS = $APES_CC_OPTIMIZATIONS"
echo "APES_LINKER           = $APES_LINKER"
echo "APES_LINKER_FLAGS     = $APES_LINKER_FLAGS"
echo "APES EXTRA COMPONENTS = $APES_EXTRA_COMPS"
echo "APES_PATH             = $APES_PATH"
echo "=============================================================================="
  
#export PRIMARY_TOOLCHAIN=/opt/toolchains/llvm-2.8
#export SECONDARY_TOOLCHAIN=/opt/toolchains/gnu-dnaos-newlibc
#export CONFIGURATION=native
# Answer 'yes' or 'no'
#export USE_ANNOTATIONS=yes

#export PATH=$PRIMARY_TOOLCHAIN/bin:$SECONDARY_TOOLCHAIN/bin:$PATH
#echo "PRIMARY_TOOLCHAIN = $PRIMARY_TOOLCHAIN"
#echo "SECONDARY_TOOLCHAIN = $SECONDARY_TOOLCHAIN"
#echo "CONFIGURATION = $CONFIGURATION"



