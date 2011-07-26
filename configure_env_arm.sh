export NASIK_HOME=$PWD

echo "[Setting-up Software Platform Environment (APES+Toolchains)]"
export SECONDARY_TOOLCHAIN=/opt/toolchains/apes-arm
export APES_ROOT=/opt/Apes
export APES_EXTRA_COMPS=$NASIK_HOME/sw/apes-components
source $APES_ROOT/install.sh
export APES_PATH=$APES_PATH:$APES_EXTRA_COMPS

#APPLICATION=kvmParallelMjpeg
#APPLICATION=susan
#APPLICATION=qsort
#APPLICATION=dijkstra
#APPLICATION=patricia
#APPLICATION=blowfish
#APPLICATION=rijndael
APPLICATION=sha
#APPLICATION=CRC32
#APPLICATION=bitcount
#APPLICATION=cjpeg
#APPLICATION=djpeg
#APPLICATION=stringsearch
#APPLICATION=kvmPi
#APPLICATION=selfProf
#APPLICATION=printFloats
export APPLICATION
export APP_DIR=$(find $NASIK_HOME/examples/applications -name "$APPLICATION")

echo "[Setting-up Software Application Environment ($APPLICATION)]"
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

# for tty_terms
export PATH=$NASIK_HOME:$PATH
export PATH=${SECONDARY_TOOLCHAIN}/bin:$PATH

cd $NASIK_HOME

echo "======================= GENERAL ENVIRONMENT SETTINGS ========================="
echo "APPLICATION             = $APP_DIR"
echo "APES_ROOT               = $APES_ROOT"
echo "APES EXTRA COMPONENTS   = $APES_EXTRA_COMPS"
echo "APES_PATH               = $APES_PATH"

echo "======================= APES COMPILATION TOOLCHAIN ==========================="
echo "APES_COMPILER           = $APES_COMPILER"
echo "APES_CC_FLAGS           = $APES_CC_FLAGS"
echo "APES_CC_OPTIMIZATIONS   = $APES_CC_OPTIMIZATIONS"
echo "APES_LINKER             = $APES_LINKER"
echo "APES_LINKER_FLAGS       = $APES_LINKER_FLAGS"
echo "APES_LINKER_TRAIL_FLAGS = $APES_LINKER_TRAIL_FLAGS"
echo "=============================================================================="

