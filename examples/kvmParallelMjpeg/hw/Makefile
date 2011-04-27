SRC_BASE_DIR = $(NASIK_HOME)
COMPONENTS   = hw/components
PLATFORM     = examples/$(APPLICATION)/hw
SUB_DIRS     = $(COMPONENTS)/abstract_noc       \
               $(COMPONENTS)/mem                \
               $(COMPONENTS)/kvm_wrapper        \
               $(COMPONENTS)/ramdac             \
               $(COMPONENTS)/framebuffer        \
               $(COMPONENTS)/sem                \
               $(COMPONENTS)/slave_device       \
               $(COMPONENTS)/master_device      \
               $(COMPONENTS)/tg                 \
               $(COMPONENTS)/sl_tg              \
               $(COMPONENTS)/timer              \
               $(COMPONENTS)/tty_serial         \
               $(COMPONENTS)/sl_blockdevice     \
               $(COMPONENTS)/hosttime           \
               $(PLATFORM)

CC           = g++
OPT          = -O0 
DEBUG        = -g
OTHER        =  -Wno-deprecated
CFLAGS       = $(DEBUG) $(OTHER) $(OPT)
LIBDIR       = -L$(SYSTEMC)/lib-$(TARGET_ARCH) -L$(LIBKVM_PREFIX)/lib 
LIBS         = -lsystemc -ldl -lpthread -lelf -lsoc_kvm -lkvm -lSDL -lc -lstdc++
TARGET_ARCH  = linux

INCDIR += -I$(SYSTEMC)/include
INCDIR += -I$(LIBKVM_PREFIX)/include
INCDIR += $(foreach SUB_DIR, $(SUB_DIRS), $(addprefix -I$(SRC_BASE_DIR)/, $(SUB_DIR)))

MODULE       = arch
EXE          = $(MODULE).x

BUILD_DIR    = ./objs
define FIND_SRCS
$(addprefix $(1)/, $(notdir $(wildcard $(SRC_BASE_DIR)/$(1)/*.cpp)))
endef
TMPSRCS=$(foreach SUB_DIR, $(SUB_DIRS), $(call FIND_SRCS,$(SUB_DIR)))
SRCS=$(addprefix $(SRC_BASE_DIR)/, $(TMPSRCS))
OBJS=$(addprefix $(BUILD_DIR)/, $(TMPSRCS:.cpp=.o))
DEPS=$(OBJS:.o=.d)

$(shell mkdir -p $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))

.SUFFIXES: .cc .cpp .o .x
.PHONY: clean all graphs verif

all: $(EXE)

$(EXE): $(OBJS)
	@echo Building $@ ...
	@$(CC) $(CFLAGS) $(INCDIR) $(LIBDIR) $(RLIBDIR) -o $@ $(OBJS) $(LIBS) 2>&1 | c++filt

$(BUILD_DIR)/%.o:$(SRC_BASE_DIR)/%.cpp
	@echo "Compiling $(notdir $<) ..."
	@rm -fr $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $< -fvisibility=hidden 
	@echo -n "$(dir $@)" > $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -M $< >> $(addsuffix .d, $(basename $@))

clean:
	@echo Cleaning ...
	@rm -f $(EXE)
	@rm -f logCPU*
	@rm -f aa
	@rm -f $(addsuffix /*.o, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	@rm -f $(addsuffix /*.d, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	@rm -fr $(BUILD_DIR)

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
