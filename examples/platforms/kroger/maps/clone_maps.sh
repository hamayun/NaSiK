#!/bin/bash

SRC_MAP=$1
BASE_NAME=$2
FROM=$3
TO=$4

if [ $# -lt 4 ]; then
	echo "Usage: $0 <source_map_file> <base_name> <from_count> <to_count>"
	exit;
fi

echo "Cloning Node Map ... ${SRC_MAP} From: ${FROM} ... To: ${TO}"

for (( i=${FROM}; i<=${TO}; i++ ))
do
	#echo "cp ${SRC_MAP} ${BASE_NAME}${i}.map"
	#cp ${SRC_MAP} ${BASE_NAME}${i}.map
	echo "linking ${SRC_MAP} ${BASE_NAME}${i}.map"
	ln -sf ${SRC_MAP} ${BASE_NAME}${i}.map
done

echo "Done !"

