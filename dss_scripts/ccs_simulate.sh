#!/bin/bash

# Place this script + associated javascript in /opt/ti/ccsv5/ccs_base/scripting/examples/DebugServerExamples 
# And then Execute it.

APP_LIST=`ls $1/*.out__*`

echo "Testing with CCS C64x (Device Functional) Simulator ..."
for TARGET_APP in $APP_LIST
do
(
    echo "Launching Test for :" $TARGET_APP
    ../../bin/dss.sh ccs_simulate.js $TARGET_APP
    echo "----------------------------------------------------------------------"
)
done

echo "======================================================================"
echo "Now Testing with CCS C64x+ (Full Cycle Accurate) Simulator ..."

for TARGET_APP in $APP_LIST
do
(
    echo "Launching Test for :" $TARGET_APP
    ../../bin/dss.sh ccs_simulate.js $TARGET_APP -p
    echo "----------------------------------------------------------------------"
)
done

echo "======================================================================"
echo "Now Testing with CCS Custom Simulator ..."

for TARGET_APP in $APP_LIST
do
(
    echo "Launching Test for :" $TARGET_APP
    ../../bin/dss.sh ccs_simulate.js $TARGET_APP -q
    echo "----------------------------------------------------------------------"
)
done

echo "Done !!!"

