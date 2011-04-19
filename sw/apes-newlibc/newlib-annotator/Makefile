
NAME=newlib-annotater
SRC_BASE_DIR=sources
BUILD_DIR=./build
# List of subdirectories to compile starting from SRC_BASE_DIR
SUB_DIRS= . 

CC=     gcc
OPT=    -O0
DEBUG=  -g 
WARN=   -Wall 
CFLAGS= $(OPT) $(DEBUG) $(WARN) 

# Build the list of source files relative to SRC_BASE_DIR directory
define FIND_SRCS
$(addprefix $(1)/, $(notdir $(wildcard $(SRC_BASE_DIR)/$(1)/*.c)))
endef
TMPSRCS=$(foreach SUB_DIR, $(SUB_DIRS), $(call FIND_SRCS,$(SUB_DIR)))
# Build the list of object files relative to the BUILD_DIR directory
SRCS=$(addprefix $(SRC_BASE_DIR)/, $(TMPSRCS))
OBJS=$(addprefix $(BUILD_DIR)/, $(TMPSRCS:.c=.o))
DEPS=$(OBJS:.o=.d)

INCDIR += -I./include

LD = gcc
LIBDIR = 
LIBS =
LDFLAGS =

.SUFFIXES: .c .o .x .d

default: all

all: info verif bin

info:
	@echo 'CC=$(CC)'
	@echo 'OPT=$(OPT)'
	@echo 'DEBUG=$(DEBUG)'
	@echo 'WARN=$(WARN)'
	@echo "SRCS=$(SRCS)"

verif:
	@echo '[BUILD.....] $(NAME)'
	mkdir -p $(addprefix $(BUILD_DIR)/, $(SUB_DIRS))

bin: $(OBJS) 
	@echo '    | linking $(NAME)'
	$(LD) $(LDFLAGS) -o $(NAME) $(OBJS) $(LIBDIR) $(LIBS)
	@echo 'done...'

$(BUILD_DIR)/%.o:$(SRC_BASE_DIR)/%.c
	@echo '    | Compiling $<'
	@echo -n "$(dir $@)" > $(addsuffix .d, $(basename $@))
	$(CC) $(CFLAGS) $(INCDIR) -M $< >> $(addsuffix .d, $(basename $@))
	$(CC) $(CFLAGS) $(INCDIR) -o $@ -c $<

install: 
ifndef ANNOTATOR_INSTALL_PATH 
	$(error ANNOTATOR_INSTALL_PATH environment variable not set !!!);
endif

ifndef ANNOTATOR_INSTALL_NAME 
	$(error ANNOTATOR_INSTALL_NAME environment variable not set !!!);
endif

	@echo '[INSTALLING ANNOTATOR TO: $ANNOTATOR_INSTALL_PATH]'

	install -d $(ANNOTATOR_INSTALL_PATH)/bin
	install $(NAME) $(ANNOTATOR_INSTALL_PATH)/bin/$(ANNOTATOR_INSTALL_NAME)
	
clean:
	@rm -rf $(NAME)
	@rm -rf $(BUILD_DIR)
	
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

