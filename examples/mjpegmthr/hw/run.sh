#!/bin/bash

./arch.x configurations/config_hw configurations/config_dna

# With Online Analysis  
#./arch.x configurations/config_native --native-option="analyze online"  

# And then use 
# grep "unpack_block:cost" output.txt-bugfix | cut -f 3 -d ':' > cost_instr.txt
# grep "unpack_block:cost" output.txt-bugfix | cut -f 4 -d ':' > cost_cycles.txt

#./arch.x configurations/config_native --native-option="analyze online" --native-debug="analyzer" 

# with no_thread option simulation is very slow
#./arch.x configurations/config_native --native-option="analyze online no_thread" &> output.txt

# GTKWave 
# gtkwave -a waveforms.sav waveforms.vcd 
