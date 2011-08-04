#!/bin/bash

# When compiling with LLVM we have to use -msoft-float to link proper version of libgcc.a
if [ ${!APES_SPLIT_CC[@]} ]; then
export APES_CC1_FLAGS=${APES_CC1_FLAGS}" -msoft-float"
export APES_CC_FLAGS=${APES_CC_FLAGS}" -msoft-float"
export APES_LINKER_FLAGS=${APES_LINKER_FLAGS}" -msoft-float"
fi
