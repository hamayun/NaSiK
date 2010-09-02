echo "[Setting up Hardware Platform environment]"
PROJ_TOPDIR=$PWD
HW_PLATFORM=$PROJ_TOPDIR/hw
APP_DIR=$PROJ_TOPDIR/examples/mjpegmthr

export LIBTA_HOME=/opt/libs/libta
export SYSTEMC=/opt/libs/systemc-2.2.0

echo "LIBTA_HOME = $LIBTA_HOME"
echo "SYSTEMC = $SYSTEMC"
echo "APPLICATION = $APP_DIR"
export LD_LIBRARY_PATH=$LIBTA_HOME/lib:$LD_LIBRARY_PATH

echo "[Setting up Software platform environment]"
SW_PLATFORM=$PWD/sw

cd $SW_PLATFORM/apes-elements
source install.sh

export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
export PATH=/opt/toolchains/gnu-dnaos-newlibc/bin:$PATH
# For tty_term
export PATH=/opt:$PATH

# Now move to Application Dir
cd $APP_DIR/sw
source install.sh native

cd $PROJ_TOPDIR

