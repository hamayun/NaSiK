#!/bin/bash

if [ $USE_ANNOTATIONS = "yes" ] ; then 
	echo "Simulating with Annotations Option [$ANNOTATION]"
else
	echo "Simulating without Any Annotations"
fi 

./arch.x configurations/config_native


