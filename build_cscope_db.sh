#!/bin/bash

echo "Building cscope Database in ... " $PWD
find $PWD -name "*.cpp" -o -name "*.h" > cscope.files

# Build Database, Recursively with Inverted Index for Faster Searches
cscope -b -R -q

# Export CSCOPE_DB, so we can launch vim from anywhere within the root directory.
CSCOPE_DB=$PWD/cscope.out; export CSCOPE_DB  
