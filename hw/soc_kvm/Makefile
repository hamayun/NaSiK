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
	@echo "    | Building  ... ${LIBSOCKVM}"
	@$(CC) $(LDFLAGS) -o $@ $^
	@rm -f *.o *.a *.d

#	$(AR) rcs $@ $^

%.o: %.c
	@echo "    | Compiling $(notdir $<) ..."
	@rm -fr $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(CINCDIR) -c $^ -o $@
	@echo -n "$(dir $@)" > $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(CINCDIR) -M $< >> $(addsuffix .d, $(basename $@))

clean:
	@$(RM) *.o *.a *.d ${LIBSOCKVM}

install: 
	@echo "    | Installing  ... ${LIBSOCKVM}"
	@install -D -m 644 soc_kvm.h $(LIBKVM_PREFIX)/include/soc_kvm.h
	@install -D -m 644 $(LIBSOCKVM) $(LIBKVM_PREFIX)/lib/$(LIBSOCKVM)

