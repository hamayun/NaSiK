SRC_DIR  = Sources
INCLUDES = Headers
OBJ_DIR  = obj
BIN_DIR  = bin

GCC     := gcc
OBJCOPY := objcopy
OBJDUMP := objdump

SRCS_C  := $(wildcard $(SRC_DIR)/*.c)
SRCS_S  := $(wildcard $(SRC_DIR)/*.S)

CFLAGS   = -g -O0 -Wall -nostdlib -Wstrict-prototypes -mregparm=3 -fno-strict-aliasing -fomit-frame-pointer -ffreestanding
CFLAGS  += -fno-toplevel-reorder -fno-stack-protector -mpreferred-stack-boundary=2 -m32 -march=i386
LDFLAGS += -nostdlib

OBJS_C  := $(SRCS_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJS_S  := $(SRCS_S:$(SRC_DIR)/%.S=$(OBJ_DIR)/%.o)
OBJS    := $(OBJS_C) $(OBJS_S)
DEPS    := $(OBJS:%.o=%.d)

ifeq ($(filter clean,$(MAKECMDGOALS)),)
    $(shell mkdir -p $(OBJ_DIR))
    $(shell mkdir -p $(BIN_DIR))
endif

SETUP_ELF := $(BIN_DIR)/KVMx86BootLoader.elf
SETUP_BIN := $(BIN_DIR)/KVMx86BootLoader.bin
SETUP_ASM := $(OBJ_DIR)/KVMx86BootLoader.asm

V=true
ifeq ($(V),)
    V := @
else
    V :=
endif

.PHONY: all clean

all: $(SETUP_ELF) $(SETUP_BIN) $(SETUP_ASM)
	@echo "Done !!!"

$(SETUP_BIN): $(SETUP_ELF)
	@echo "Object Copy ... $@"
	$(V)$(OBJCOPY) --strip-debug -Obinary $< $@

$(SETUP_ASM): $(SETUP_ELF)
	@echo "Object Dump ... $@"
	$(V)$(OBJDUMP) -mi8086 -D $< > $@

$(SETUP_ELF): $(OBJS)
	@echo "Linking     ... $@"
	$(V)$(GCC) -T kvmx86bootloader.ld -Wl,--build-id=none -o $@ $(CFLAGS) $(LDFLAGS) $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S
	@echo "Assembling  ... $@"
	$(V)$(GCC) -I$(INCLUDES) -c -MMD -o $@ $(CFLAGS) -D__ASSEMBLY__ $<

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling   ... $@"
	$(V)$(GCC) -I$(INCLUDES) -c -MMD -o $@ $(CFLAGS) $<

clean:
	@echo "Cleaning    ..."
	$(V)rm -fr $(OBJ_DIR) $(SETUP_ELF) $(SETUP_BIN) $(SETUP_ASM) $(BIN_DIR)
	@echo "Done !!!"

-include $(DEPS)

