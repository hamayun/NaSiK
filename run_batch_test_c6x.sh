#!/bin/bash

HOME=$PWD
TIMESTAMP=$(date +%Y-%m-%d-%Hh%M)
RESULTSDIR=$HOME/batchtest-c6x-${TIMESTAMP}
RESULTFILE=hosttime_kvm.txt
TTYLOGFILE=tty_debug_00
TTYCONFILE=tty_console_00
ALL_TTYLOGS_FILE=${RESULTSDIR}/tty_debug_all.txt
ALL_TTYCONS_FILE=${RESULTSDIR}/tty_console_all.txt
ALL_RESULTS_PATH=${RESULTSDIR}/results

#APPS_LIST="fibonacci matmult factorial"
APPS_LIST="factorial IDCT fibonacci"
TESTS_FILE="tests_list_c6x.txt"
SCRIPT="simulatec6x.sh"

echo "Batch Test ... Started: $(date)"
mkdir -p ${RESULTSDIR}

for TARGET_APP in ${APPS_LIST}
do
(
    APP_RESULTS_PATH=${ALL_RESULTS_PATH}_${TARGET_APP}.txt

    export CCS_EXAMPLE_BUILD=Debug
    export CCS_EXAMPLE_NAME=$TARGET_APP
    export CCS_EXAMPLE_OUTFILE=${CCS_EXAMPLE_NAME}.out
    source ${CCS_WORKSPACE_PATH}/install.sh

    echo "Building All Versions of :" $TARGET_APP
    cd ${CCS_WORKSPACE_PATH}/${TARGET_APP}/${CCS_EXAMPLE_BUILD}
    ./build_all.sh ${TARGET_APP}.out
    TGT_APP_VER_LIST=`ls ${CCS_WORKSPACE_PATH}/${TARGET_APP}/${CCS_EXAMPLE_BUILD}/${TARGET_APP}.out__*`

    cd $PFORM_DIR
    Test=1
    while read TestType; 
    do
        for TGT_APP_VER in ${TGT_APP_VER_LIST}
        do
        (
            echo "Launching Test for :" $TGT_APP_VER
            export CCS_EXAMPLE_PATH=$TGT_APP_VER

            ./$SCRIPT $TestType

            echo "[Test # $Test] $SCRIPT $TestType [Application: $TGT_APP_VER]" >> ${APP_RESULTS_PATH}
            cat ${RESULTFILE} >> ${APP_RESULTS_PATH}
            echo "------------------------------------------------------------------------------------------------------------" >> ${APP_RESULTS_PATH}
            cat ${TTYLOGFILE} >> ${ALL_TTYLOGS_FILE}
            cat ${TTYCONFILE} >> ${ALL_TTYCONS_FILE}
            rm -f ${RESULTFILE} ${TTYLOGFILE} ${TTYCONFILE}
        )
        done
        echo "++++++++++++++++++++++++++++++++++++++++++ END OF TEST # $Test +++++++++++++++++++++++++++++++++++++++++++++++++" >> ${APP_RESULTS_PATH}
        (( Test = Test + 1 ))
    done < $HOME/$TESTS_FILE
    echo "########################################## END OF APPLICATION TEST #########################################" >> ${APP_RESULTS_PATH}
)
done

echo "Batch Test ... Completed: $(date)"
echo "Done !"

