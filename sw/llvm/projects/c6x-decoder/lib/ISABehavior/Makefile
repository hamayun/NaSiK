OBJS 	  = $(SRCS:.c=.o)
DEPS 	  = $(SRCS:.c=.dep)

CC        = llvm-gcc
LLVMDIS   = llvm-dis
CFLAGS    =-g -O3 -Wall -Wno-format -std=c99 --emit-llvm
INCDIR    =-I. -I../../include/
INCDIR   +=-I/opt/toolchains/i386-pc-baremetal/i386-pc-baremetal/include
INCDIR   +=-I/opt/toolchains/i386-pc-baremetal/lib/gcc/i386-pc-baremetal/4.5.0/include

CFILES    = C62xISABehavior.c
TARGET    = C62xISABehavior.bc
ASM       = C62xISABehavior.ll

all : $(TARGET) $(ASM)

.PHONY  : clean

%.bc : %.c
	@echo "  Compile ... $<"
	@$(CC) $(CFLAGS) -nostdinc $(INCDIR) -c $< -o $@

%.ll : %.bc
	@echo "  Disassemble ... $<"
	@$(LLVMDIS) $< -o $@

clean:
	@echo "  Clean ..."
	@rm -f $(TARGET) $(ASM) *~ *.o *.bin

