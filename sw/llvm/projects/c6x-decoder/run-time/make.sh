#!/bin/sh

llvm-dis GeneratedModule.bc -o GeneratedModule.ll
llc      GeneratedModule.bc -o GeneratedModule.s
as       GeneratedModule.s  -o GeneratedModule.o
gcc -o   GeneratedModule.x     GeneratedModule.o

#/opt/toolchains/apes-i386/i386-sls-dnaos/lib/crt0.o

