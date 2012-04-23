#! /bin/bash

rm -f $1__*

for index in {1..30}
do
    export CUSTOM_PARAM=-DCUSTOM_INDEX=$index
	source ../config-apps-kvm.sh
    touch Sources/*
    apes-compose

    if [ $index -lt 10 ]; then
        mv $1 $1__0$index
    else
        mv $1 $1__$index
    fi
done


