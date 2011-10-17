
include config.mak

DESTDIR :=

.PHONY: clean

#make sure env CFLAGS variable is not used
CFLAGS =
CFLAGS += -D__i386__ -m32 -O1
CFLAGS += $(autodepend-flags) -g -fomit-frame-pointer -Wall -fno-stack-protector
CFLAGS += -I $(KERNELDIR)/include -I ../libkvm 

TEST_DIR=test/x86

LDFLAGS += $(CFLAGS) -L ../libkvm

autodepend-flags = -MMD -MF $(dir $*).$(notdir $*).d

all: $(TEST_DIR)/dnastart.o $(TEST_DIR)/bootstrap $(TEST_DIR)/kvm_kickstart.o

$(TEST_DIR)/%.o: CFLAGS += -std=gnu99 -ffreestanding -I test/lib -I test/lib/x86

$(TEST_DIR)/bootstrap: $(TEST_DIR)/bootstrap.o
	$(CC) -nostdlib -o $@ -Wl,-T,bootstrap.lds $^

%.o: %.S
	$(CC) $(CFLAGS) -c -nostdlib -o $@ $^

-include .*.d

clean:
	$(RM) $(TEST_DIR)/dnastart.o $(TEST_DIR)/bootstrap.o $(TEST_DIR)/bootstrap $(TEST_DIR)/kvm_kickstart.o
