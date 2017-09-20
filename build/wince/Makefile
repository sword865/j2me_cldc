
# To build the preverifier from EVC environment please set
# variables X86_PATH;X86_INCLUDE;X86_LIB to MSVC resources

BUILDDIR=$(shell pwd)
TOP=../..
PLATFORM=win32
export PLATFORM

ifndef USE_JAM
    USE_JAM=true
    export USE_JAM
endif

SUBDIRS = $(TOP)/api
  
ifeq ($(DEBUG), true) 
   SUBDIRS += $(TOP)/tools/kdp
endif

ifneq ($(ROMIZING), false) 
   SUBDIRS += $(TOP)/tools/jcc
endif

SUBDIRS += $(TOP)/kvm/VmWinCE/build

all clean : FORCE
	@(cd $(TOP)/tools/preverifier/build/win32; \
 	 export PATH="$(X86_PATH);$(PATH)";       \
	 $(MAKE) -e $@ LIB="$(X86_LIB)" INCLUDE="$(X86_INCLUDE)")
	for i in $(SUBDIRS) ; do \
	    echo ">>>Recursively making "$$i" "$@"..."; \
	    cd $$i; $(MAKE) $@ \
	    || exit 1; cd $(BUILDDIR); \
	    echo "<<<Finished Recursively making "$$i" "$@"." ; \
	done
FORCE: ;
