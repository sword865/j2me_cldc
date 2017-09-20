# 
# Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
# 
#

TOP=../../..
include $(TOP)/build/Makefile.inc

ifeq ($(PLATFORM), linux)
  export GCC=true
endif

ifeq ($(DEBUG), true)
   g = _g
else
   g =
endif

j =

ifeq ($(DEBUG_COLLECTOR), true)
COLLECTOR = collectorDebug.c
else
COLLECTOR = collector.c
endif

FP_FILES = fp_math.c fp_bytecodes.c k_cos.c k_rem_pio2.c \
           k_sin.c k_tan.c s_ceil.c s_copysign.c         \
           s_cos.c s_floor.c s_scalbn.c                  \
           s_sin.c s_tan.c e_rem_pio2.c                  \
           w_sqrt.c e_sqrt.c s_fabs.c

SRCFILES  = cache.c class.c fields.c frame.c garbage.c $(COLLECTOR)   \
            global.c interpret.c execute.c loader.c main.c native.c   \
            property.c thread.c nativeCore.c loaderFile.c hashtable.c \
            verifier.c log.c jar.c inflate.c  stackmap.c profiling.c  \
     	    pool.c runtime_md.c StartJVM.c                            \
            nativeFunctionTableUnix.c events.c resource.c             \
            verifierUtil.c 

ifeq ($(DEBUG), true)
   SRCFILES += debugger.c debuggerSocketIO.c debuggerOutputStream.c debuggerInputStream.c
   OTHER_FLAGS += -DENABLE_JAVA_DEBUGGER=1
else
   OTHER_FLAGS += -DENABLE_JAVA_DEBUGGER=0
endif

OTHER_FLAGS +=-DPADTABLE=1

ifeq ($(USE_JAM), true)
   OTHER_FLAGS += -DUSE_JAM=1
   SRCFILES += jam.c jamParse.c jamHttp.c jamStorage.c \
               socketProtocol.c socketProtocol_md.c
   j = _j
else

endif

ifeq ($(USE_KNI), true)
  OTHER_FLAGS += -DUSE_KNI=1
  SRCFILES += kni.c
endif

ifeq ($(ROMIZING), false) 
   ROMFLAGS = -DROMIZING=0
else
   SRCFILES += ROMjavaUnix.c
endif

all: kvm$(j)$(g)

OBJFILES = $(patsubst %.c,obj$(j)$(g)/%.o,$(SRCFILES))
FP_OBJFILES = $(patsubst %.c,fp_obj$(j)$(g)/%.o,$(FP_FILES))

vpath %.c  $(TOP)/kvm/VmCommon/src/ $(TOP)/kvm/VmExtra/src/fp \
	   $(TOP)/kvm/VmUnix/src/ $(TOP)/kvm/VmExtra/src/     \
	   $(TOP)/tools/jcc/ $(TOP)/jam/src

ifeq ($(PLATFORM), solaris)
LIBS =    -lm -lsocket -lnsl
CPPFLAGS = -DUNIX -DSOLARIS -D$(ARCH) \
	   -I$(TOP)/kvm/VmExtra/h -I$(TOP)/kvm/VmCommon/h \
	   -I$(TOP)/kvm/VmUnix/h -I$(TOP)/jam/h -I$(TOP)/kvm/VmCommon/src
endif

ifeq ($(PLATFORM), linux)
LIBS =    -lm -lnsl
CPPFLAGS = -DUNIX -DLINUX -D$(ARCH) \
	   -I$(TOP)/kvm/VmCommon/h -I$(TOP)/kvm/VmUnix/h \
	   -I$(TOP)/kvm/VmExtra/h -I$(TOP)/jam/h
endif

ifeq ($(GCC), true)
   CC = gcc
   CFLAGS =  -Wall $(CPPFLAGS) $(ROMFLAGS) $(OTHER_FLAGS)
   DEBUG_FLAG = -g 
   OPTIMIZE_FLAG = -O2
   FP_OPTIMIZE_FLAG =
else 
   CC = cc
   CFLAGS = -Xa $(CPPFLAGS) $(ROMFLAGS) $(OTHER_FLAGS)
   DEBUG_FLAG = -g -xsb
   OPTIMIZE_FLAG = -xO2
   FP_OPTIMIZE_FLAG =
endif

DEBUG_FLAG += -DINCLUDEDEBUGCODE=1

$(TOP)/tools/jcc/ROMjavaUnix.c $(TOP)/tools/jcc/nativeFunctionTableUnix.c: jccUnix

.PHONY:  jccUnix
jccUnix: 
	@(cd $(TOP)/tools/jcc; $(MAKE) unix)       

kvm$(j)$(g): obj$j$g/ fp_obj$j$g/ $(CLEANUPXPM) $(OBJFILES) $(FP_OBJFILES)
	@echo "Linking ... $@"
	@$(CC) $(OBJFILES) $(FP_OBJFILES) -o $@ $(LIBS)

clean: 
	rm -rf core kvm* .noincludexpm* obj* ./SunWS_cache fp_obj*
	rm -rf $(TOP)/tools/jcc/ROMjavaUnix.c $(TOP)/tools/jcc/nativeFunctionTableUnix.c

obj$(j)$g/execute.o : execute.c bytecodes.c 

obj$(j)/%.o: %.c
		@echo "... $@"
		@$(CC) $(EXTRACFLAGS) $(CFLAGS) $(OPTIMIZE_FLAG) -c -o $@ $<

obj$(j)_g/%.o: %.c
		@echo "... $@"
		@$(CC) $(EXTRACFLAGS) $(CFLAGS) $(DEBUG_FLAG) -c -o $@ $<

obj$j$g/:
		mkdir -p $@


fp_obj$(j)/%.o: %.c
		@echo "... $@"
		@$(CC) $(EXTRACFLAGS) $(CFLAGS) $(FP_OPTIMIZE_FLAG) -c -o $@ $<

fp_obj$(j)_g/%.o: %.c
		@echo "... $@"
		@$(CC) $(EXTRACFLAGS) $(CFLAGS) $(DEBUG_FLAG) -c -o $@ $<

fp_obj$j$g/:
		mkdir -p $@

ifeq ($(GCC), true)

%.i: %.c .FORCE
	echo $(CC)
	$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -E -o $@ $<

else 

%.i: %.c .FORCE
	echo $(CC)
	$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -E $< > $@

endif

%.s: %.c .FORCE
	@echo "... $@"
	@$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -S -o $@ $<

regression:
	CLASSPATH=../../../api/classes:../../../samples/classes kvm tests.RegressionTest

.FORCE:


