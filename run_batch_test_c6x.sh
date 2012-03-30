#!/bin/bash

HOME=$PWD
TIMESTAMP=$(date +%Y-%m-%d-%Hh%M)
RESULTSDIR=$HOME/batchtest-c6x-${TIMESTAMP}
RESULTFILE=hosttime_kvm.txt
TTYLOGFILE=tty_debug_00
TTYCONFILE=tty_console_00
ALL_TTYLOGS_FILE=${HOME}/tty_debug_all.txt
ALL_TTYCONS_FILE=${HOME}/tty_console_all.txt
ALL_RESULTS_FILE=${HOME}/hosttime_kvm_results.txt

APPS_LIST="fibonacci matmult factorial"
TESTS_FILE="tests_list_c6x.txt"
SCRIPT="simulatec6x.sh"

echo "Batch Test ... Started: $(date)"
mkdir -p ${RESULTSDIR}

for TARGET_APP in ${APPS_LIST}
do
(
    echo "Launching Test for :" $TARGET_APP

    export CCS_EXAMPLE_NAME=$TARGET_APP
    export CCS_EXAMPLE_BUILD=Debug
    export CCS_EXAMPLE_OUTFILE=${CCS_EXAMPLE_NAME}.out
    export CCS_EXAMPLE_PATH=${CCS_WORKSPACE_PATH}/${CCS_EXAMPLE_NAME}/${CCS_EXAMPLE_BUILD}/${CCS_EXAMPLE_OUTFILE}
    source ${CCS_WORKSPACE_PATH}/install.sh

    cd $PFORM_DIR
    Test=1
    while read TestType; 
    do
        ./$SCRIPT $TestType

        echo "[Test # $Test] $SCRIPT $TestType [Application: $TARGET_APP]" >> ${ALL_RESULTS_FILE}
        cat ${RESULTFILE} >> ${ALL_RESULTS_FILE}
        echo "------------------------------------------------------------------------------------------------------------" >> ${ALL_RESULTS_FILE}
        cat ${TTYLOGFILE} >> ${ALL_TTYLOGS_FILE}
        cat ${TTYCONFILE} >> ${ALL_TTYCONS_FILE}
        rm -f ${RESULTFILE} ${TTYLOGFILE} ${TTYCONFILE}

        (( Test = Test + 1 ))
    done < $HOME/$TESTS_FILE
    echo "============================================================================================================" >> ${ALL_RESULTS_FILE}
)
done

mv -f ${ALL_RESULTS_FILE} ${RESULTSDIR}/
mv -f ${ALL_TTYLOGS_FILE} ${RESULTSDIR}/
mv -f ${ALL_TTYCONS_FILE} ${RESULTSDIR}/
echo "Batch Test ... Completed: $(date)"
echo "Done !"

