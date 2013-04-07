#!/bin/bash

export PATH=${NASIK_HOME}/tools:$PATH
export CCS_WORKSPACE_PATH=/home/hamayun/workspace_ccs
export CCS_EXAMPLE_RPATH=.

HOME=$PWD
TIMESTAMP=$(date +%Y-%m-%d-%Hh%M)
RESULTSDIR=$NASIK_HOME/tests/batchtest-c6x-polybench_${TIMESTAMP}
RESULTFILE=hosttime_kvm.txt
TTYLOGFILE=tty_debug_-cpu-0
TTYCONFILE=tty_console_-cpu-0
ALL_TTYLOGS_FILE=${RESULTSDIR}/tty_debug_all.txt
ALL_TTYCONS_FILE=${RESULTSDIR}/tty_console_all.txt
ALL_RESULTS_PATH=${RESULTSDIR}/results

# Specify the relative path to the target binary here; for example to test "./polybench-c-3.2/datamining/correlation/Debug/correlation.out" 
# provide the "./polybench-c-3.2/datamining/correlation/Debug/correlation" without the .out extension.

#APPS_LIST="./IDCT/Debug/IDCT"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/stencils/adi/Debug/adi"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/datamining/correlation/Debug/correlation"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/datamining/covariance/Debug/covariance"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/linear-algebra/kernels/doitgen/Debug/doitgen"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/stencils/fdtd-2d/Debug/fdtd-2d"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/stencils/jacobi-2d-imper/Debug/jacobi-2d-imper"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/linear-algebra/solvers/lu/Debug/lu"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/linear-algebra/kernels/mvt/Debug/mvt"
#APPS_LIST="${APPS_LIST} ./polybench-c-3.2/medley/reg_detect/Debug/reg_detect"

APPS_LIST="./polybench-c-3.2/stencils/adi/Debug/adi"
APPS_LIST="${APPS_LIST} ./polybench-c-3.2/stencils/fdtd-2d/Debug/fdtd-2d"

TESTS_FILE="tests_c6x.list"
SCRIPT="simulatec6x.sh"

echo "Batch Test ... Started: $(date)"
mkdir -p ${RESULTSDIR}

source ${CCS_WORKSPACE_PATH}/install.sh

for TARGET_APP in ${APPS_LIST}
do
(
    TARGET_APP_BASE=$(basename ${TARGET_APP})
    TARGET_APP_DIR=$(dirname ${TARGET_APP})

    APP_RESULTS_PATH=${ALL_RESULTS_PATH}_${TARGET_APP_BASE}.txt
    export CCS_EXAMPLE_NAME=${TARGET_APP_BASE}

    echo "Building ... " ${TARGET_APP}
    cd ${CCS_WORKSPACE_PATH}/${TARGET_APP_DIR}/
    gmake clean
    rm -f ${CCS_WORKSPACE_PATH}/${TARGET_APP}.*
    gmake all

    TGT_APP_BIN_FILE="${CCS_WORKSPACE_PATH}/${TARGET_APP}.out"

    cd $PFORM_DIR
    Test=1
    while read TestType; 
    do
        echo "Launching Test for :" $TGT_APP_BIN_FILE
        export CCS_EXAMPLE_PATH=$TGT_APP_BIN_FILE

        ./$SCRIPT $TestType

        echo "[Test # $Test] $SCRIPT $TestType [Application: $TGT_APP_BIN_FILE]" >> ${APP_RESULTS_PATH}
        cat ${RESULTFILE} >> ${APP_RESULTS_PATH}
        echo "------------------------------------------------------------------------------------------------------------" >> ${APP_RESULTS_PATH}
        cat ${TTYLOGFILE} >> ${ALL_TTYLOGS_FILE}
        cat ${TTYCONFILE} >> ${ALL_TTYCONS_FILE}
        rm -f ${RESULTFILE} ${TTYLOGFILE} ${TTYCONFILE}
        echo "++++++++++++++++++++++++++++++++++++++++++ END OF TEST # $Test +++++++++++++++++++++++++++++++++++++++++++++++++" >> ${APP_RESULTS_PATH}
        (( Test = Test + 1 ))
    done < $HOME/$TESTS_FILE
    echo "########################################## END OF APPLICATION TEST #########################################" >> ${APP_RESULTS_PATH}
)
done

echo "Batch Test ... Completed: $(date)"
echo "Done !"

