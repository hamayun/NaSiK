
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

all: $(TEST_DIR)/dnastart.o $(TEST_DIR)/bootstrap $(TEST_DIR)/kvm_kickstart.o $(TEST_DIR)/kvm_c6x_bootstrap

$(TEST_DIR)/%.o: CFLAGS += -std=gnu99 -ffreestanding -I test/lib -I test/lib/x86

$(TEST_DIR)/bootstrap: $(TEST_DIR)/bootstrap.o
	@echo "  Linking    ... "$<
	@$(CC) -nostdlib -o $@ -Wl,-T,bootstrap.lds $^

$(TEST_DIR)/kvm_c6x_bootstrap: $(TEST_DIR)/kvm_c6x_bootstrap.o
	@echo "  Linking    ... "$<
	@$(CC) -nostdlib -o $@ -Wl,-T,bootstrap.lds $^

%.o: %.S
	@echo "  Assembling ... "$<
	@$(CC) $(CFLAGS) -c -nostdlib -o $@ $^

-include .*.d

clean:
	@echo "  Clean ..."
	@$(RM) $(TEST_DIR)/dnastart.o $(TEST_DIR)/kvm_kickstart.o
	@$(RM) $(TEST_DIR)/bootstrap.o $(TEST_DIR)/bootstrap
	@$(RM) $(TEST_DIR)/kvm_c6x_bootstrap.o $(TEST_DIR)/kvm_c6x_bootstrap
