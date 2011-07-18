#!/bin/bash

TIMESTAMP=$(date +%Y-%m-%d-%Hh%M)
RESULTSDIR=$PFORM_DIR/batchtest-results-${TIMESTAMP}
RESULTFILE=hosttime_kvm.txt
TTYLOGFILE=logCPUs
ALL_TTYLOGS_FILE=logCPUs_all_tests.txt
ALL_RESULTS_FILE=hosttime_kvm_results.txt

APP_LIST="susan qsort dijkstra patricia blowfish rijndael sha CRC32 bitcount cjpeg djpeg stringsearch"

echo "Batch Test ... Started: $(date)"
mkdir -p ${RESULTSDIR}

for APPLICATION in ${APP_LIST}
do
    export APPLICATION
    export APP_DIR=$(find $NASIK_HOME/examples/applications -name "$APPLICATION")

    echo "Configuring Application ... $APPLICATION"
    cd $NASIK_HOME/examples/applications
    source config-apps-kvm.sh

    if [ -e ${APP_DIR}/app_specific_config.sh ]; then
	    echo "Application Specific Configuration Found ... Sourcing it."
	    cd ${APP_DIR}
	    source app_specific_config.sh
    fi

    #Update Links in Application.
    echo "Updating Application Specific Symlinks ..."
    cd ${APP_DIR}
    ln -sf $NASIK_HOME/examples/applications/ldscript_elf.kvm elf.lds
    ln -sf interface.xmi.kvm interface.xmi

    echo "Building Application ..."
    apes-compose
    cd $PFORM_DIR
    make 

    echo "Simulation of $APPLICATION Started At: $(date)"
    ./run.sh

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

