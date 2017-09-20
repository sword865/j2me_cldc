# 
# Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
# 
#
#
# Makefile for GNU make, adapted from the solaris version
# @(#)GNUmakefile	1.5 99/11/22
#

TOP=../../..
include $(TOP)/build/Makefile.inc

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

ifeq ($(ASM_LOOP), true)
OTHER_FLAGS += -DALTERNATIVE_FAST_INTERPRETER=1
endif

SRCFILES  = cache.c class.c fields.c frame.c garbage.c $(COLLECTOR)   \
            global.c interpret.c execute.c loader.c main.c native.c   \
            property.c thread.c nativeCore.c loaderFile.c hashtable.c \
            verifier.c log.c jar.c inflate.c  stackmap.c profiling.c  \
            pool.c events.c resource.c StartJVM.c                     \
            nativeFunctionTableWin.c runtime_md.c runtime2_md.c       \
            async.c verifierUtil.c 

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
   SRCFILES += ROMjavaWin.c
endif

all: kvm$(j)$(g).exe

OBJFILES = $(patsubst %.c,obj$(j)$(g)/%.o,$(SRCFILES))

FP_OBJFILES = $(patsubst %.c,fp_obj$(j)$(g)/%.o,$(FP_FILES))

ifeq ($(ASM_LOOP), true)
    OBJFILES += obj$(j)$(g)/winFastInterpret.o
    SRCFILES += winFastInterpretSupport.c
endif

obj$(j)$(g)/winFastInterpret.o: $(TOP)/kvm/vmWin/src/winFastInterpret.asm $(TOP)/kvm/vmWin/src/defines.asm
	@ml /nologo /Zi /c /coff /Cx /Fo$@ \
		$(TOP)/kvm/vmWin/src/winFastInterpret.asm
	
vpath %.c  $(TOP)/kvm/VmCommon/src/ $(TOP)/kvm/VmExtra/src/fp \
		   $(TOP)/kvm/VmWin/src/ $(TOP)/kvm/VmExtra/src/      \
		   $(TOP)/tools/jcc/ $(TOP)/jam/src/

ifeq ($(GCC), true)
   CC = gcc
   LD = gcc
   LIBS= -luser32 -lgdi32 -lkernel32 -lmsvcrt -lwsock32
   CPPFLAGS = -DWIN32 -DGCC -I$(TOP)/kvm/VmCommon/h -I$(TOP)/kvm/VmWin/h \
	   -I$(TOP)/kvm/VmExtra/h -I$(TOP)/jam/h -I$(TOP)/kvm/VmCommon/src
   CFLAGS = -mno-cygwin -mcpu=i386 -Wall $(CPPFLAGS) $(ROMFLAGS) $(OTHER_FLAGS)
   DEBUG_FLAG = -g
   OPTIMIZE_FLAG = -O2
   FP_OPTIMIZE_FLAG =
   OUTPUT = -o 
   LINKER_OUTPUT = -o
   LDFLAGS = -mno-cygwin
else
   CC= cl
   LD= link
   LIBS= user32.lib gdi32.lib kernel32.lib wsock32.lib
   CPPFLAGS = -DWIN32 -I$(TOP)/kvm/VmCommon/h -I$(TOP)/kvm/VmWin/h \
	   -I$(TOP)/kvm/VmExtra/h -I$(TOP)/jam/h /MT
   CFLAGS= -nologo $(CPPFLAGS) $(ROMFLAGS) $(OTHER_FLAGS)
   DEBUG_FLAG= -Zi -Od
   OPTIMIZE_FLAG= -Ox
   FP_OPTIMIZE_FLAG= 
   OUTPUT = -Fo
   LINKER_OUTPUT = -out:
   LDFLAGS = 
ifeq ($(DEBUG), true)
   LDFLAGS += -debug
endif

endif

$(TOP)/kvm/vmWin/src/defines.asm: makeAsmOffsets
	@echo "generating assembly defines..."
	@rm -f $@
	@./makeAsmOffsets > $@

makeAsmOffsets: $(TOP)/kvm/VmWin/src/makeAsmOffsets.c
	@$(CC) $(CFLAGS) $(OUTPUT)$@ $<

DEBUG_FLAG += -DINCLUDEDEBUGCODE=1 

$(TOP)/tools/jcc/ROMjavaWin.c $(TOP)/tools/jcc/nativeFunctionTableWin.c: jccWin

.PHONY:  jccWin
jccWin: 
	@(cd $(TOP)/tools/jcc; $(MAKE) windows)

kvm$(j)$(g).exe: obj$j$g/  fp_obj$j$g/ $(OBJFILES) $(FP_OBJFILES)
	@echo "Linking ... $@"
	@$(LD) $(LDFLAGS) $(OBJFILES) $(FP_OBJFILES) $(LINKER_OUTPUT)$@ $(LIBS)

clean: 
	rm -rf kvm* obj* fp_obj* makeAsmOffsets.* $(TOP)/kvm/vmWin/src/defines.asm

$(OBJFILES): $(TOP)/kvm/VmWin/h/machine_md.h

obj$(j)$g/execute.o : execute.c bytecodes.c 

obj$(j)/%.o: %.c
		@echo "... $@"
		@$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -c $(OUTPUT)$@ $<

obj$(j)_g/%.o: %.c
		@echo "... $@"
		@$(CC) $(CFLAGS) $(DEBUG_FLAG) -c $(OUTPUT)$@ $<

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

ifeq ($(GCC), true)

%.i: %.c .FORCE
	echo $(CC)
	@$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -E $(OUTPUT)$@ $<

else 

%.i: %.c .FORCE
	echo $(CC)
	@$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -E $< > $@

endif

%.s: %.c .FORCE
	@echo "... $@"
	@$(CC) $(CFLAGS) $(OPTIMIZE_FLAG) -S $(OUTPUT)$@ $<

.FORCE:


