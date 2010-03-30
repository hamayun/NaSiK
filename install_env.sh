echo "[Setting up Hardware Platform environment]"
LOCAL=$PWD
HW_PLATFORM=$PWD/hw
export LIBTA_HOME=/opt/libs/libta
export SYSTEMC=/opt/libs/systemc-2.2.0

echo "LIBTA_HOME = $LIBTA_HOME"
echo "SYSTEMC = $SYSTEMC"
export LD_LIBRARY_PATH=$LIBTA_HOME/lib:$LD_LIBRARY_PATH

echo "[Setting up Software platform environment]"
SW_PLATFORM=$PWD/sw

cd $SW_PLATFORM/apes-elements
source install.sh
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
cd $LOCAL

export PATH=/opt/toolchains/gnu-dnaos-newlibc/bin:$PATH
# For tty_term
export PATH=/opt:$PATH
