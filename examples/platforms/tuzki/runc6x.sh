#! /bin/bash

print_step "Running the Simulation ... "
rm -f tty100
export PATH=~/workspace/Rabbits-sls/rabbits/tools:$PATH
./arch.x kvm_c6x_bootstrap APPLICATION.X /home/hamayun/workspace_ccs/factorial/Debug/factorial.out

