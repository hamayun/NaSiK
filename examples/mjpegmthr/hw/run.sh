#!/bin/bash

if [ $USE_ANNOTATIONS = "yes" ] ; then 
	echo "Simulating with Annotations Option [$ANNOTATION]"
else
	echo "Simulating without Any Annotations"
fi

./arch.x configurations/config_native 

# With Online Analysis  
#./arch.x configurations/config_native --libta-option="analyze online"  

# And then use 
# grep "unpack_block:cost" output.txt-bugfix | cut -f 3 -d ':' > cost_instr.txt
# grep "unpack_block:cost" output.txt-bugfix | cut -f 4 -d ':' > cost_cycles.txt

#./arch.x configurations/config_native --libta-option="analyze online" --libta-debug="analyzer" 

# with no_thread option simulation is very slow
#./arch.x configurations/config_native --libta-option="analyze online no_thread" &> output.txt

# GTKWave 
# gtkwave -a waveforms.sav waveforms.vcd 
