echo "[Setting up Hardware Platform environment]"
PROJ_TOPDIR=$PWD
HW_PLATFORM=$PROJ_TOPDIR/hw
APP_DIR=$PROJ_TOPDIR/examples/mjpegmthr

export LIBTA_HOME=/opt/libs/libta
export SYSTEMC=/opt/libs/systemc-2.2.0
export PRIMARY_TOOLCHAIN=/opt/toolchains/llvm-2.4
export SECONDARY_TOOLCHAIN=/opt/toolchains/gnu-dnaos-newlibc
export CONFIGURATION=native
# Answer 'yes' or 'no'
export USE_ANNOTATIONS=yes

echo "LIBTA_HOME = $LIBTA_HOME"
echo "SYSTEMC = $SYSTEMC"
echo "PRIMARY_TOOLCHAIN = $PRIMARY_TOOLCHAIN"
echo "SECONDARY_TOOLCHAIN = $SECONDARY_TOOLCHAIN"
echo "APPLICATION = $APP_DIR"
echo "CONFIGURATION = $CONFIGURATION"

export LD_LIBRARY_PATH=$LIBTA_HOME/lib:/usr/local/lib:$LD_LIBRARY_PATH
export PATH=$PRIMARY_TOOLCHAIN/bin:$SECONDARY_TOOLCHAIN/bin:$PATH
# for tty_term
export PATH=$PROJ_TOPDIR:$PATH

echo "[Setting up Software platform environment]"
SW_PLATFORM=$PWD/sw
cd $SW_PLATFORM/apes-elements
source install.sh

# Now Install the Application Configurations
cd $APP_DIR/sw
source install.sh $CONFIGURATION

cd $PROJ_TOPDIR

