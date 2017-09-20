# 
# Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
# 
#

#
# Makefile for GNU make, adapted from the Solaris version
# 18:20:24 01/14/03 @(#)Makefile	1.11
#

TOP=../../..
include $(TOP)/build/Makefile.inc

CC  = clarm
LD  = link
RSC = rc

CECrt=C
CECrtDebug=C
CECrtMT=C
CECrtMTDebug=C
CESubsystem=windowsce,3.00
CEVersion=300
CENoDefaultLib=libc.lib /nodefaultlib:libcd.lib /nodefaultlib:libcmt.lib \
               /nodefaultlib:libcmtd.lib /nodefaultlib:msvcrt.lib        \
               /nodefaultlib:msvcrtd.lib

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

FP_FILES = fp_math.c fp_bytecodes.c k_rem_pio2.c    \
           k_cos.c k_sin.c k_tan.c s_ceil.c s_cos.c \
           s_floor.c s_scalbn.c s_copysign.c        \
           s_sin.c s_tan.c e_rem_pio2.c w_sqrt.c    \
           e_sqrt.c s_fabs.c

SRCFILES  = cache.c class.c fields.c frame.c garbage.c $(COLLECTOR)   \
            global.c interpret.c execute.c loader.c main.c native.c   \
            property.c thread.c nativeCore.c hashtable.c async.c      \
            verifier.c log.c jar.c inflate.c  stackmap.c profiling.c  \
            pool.c events.c resource.c StartJVM.c verifierUtil.c      \
            nativeFunctionTableWin.c runtime_md.c runtime2_md.c       \
            kvmutil.c loaderFile.c wince_io.c

ifeq ($(DEBUG), true)
   SRCFILES += debugger.c debuggerSocketIO.c debuggerOutputStream.c debuggerInputStream.c
   OTHER_FLAGS += -DENABLE_JAVA_DEBUGGER=1 -DDEBUG
else
   OTHER_FLAGS += -DENABLE_JAVA_DEBUGGER=0 -DNDEBUG
endif

OTHER_FLAGS +=-DPADTABLE=1

ifneq ($(USE_JAM), false)
   OTHER_FLAGS += -DUSE_JAM=1
   SRCFILES += socketProtocol.c socketProtocol_md.c
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
   SRCFILES += ROMjavaWin.c
endif

all: kvm$(j)$(g).exe

OBJFILES = $(patsubst %.c,obj$(j)$(g)/%.o,$(SRCFILES))

RCFILES=obj$(j)$(g)/kvmutil.res

FP_OBJFILES = $(patsubst %.c,fp_obj$(j)$(g)/%.o,$(FP_FILES))

vpath %.c  $(TOP)/kvm/VmCommon/src/ $(TOP)/kvm/VmExtra/src/fp            \
		   $(TOP)/kvm/VmWinCE/src/ $(TOP)/kvm/VmExtra/src/       \
		   $(TOP)/kvm/VmWin/src/ $(TOP)/tools/jcc/
vpath %.rc $(TOP)/kvm/VmWinCE/src/

LIBS   = commctrl.lib coredll.lib aygshell.lib winsock.lib
CFLAGS = -DARM -D_ARM_ -DUNDER_CE=$(CEVersion) -D_WIN32_WCE=$(CEVersion) \
         -D"$(CePlatform)" /M$(CECrtMT)                                  \
         -I$(TOP)/kvm/VmCommon/h -I$(TOP)/kvm/VmWinCE/h \
         -I$(TOP)/kvm/VmWin/h -I$(TOP)/kvm/VmExtra/h

CFLAGS+=-DALTERNATIVE_FAST_INTERPRETER=0
CFLAGS+=-nologo -DUNICODE -D_UNICODE -DPOCKETPC $(ROMFLAGS) $(OTHER_FLAGS)


RCFLAGS= /l 0x409 /r -DARM -D_ARM_ -DUNDER_CE=$(CEVersion)        \
         -D_WIN32_WCE=$(CEVersion) -D"$(CePlatform)" /M$(CECrtMT) \
         -I$(TOP)/kvm/VmWinCE/src/


DEBUG_FLAG= -Zi -Od
OPTIMIZE_FLAG= -Ox
FP_OPTIMIZE_FLAG= 
OUTPUT = -Fo
LINKER_OUTPUT = -out:
LDFLAGS =  /nologo /base:"0x00010000" /stack:0x10000,0x1000    \
           /entry:"WinMainCRTStartup" /incremental:no          \
           /nodefaultlib:"$(CENoDefaultLib)"                   \
           /subsystem:$(CESubsystem) /align:"4096" /MACHINE:ARM 

ifeq ($(DEBUG), true)
   LDFLAGS += /debug
endif


DEBUG_FLAG += -DINCLUDEDEBUGCODE=1 

$(TOP)/tools/jcc/ROMjavaWin.c $(TOP)/tools/jcc/nativeFunctionTableWin.c: jccWin

.PHONY:  jccWin
jccWin: 
	@(cd $(TOP)/tools/jcc; $(MAKE) windows)

kvm$(j)$(g).exe: obj$j$g/  fp_obj$j$g/ $(OBJFILES) $(FP_OBJFILES) $(RCFILES)
	@echo "Linking ... $@"
	@$(LD) $(OBJFILES) $(FP_OBJFILES) $(RCFILES) $(LINKER_OUTPUT)$@ \
	       $(LIBS) $(LDFLAGS)

clean: 
	rm -rf kvm* obj* fp_obj*

obj$(j)$g/execute.o : execute.c bytecodes.c 

obj$(j)/%.o: %.c
		@echo "... $@"
		@$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -c $(OUTPUT)$@ $<

obj$(j)_g/%.o: %.c
		@echo "... $@"
		@$(CC) $(CFLAGS) $(DEBUG_FLAG) -c $(OUTPUT)$@ $<

obj$(j)$(g)/kvmutil.res: kvmutil.rc
		@echo "... $@"
		@$(RSC) $(RCFLAGS) $(OUTPUT)$@ $<

obj$j$g/:
		mkdir -p $@

fp_obj$(j)/%.o: %.c
		@echo "... $@"
		@$(CC) $(CFLAGS) $(FP_OPTIMIZE_FLAG) -c $(OUTPUT)$@ $<

fp_obj$(j)_g/%.o: %.c
		@echo "... $@"
		@$(CC) $(CFLAGS) $(DEBUG_FLAG) -c $(OUTPUT)$@ $<

fp_obj$j$g/:
		mkdir -p $@

.FORCE:


