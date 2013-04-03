echo "[Setting-up Hardware Platform Environment (SystemC, LIBKVM)]"
export NASIK_HOME=$PWD
export SYSTEMC=/opt/libs/systemc-2.2.0
export LIBKVM_HOME=${NASIK_HOME}/hw/kvm-85
export LIBSOCKVM_HOME=${NASIK_HOME}/hw/soc_kvm
export LIBKVM_PREFIX=/opt/libs/libkvm
export LIBKVMTOOL_PREFIX=/home/hamayun/workspace/linux-kvm/tools/kvm
export SYSTEMC_INCLUDE_FOR_KVM=${NASIK_HOME}/hw/components/kvm_wrapper_smp/

echo "[Setting-up Software Platform Environment (Toolchains)]"
export PRIMARY_TOOLCHAIN=/opt/toolchains/llvm-2.8
export BAREMETAL_TOOLCHAIN=/opt/toolchains/i386-pc-baremetal

#export TARGET_BIN_WRITER=/home/hamayun/examples/binary-file-writer
export TARGET_BIN_WRITER=/home/hamayun/examples/ti-c6x-examples/BinWriter

export CCS_WORKSPACE_PATH=/home/hamayun/workspace_ccs
#export CCS_EXAMPLE_RPATH=./polybench-c-3.2/stencils
#export CCS_EXAMPLE_RPATH=./polybench-c-3.2/linear-algebra/kernels
#export CCS_EXAMPLE_RPATH=./polybench-c-3.2/linear-algebra/solvers
#export CCS_EXAMPLE_RPATH=./polybench-c-3.2/datamining/
#export CCS_EXAMPLE_RPATH=./polybench-c-3.2/medley/
export CCS_EXAMPLE_RPATH=.
#export CCS_EXAMPLE_NAME=matmult
#export CCS_EXAMPLE_NAME=factorial
#export CCS_EXAMPLE_NAME=fibonacci
#export CCS_EXAMPLE_NAME=file_io
export CCS_EXAMPLE_NAME=IDCT
#export CCS_EXAMPLE_NAME=jacobi-2d-imper
#export CCS_EXAMPLE_NAME=doitgen
#export CCS_EXAMPLE_NAME=lu
#export CCS_EXAMPLE_NAME=2mm		# Prints Some Errors
#export CCS_EXAMPLE_NAME=seidel-2d  # Prints Some Errors
#export CCS_EXAMPLE_NAME=correlation
#export CCS_EXAMPLE_NAME=covariance
#export CCS_EXAMPLE_NAME=reg_detect

export CCS_EXAMPLE_BUILD=Debug
export CCS_EXAMPLE_OUTFILE=${CCS_EXAMPLE_NAME}.out
export CCS_EXAMPLE_PATH=${CCS_WORKSPACE_PATH}/${CCS_EXAMPLE_RPATH}/${CCS_EXAMPLE_NAME}/${CCS_EXAMPLE_BUILD}/${CCS_EXAMPLE_OUTFILE}
source ${CCS_WORKSPACE_PATH}/install.sh

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
echo "CCS_WORKSPACE_PATH      = $CCS_WORKSPACE_PATH"
echo "CCS_EXAMPLE_PATH        = $CCS_EXAMPLE_PATH"
echo "C6XDEC_SOURCE           = $C6XDEC_SOURCE"
echo "C6XDEC_BUILD            = $C6XDEC_BUILD"
echo "C6XISA_BEHAVIOR         = $C6XISA_BEHAVIOR"
echo "GENERATED_APP           = $GENERATED_APP"
echo "PLATFORM                = $PFORM_DIR"
echo "=============================================================================="

echo "Hint: You can use the 'simulatec6x.sh' script in '$PFORM_DIR'"
echo "For Executing C6x Decoder and Generating the Simulation Platform. Type './simulatec6x.sh -h' for more details."
