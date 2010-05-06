CFLAGS =  $(TARGET_CFLAGS)

BUILDDIR := $(BUILDDIR)/APP

SRC=    $(wildcard *.c)
OBJ=    $(SRC:%.c=$(BUILDDIR)/%.o)

default: apes_objects

ifdef MAKEFILE_RULES
include $(MAKEFILE_RULES)
else
include $(APES_HOME)/support/Makefile.rules
endif
