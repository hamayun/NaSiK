LIBSOCKVM    = libsoc_kvm.so
CC           = gcc
OPT          = -O3
DEBUG        = -Wall -g
#PROFILE      = -pg
OTHER        = -Wno-deprecated

CFLAGS       = $(DEBUG) $(OTHER) $(OPT) $(PROFILE)
LDFLAGS      = -shared $(PROFILE)
CINCDIR      = -I${LIBKVM_HOME}/user/test/lib/x86
CINCDIR     += -I${LIBKVM_HOME}/user/test/x86/
CINCDIR     += -I${LIBKVM_HOME}/libkvm

all: $(LIBSOCKVM)

$(LIBSOCKVM): iotable.o soc_kvm.o hosttime.o
	@$(CC) $(LDFLAGS) -o $@ $^

#	$(AR) rcs $@ $^

%.o: %.c
	@$(CC) $(CFLAGS) $(CINCDIR) -c $^ -o $@

clean:
	@$(RM) *.o *.a .*.d

install: 
	@install -D -m 644 soc_kvm.h $(LIBKVM_PREFIX)/include/soc_kvm.h
	@install -D -m 644 $(LIBSOCKVM) $(LIBKVM_PREFIX)/lib/$(LIBSOCKVM)

