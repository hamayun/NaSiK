#! /bin/bash

rm -f tty_console_00
rm -f tty_debug_00

print_step "Running the Simulation ... "

export PATH=~/workspace/Rabbits-sls/rabbits/tools:$PATH
./arch.x kvm_c6x_bootstrap APPLICATION.X ${CCS_EXAMPLE_PATH}
