#! /bin/bash

rm -f tty100
export PATH=${NASIK_HOME}/tools/:$PATH

./arch.x BOOTSTRAP_SMP APPLICATION.X
