# We execute without tracing and for one processor

./arch.x 1 trace_off configurations/config_dna configurations/config_hw >& log.txt
#./arch.x 1 trace_off configurations/config_dna configurations/config_hw --native-option="analyze online" >& log.txt

#./arch.x 1 trace_off configurations/config_dna configurations/config_hw --native-option="analyze online" >& output.txt

# Here are a few debug options for native (libta) library. 
# Usage: --native-debug="<option-1, option-2, ...>"
# Options: analyzer eu_base dna_eu spinlock dna_hal dna_eu_context generic_noc ... 

# With Online Analysis  
#./arch.x 1 trace_off configurations/config_dna configurations/config_hw --native-option="analyze online" >& output.txt

# And then use 
# grep "unpack_block:cost" output.txt | cut -f 3 -d ':' > cost_instr.txt
# grep "unpack_block:cost" output.txt | cut -f 4 -d ':' > cost_cycles.txt

#./arch.x 1 trace_off configurations/config_dna configurations/config_hw --native-option="analyze online" --native-debug="analyzer" 

# with no_thread option simulation is very slow
#./arch.x 1 trace_off configurations/config_dna configurations/config_hw --native-option="analyze online" --native-debug="no_thread" &> output.txt

# GTKWave 
# gtkwave -a waveforms.sav waveforms.vcd 

