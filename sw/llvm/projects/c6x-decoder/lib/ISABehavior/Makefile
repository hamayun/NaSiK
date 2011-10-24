VERBOSE = true

OBJS 	  = $(SRCS:.c=.o)
DEPS 	  = $(SRCS:.c=.dep)

CC        = llvm-gcc
CFLAGS    =-g -O3 -Wall -Wno-format -std=c99 -U__linux__ --emit-llvm
INCDIR    =-I. -I../../include/

CFILES    = C62xISABehavior.c
TARGET    = C62xISABehavior.bc

quiet-command = $(if $(VERBOSE),$1,$(if $(2),@echo $2 && $1, @$1))

all : $(TARGET)

.PHONY  : clean

%.bc : %.c
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

#$(call quiet-command, $(CC) $(CFLAGS) $(INCDIR) -c $<, "  Compile Behavior Model ... $<")

clean:
	$(call quiet-command, rm -f $(TARGET) *~ *.bin, "  Clean ...    ")


