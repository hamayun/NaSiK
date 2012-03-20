
SRCS      = $(wildcard *.c)
OBJS 	  = $(SRCS:.c=.bc)
ASMS 	  = $(SRCS:.c=.ll)
DEPS 	  = $(SRCS:.c=.dep)

CC        = llvm-gcc
LLVMDIS   = llvm-dis
CFLAGS    =-g -O3 -Wall -Wno-format -std=c99 --emit-llvm ${ISA_EXTRA_FLAGS}
INCDIR    =-I. -I../../include/
INCDIR   +=-I/opt/toolchains/i386-pc-baremetal/i386-pc-baremetal/include
INCDIR   +=-I/opt/toolchains/i386-pc-baremetal/lib/gcc/i386-pc-baremetal/4.5.0/include

all : $(OBJS) $(ASMS)

.PHONY  : clean

%.bc : %.c
	@echo "  Compile ... $<"
	@$(CC) $(CFLAGS) -nostdinc $(INCDIR) -c $< -o $@
	@echo "  Copying ... $@"
	@cp -f $@ ${GENERATED_APP}/

%.ll : %.bc
	@echo "  Disassemble ... $<"
	@$(LLVMDIS) $< -o $@

clean:
	@echo "  Clean ..."
	@rm -f $(OBJS) $(ASMS) *~ *.o *.bin

