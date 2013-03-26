#! /bin/bash

rm -f tty-cpu-*
rm -fr logCPU*
rm -fr qemu_f*.lst

export PATH=${NASIK_HOME}/tools/:$PATH

./arch.x -cpu arm11mpcore -ncpu 1 -kernel qemu_soft/linux/linux/arch/arm/boot/zImage \
    -initrd qemu_soft/initrd/initrd.gz -kvm_bootstrap BOOTSTRAP_SMP -kvm_software APPLICATION.X "$@"

# For viewing trace, if enabled.
# gtkwave -A waveforms.vcd waveforms.sav
