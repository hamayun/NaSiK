.globl  inc_secondary_start

.data
inc_secondary_start:

        .incbin "bin/KVMx86SecondaryBoot.bin"

        .globl  inc_secondary_end
inc_secondary_end:


