#! /bin/bash

export PATH=~/workspace/Rabbits-sls/rabbits/tools:$PATH

APP_LIST=`ls ${APP_DIR}/*__*`

for APP in ${APP_LIST}
do
	echo "Executing ... "$APP
    ./arch.x bootstrap $APP
done
