LIBSOCKVM    = libsoc_kvm.so
CC           = gcc
OPT          = -O0 
DEBUG        = -g
OTHER        = -fPIC

CFLAGS       = $(DEBUG) $(OTHER) $(OPT)
LDFLAGS      = -shared
CINCDIR      = -I${LIBKVM_HOME}/user/test/lib/x86 
CINCDIR     += -I${LIBKVM_HOME}/user/test/x86/
CINCDIR     += -I${LIBKVM_HOME}/libkvm

all: $(LIBSOCKVM)

$(LIBSOCKVM): iotable.o soc_kvm.o
	$(CC) -shared -o $@ $^

#	$(AR) rcs $@ $^

iotable.o: iotable.c
	$(CC) $(CFLAGS) $(CINCDIR) -c $^ -o $@ 

soc_kvm.o: soc_kvm.c
	$(CC) $(CFLAGS) $(CINCDIR) -c $^ -o $@ 

clean:
	$(RM) *.o *.a .*.d

install: 
	install -D -m 644 soc_kvm.h $(LIBKVM_PREFIX)/include/soc_kvm.h
	install -D -m 644 $(LIBSOCKVM) $(LIBKVM_PREFIX)/lib/$(LIBSOCKVM)

