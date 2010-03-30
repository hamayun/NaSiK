
NAME=  libta.so
SRC_BASE_DIR=sources
BUILD_DIR=./build
SUPPORTED_OS= base dna
# List of subdirectories to compile starting from SRC_BASE_DIR
SUB_DIRS= . ./annotation 
SUB_DIRS+= $(addprefix ./eu/, $(addsuffix /proc,$(SUPPORTED_OS)) $(addsuffix /hal,$(SUPPORTED_OS)))
SUB_DIRS+= $(addprefix ./linker/, $(SUPPORTED_OS))

CC=     g++
OPT=    -O3
DEBUG=  -g -DDEBUG
WARN=   -Wall 
CFLAGS= $(OPT) $(DEBUG) $(WARN) 

AR = ar
ARFLAGS = rcs

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
INCDIR += -I./include
INCDIR += -I./include/annotation
INCDIR += -I./include/eu
INCDIR += -I./include/linker

LD = g++
LIBDIR = -L$(SYSTEMC)/lib-linux
LIBS = -ldl -lsystemc -lelf -lpthread -lSDL
LDFLAGS = -shared 

.SUFFIXES: .cpp .o .x .d
.PHONY: deps

default: all

all: info verif bin
	@echo '[TOOLS.....]'
	make -C ./tools

info:
	@echo 'CC=$(CC)'
	@echo 'OPT=$(OPT)'
	@echo 'DEBUG=$(DEBUG)'
	@echo 'WARN=$(WARN)'
	@echo "SUB_DIRS=$(SUB_DIRS)"
	@echo "SRCS=$(SRCS)"

verif:
	@echo '[BUILD.....] $(NAME)'
ifndef SYSTEMC 
	$(error SYSTEMC environment variable not set !!!)
endif
	if [ ! -d $(SYSTEMC) ] ; then echo $(SYSTEMC) not found !!!; exit 1; fi;
	mkdir -p $(addprefix $(BUILD_DIR)/, $(SUB_DIRS))

bin: $(DEPS) $(OBJS)
	@echo '    | linking $(NAME)'
	$(LD) $(LDFLAGS) -o $(NAME) $(OBJS) $(LIBDIR) $(LIBS)
	@echo 'done...'

#.cpp.o:
#	@echo '    | Compiling $<'
#	$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<

$(BUILD_DIR)/%.d:$(SRC_BASE_DIR)/%.cpp
	@echo '    | Dependencies $<'
	$(CC) $(CFLAGS) $(INCDIR) -M -MT $(addsuffix .o, $(basename $@)) -MF $(addsuffix .d, $(basename $@)) $<


$(BUILD_DIR)/%.o:$(SRC_BASE_DIR)/%.cpp
	@echo '    | Compiling $<'
	$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<

define INSTALL
	if [ -d $(1) ] ; then install -m 644 $(1)/*.h $(LIBTA_HOME)/$(1); fi;

endef

install: 
	@echo '[INSTALL...]'
ifndef LIBTA_HOME 
	$(error LIBTA_HOME environment variable not set !!!);
endif
	install -d $(LIBTA_HOME)
	install -d $(LIBTA_HOME)/lib
	install -d $(LIBTA_HOME)/include
	install -d $(LIBTA_HOME)/include/interfaces
	install -d $(LIBTA_HOME)/include/annotation
	install $(NAME) $(LIBTA_HOME)/lib
	install -m 644 ./include/*.h $(LIBTA_HOME)/include
	install -m 644 ./include/interfaces/*.h $(LIBTA_HOME)/include/interfaces
	install -m 644 ./include/annotation/*.h $(LIBTA_HOME)/include/annotation
	cp -rf ./include/eu $(LIBTA_HOME)/include/.
	cp -rf ./include/linker $(LIBTA_HOME)/include/.
	@echo '[TOOLS.....]'
	make -C ./tools install
	
clean:
	@echo '[CLEANING ...]'
	rm -f $(NAME)
	rm -f $(addsuffix /*.o, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	rm -f $(addsuffix /*.d, $(addprefix $(BUILD_DIR)/, $(SUB_DIRS)))
	rm -rf $(BUILD_DIR)
	@echo '[CLEANING TOOLS.....]'
	make -C ./tools clean

$(BUILD_DIR)/%.d:$(SRC_BASE_DIR)/%.cpp 
	@echo '    | build dependencies for $<'
	mkdir -p $(addprefix $(BUILD_DIR)/, $(SUB_DIRS))
	@echo -n "$(dir $@)" > $@
	$(CC) $(CFLAGS) $(INCDIR) -M $< >> $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

