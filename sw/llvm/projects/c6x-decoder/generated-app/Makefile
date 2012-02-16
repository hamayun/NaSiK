TOOLCHAIN=/opt/toolchains/i386-pc-baremetal
KVMSTART=/home/hamayun/workspace/NaSiK/hw/kvm-85/user/test/x86/kvm_kickstart.o
CRT0=${TOOLCHAIN}/i386-pc-baremetal/lib/crt0.o

SRCS      = $(wildcard *.c)
OBJS      = $(SRCS:.c=.o)
DEPS 	  = $(SRCS:.c=.dep)
GEN_SRCS  = $(wildcard *.bc)
GEN_OBJS  = $(GEN_SRCS:.bc=.o)
GEN_ASMS  = $(GEN_SRCS:.bc=.ll)

CC        = i386-pc-baremetal-gcc
LLC       = llc
LLVMDIS   = llvm-dis
LD        = i386-pc-baremetal-gcc
AS        = i386-pc-baremetal-as

INCDIR    =-I${NASIK_HOME}/sw/apes-components/KVMx86Processor/Headers/Public/
INCDIR   +=-I${C6XDEC_SOURCE}/lib/ISABehavior
INCDIR   +=-I${C6XDEC_SOURCE}/include
INCDIR   +=-I${TOOLCHAIN}/i386-pc-baremetal/include
INCDIR   +=-I${TOOLCHAIN}/lib/gcc/i386-pc-baremetal/4.5.0/include

CFLAGS    = -Wall -Wno-format -std=c99 -g -O3 -nostdinc
LDFLAGS   =${CRT0} ${KVMSTART} -Wl,-T,ldscript.kvm -nostdlib -L${TOOLCHAIN}/i386-pc-baremetal/lib -lc -lgcc

TARGET    = APPLICATION.X

all :  $(GEN_ASMS) $(TARGET)

.PHONY  : clean

%.s : %.bc
	@echo "  Compiling   ... " $<
	@$(LLC) $< -o $@

%.ll : %.bc
	@echo "  Disassembly ... " $<
	@$(LLVMDIS) $< -o $@

%.o : %.s
	@echo "  Assembling  ... " $<
	@$(AS) $< -o $@

%.o : %.c
	@echo "  Compiling   ... " $<
	@$(CC) $(CFLAGS) $(INCDIR) -c $<

$(TARGET) : $(OBJS) $(GEN_OBJS)
	@echo "  Linking     ... " $@
	@$(LD) $^ -o $@ $(LDFLAGS)

clean:
	@echo "  Clean ... "
	@rm -f $(OBJS) $(GEN_OBJS) $(GEN_ASMS) $(TARGET) *~
