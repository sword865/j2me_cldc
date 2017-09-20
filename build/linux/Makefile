BUILDDIR=$(shell pwd)
TOP=../..
PLATFORM=linux
export LANG=C

UNAME=$(shell uname -a)
ifneq (,$(findstring Linux,$(UNAME)))
  export PLATFORM=linux
endif

SUBDIRS = \
  $(TOP)/tools/preverifier/build/linux \
  $(TOP)/api \

ifeq ($(DEBUG), true) 
   SUBDIRS += $(TOP)/tools/kdp
endif

ifneq ($(ROMIZING), false) 
   SUBDIRS += $(TOP)/tools/jcc
endif

SUBDIRS += $(TOP)/kvm/VmUnix/build

all clean : FORCE
	@for i in $(SUBDIRS) ; do \
	    echo ">>>Recursively making "$$i" "$@"..."; \
	    cd $$i; $(MAKE) $@ \
	    || exit 1; cd $(BUILDDIR); \
	    echo "<<<Finished Recursively making "$$i" "$@"." ; \
	done

FORCE: 
