#! /bin/bash

rm -f tty-cpu-*

export PATH=${NASIK_HOME}/tools/:$PATH

./arch.x BOOTSTRAP_SMP APPLICATION.X

# For viewing trace, if enabled.
# gtkwave -A waveforms.vcd waveforms.sav
