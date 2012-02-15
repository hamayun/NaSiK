echo "[Setting-up Hardware Platform Environment (SystemC, LIBKVM)]"
export NASIK_HOME=$PWD
export SYSTEMC=/opt/libs/systemc-2.2.0
export LIBKVM_HOME=${NASIK_HOME}/hw/kvm-85
export LIBSOCKVM_HOME=${NASIK_HOME}/hw/soc_kvm
export LIBKVM_PREFIX=/opt/libs/libkvm
export LIBKVMTOOL_PREFIX=/home/hamayun/workspace/linux-kvm/tools/kvm

echo "[Setting-up Software Platform Environment (Toolchains)]"
export PRIMARY_TOOLCHAIN=/opt/toolchains/llvm-2.8
export BAREMETAL_TOOLCHAIN=/opt/toolchains/i386-pc-baremetal

export TARGET_BIN_WRITER=/home/hamayun/examples/binary-file-writer

export C6XDEC_SOURCE=${NASIK_HOME}/sw/llvm/projects/c6x-decoder
export C6XDEC_BUILD=${HOME}/workspace/Build/llvm-2.8/projects/c6x-decoder

export C6XISA_BEHAVIOR=${C6XDEC_SOURCE}/lib/ISABehavior
export GENERATED_APP=${C6XDEC_SOURCE}/generated-app
export APP_DIR=${GENERATED_APP}

export PLATFORM=tuzki
export PFORM_DIR=${NASIK_HOME}/examples/platforms/$PLATFORM

# for tty_terms
export PATH=$NASIK_HOME:$PATH
export PATH=${BAREMETAL_TOOLCHAIN}/bin:$PATH
export PATH=${PRIMARY_TOOLCHAIN}/bin:$PATH
export LD_LIBRARY_PATH=${LIBKVMTOOL_PREFIX}:${LIBKVM_PREFIX}/lib

cd $NASIK_HOME

echo "======================= GENERAL ENVIRONMENT SETTINGS ========================="
echo "SYSTEMC                 = $SYSTEMC"
echo "PRIMARY_TOOLCHAIN       = $PRIMARY_TOOLCHAIN"
echo "BAREMETAL_TOOLCHAIN     = $BAREMETAL_TOOLCHAIN"
echo "LIBKVM_HOME             = $LIBKVM_HOME"
echo "LIBKVM_PREFIX           = $LIBKVM_PREFIX"
echo "TARGET_BIN_WRITER       = $TARGET_BIN_WRITER"
echo "C6XDEC_SOURCE           = $C6XDEC_SOURCE"
echo "C6XDEC_BUILD            = $C6XDEC_BUILD"
echo "C6XISA_BEHAVIOR         = $C6XISA_BEHAVIOR"
echo "GENERATED_APP           = $GENERATED_APP"
echo "PLATFORM                = $PFORM_DIR"
echo "=============================================================================="

echo "Hint: You can use the 'simulatec6x.sh' script in '$PFORM_DIR'"
echo "For Executing C6x Decoder and Generating the Simulation Platform."