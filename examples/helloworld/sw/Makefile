export BUILDDIR = $(TARGET_PREFIX)
SUBDIRS = sources
ifeq ($(strip $(TARGET_CAL)),NATIVE)
SUBDIRS += ldscripts
endif

LDFLAGS  =
LIBOBJS =

ifneq ($(strip $(TARGET_SYSTEM_KSP_OS)),)
	LIBOBJS += $(BUILDDIR)/$(TARGET_SYSTEM_KSP_OS)/*.o
endif

ifneq ($(strip $(TARGET_SYSTEM_KSP_TASK)),)
	LIBOBJS += $(BUILDDIR)/$(TARGET_SYSTEM_KSP_TASK)/*.o
endif

ifneq ($(strip $(TARGET_SYSTEM_ASP_COM)),)
	LIBOBJS += $(BUILDDIR)/$(TARGET_SYSTEM_ASP_COM)/*.o
endif

ifneq ($(strip $(TARGET_SYSTEM_SSP_CAL)),)
	LIBOBJS += $(BUILDDIR)/$(TARGET_SYSTEM_SSP_CAL)/*.o
endif

ifneq ($(strip $(TARGET_LDSCRIPT)),)
	LDFLAGS += $(TARGET_LDSCRIPT)
endif

NAME = ${APP_NAME}

default: install_check apes apes_submake binary

export MAKEFILE_RULES=${PWD}/Makefile.rules.$(CONFIGURATION)

install_check:
	@if [ -z $(APES_HOME) ] ; then \
		echo '[ERROR  ] The APES environement has not been installed';\
		false;\
	fi;

apes: 
	make -C $(APES_HOME)
	echo '[BUILD...] $(NAME)'

clean:  install_check apes_clean 
	make -C $(APES_HOME) clean
	echo '[CLEAN...] $(NAME)'
	rm -rf ${NAME}
	rm -rf $(TARGET_PREFIX)

appclean: install_check
	rm -f $(BUILDDIR)/APP/*
	
binary : 
	$(TARGET_LD) -static $(TARGET_LDFLAGS) -o $(NAME) $(LIBOBJS) $(BUILDDIR)/APP/*.o $(LDFLAGS) $(STDINC) -lc -lgcc

ifneq ($(strip $(APES_HOME)),)

ifdef MAKEFILE_RULES
include $(MAKEFILE_RULES)
else
include $(APES_HOME)/support/Makefile.rules
endif

endif
