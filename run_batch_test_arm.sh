#!/bin/bash

TIMESTAMP=$(date +%Y-%m-%d-%Hh%M)
ARM_PLATFORM=/home/hamayun/workspace/Rabbits-sls/platforms/thumper
export PATH=/home/hamayun/workspace/Rabbits-sls/rabbits/tools:$PATH

RESULTSDIR=${ARM_PLATFORM}/batchtest-results-${TIMESTAMP}
RESULTFILE=hosttime_arm.txt
TTYLOGFILE=tty100
ALL_TTYLOGS_FILE=tty100_all_tests.txt
ALL_RESULTS_FILE=hosttime_arm_results.txt

#APP_LIST="susan qsort dijkstra patricia blowfish rijndael sha CRC32 bitcount cjpeg djpeg stringsearch"
APP_LIST="patricia blowfish rijndael sha CRC32 bitcount cjpeg djpeg stringsearch"

echo "Batch Test ... Started: $(date)"
mkdir -p ${RESULTSDIR}

for APPLICATION in ${APP_LIST}
do
    export APPLICATION
    export APP_DIR=$(find $NASIK_HOME/examples/applications -name "$APPLICATION")

    echo "Configuring Application ... $APPLICATION"
    cd $NASIK_HOME/examples/applications
    source config-apps-arm.sh

    if [ -e ${APP_DIR}/app_specific_config.sh ]; then
	    echo "Application Specific Configuration Found ... Sourcing it."
	    cd ${APP_DIR}
	    source app_specific_config.sh
    fi

    #Update Links in Application.
    echo "Updating Application Specific Symlinks ..."
    cd ${APP_DIR}
    ln -sf $NASIK_HOME/examples/applications/ldscript_elf.arm elf.lds
    ln -sf interface.xmi.arm interface.xmi

    echo "Building Application ..."
    apes-compose
    arm-sls-dnaos-objcopy -Obinary APPLICATION.X APP.bin
    
    cd ${ARM_PLATFORM}
    ln -sf ${APP_DIR}/input_data input_data

    echo "Simulation of $APPLICATION Started At: $(date)"
    ./run.x -cpu arm11mpcore -kernel ${APP_DIR}/APP.bin -ncpu 1

    echo "Profile Results for $APPLICATION ..." >> ${ALL_RESULTS_FILE}
    cat ${RESULTFILE} >> ${ALL_RESULTS_FILE}
    echo "===============================================================================================================" >> ${ALL_RESULTS_FILE}
    mv output_data ${RESULTSDIR}/output_data_$APPLICATION
    cat ${TTYLOGFILE} >> ${ALL_TTYLOGS_FILE}
    rm -f ${RESULTFILE} ${TTYLOGFILE}
done

mv ${ALL_RESULTS_FILE} ${RESULTSDIR}/
mv ${ALL_TTYLOGS_FILE} ${RESULTSDIR}/
echo "Batch Test ... Completed: $(date)"
echo "Done !"

