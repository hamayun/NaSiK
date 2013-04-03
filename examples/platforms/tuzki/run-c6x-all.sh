#! /bin/bash

export PATH=$NASIK_HOME/tools:$PATH

APP_LIST=`ls ${APP_DIR}/*__*`

for APP in ${APP_LIST}
do
	echo "Executing ... "$APP
    ./arch.x kvm_c6x_bootstrap $APP
done
