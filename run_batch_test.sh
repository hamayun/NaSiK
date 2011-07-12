#!/bin/bash

TIMESTAMP=$(date +%Y-%m-%d-%Hh%M)
RESULTSDIR=$PFORM_DIR/batchtest-results-${TIMESTAMP}

APP_LIST="susan qsort dijkstra patricia blowfish rijndael sha CRC32 bitcount cjpeg djpeg stringsearch"

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

    echo "Simulating ..."
    ./run.sh

    echo "Profile Results for $APPLICATION ..." >> hosttime_kvm_results.txt
    cat hosttime_kvm.txt >> hosttime_kvm_results.txt
    echo "===============================================================================================================" >> hosttime_kvm_results.txt
    mv output_data ${RESULTSDIR}/output_data_$APPLICATION
    rm hosttime_kvm.txt
done

mv hosttime_kvm_results.txt ${RESULTSDIR}/
mv logCPUs ${RESULTSDIR}/
echo "Done !"

