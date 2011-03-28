NAME= arch.x 
SRC_BASE_DIR=sources
BUILD_DIR=./build
# List of subdirectories to compile starting from SRC_BASE_DIR
SUB_DIRS= . 

CC=     g++
OPT=    -O0
DEBUG=  -g -DDEBUG
WARN=   -Wall 
CFLAGS= $(OPT) $(DEBUG) $(WARN) 

# Build the list of source files relative to SRC_BASE_DIR directory
define FIND_SRCS
$(addprefix $(1)/, $(notdir $(wildcard $(SRC_BASE_DIR)/$(1)/*.cpp)))
endef

TMPSRCS=$(foreach SUB_DIR, $(SUB_DIRS), $(call FIND_SRCS,$(SUB_DIR)))

# Build the list of object files relative to the BUILD_DIR directory
SRCS=$(addprefix $(SRC_BASE_DIR)/, $(TMPSRCS))
OBJS=$(addprefix $(BUILD_DIR)/, $(TMPSRCS:.cpp=.o))
DEPS=$(OBJS:.o=.d)

INCDIR = -I$(SYSTEMC)/include
INCDIR += -I$(LIBTA_HOME)/include
INCDIR += -I$(LIBTA_HOME)/include/eu
INCDIR += -I$(LIBTA_HOME)/include/annotation
INCDIR += -I$(LIBTA_HOME)/include/linker

LD = g++
LIBDIR = -L$(SYSTEMC)/lib-linux -L$(LIBTA_HOME)/lib -Wl,--export-dynamic 
LIBS = -ldl -lnative -lsystemc -lsc_config
LDFLAGS = 

.SUFFIXES: .cpp .o .x .d

default: all

all: verif bin

verif:
	@echo '[BUILD.....] $(NAME)'

ifndef SYSTEMC 
	@$(error SYSTEMC environment variable not set !!!)
endif

ifndef LIBTA_HOME 
	@$(error LIBTA_HOME environment variable not set !!!)
endif

	@if [ ! -d $(SYSTEMC) ] ; then echo $(SYSTEMC) not found !!!; exit 1; fi;
	@mkdir -p $(addprefix $(BUILD_DIR)/, $(SUB_DIRS))

bin: $(OBJS) 
	@echo '    | linking $(NAME)'
	$(LD) $(LDFLAGS) -o $(NAME) $(OBJS) $(LIBDIR) $(LIBS)
	@echo 'done...'

$(BUILD_DIR)/%.o:$(SRC_BASE_DIR)/%.cpp
	@echo '    | Compiling $<'
	@echo -n "$(dir $@)" > $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -M $< >> $(addsuffix .d, $(basename $@))
	@$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<

clean:
	@echo '[CLEANING ...]'
	@rm -f $(NAME)
	@rm -f $(addsuffix /*.o, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	@rm -f $(addsuffix /*.d, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	@rm -f *.vcd
	@rm -rf build

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

