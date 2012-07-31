SRC_BASE_DIR = $(NASIK_HOME)
COMPONENTS   = hw/components
PLATFORM     = examples/platforms/kroger
SUB_DIRS     = $(COMPONENTS)/abstract_noc       \
               $(COMPONENTS)/mem                \
               $(COMPONENTS)/kvm_wrapper_2      \
               $(COMPONENTS)/framebuffer        \
               $(COMPONENTS)/sem                \
               $(COMPONENTS)/slave_device       \
               $(COMPONENTS)/master_device      \
               $(COMPONENTS)/tg                 \
               $(COMPONENTS)/sl_tg              \
               $(COMPONENTS)/sl_tty             \
               $(COMPONENTS)/timer              \
               $(COMPONENTS)/sl_blockdevice     \
               $(PLATFORM)


CC           = g++
#OPT          = -O3
OPT          = -O0
DEBUG        = -Wall -g
#PROFILE      = -pg
OTHER        = -Wno-deprecated
CFLAGS       = $(DEBUG) $(OTHER) $(OPT) $(PROFILE)
LIBDIR       = -L$(SYSTEMC)/lib-$(TARGET_ARCH) -L$(LIBKVMTOOL_PREFIX)
LIBS         = -lsystemc -lSDL -lelf -lkvm-tool -lz -lrt -lpthread -lutil
TARGET_ARCH  = linux

INCDIR += -I$(SYSTEMC)/include
INCDIR += -I$(LIBKVM_PREFIX)/include
INCDIR += $(foreach SUB_DIR, $(SUB_DIRS), $(addprefix -I$(SRC_BASE_DIR)/, $(SUB_DIR)))

MODULE       = arch
EXE          = $(MODULE).x
SYMLINKS     = links

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

all: $(SYMLINKS) $(EXE)

$(EXE): $(OBJS)
	@echo "    | Building $@ ..."
	@$(CC) $(CFLAGS) $(INCDIR) $(LIBDIR) $(RLIBDIR) -o $@ $(OBJS) $(LIBS) 2>&1 | c++filt

$(SYMLINKS): ${APP_DIR}/APPLICATION.X
	@echo "    | Updating Application and Input File Symbolic Links ..."
	@ln -sf ${APP_DIR}/APPLICATION.X APPLICATION.X
	@ln -sf ${APP_DIR}/input_data input_data

$(BUILD_DIR)/%.o:$(SRC_BASE_DIR)/%.cpp
	@echo "    | Compiling $(notdir $<) ..."
	@rm -fr $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<
	@echo -n "$(dir $@)" > $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -M $< >> $(addsuffix .d, $(basename $@))

#	@$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $< -fvisibility=hidden

clean:
	@echo Cleaning ...
	@rm -f $(EXE)
	@rm -f logCPU* tty100
	@rm -f tty*.log hosttime_kvm.txt output_data
	@rm -f $(addsuffix /*.o, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	@rm -f $(addsuffix /*.d, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	@rm -fr $(BUILD_DIR)
	@ln -sf /dev/null APPLICATION.X
	@ln -sf /dev/null input_data

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif
