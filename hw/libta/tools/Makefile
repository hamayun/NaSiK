SUBDIRS=sc_config 

all:
ifneq ($(strip $(SUBDIRS)),)
	@for i in $(SUBDIRS) ; do make -C $$i || exit $? ; done;
endif

install:
ifneq ($(strip $(SUBDIRS)),)
	@for i in $(SUBDIRS) ; do make -C $$i install || exit $? ; done;
endif

clean:
ifneq ($(strip $(SUBDIRS)),)
	@for i in $(SUBDIRS) ; do make -C $$i clean || exit $? ; done;
endif

