# 
# Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
# 
#

TOP=../..
include $(TOP)/build/Makefile.inc

JAVA          = java
JAVAC         = javac
JAR           = jar

JAVAFILES =  $(shell find src -name "*.java"|grep -v SCCS)
 
CLASSFILES = $(subst src,classes,$(JAVAFILES:java=class))

# $< is dependency
# $@ is target


$(CLASSFILES): classes/%.class : src/%.java
	@echo $< >> .filelist

eraselists: 
	@rm -f .filelist 

compilefiles:
	@if [ '!' -d classes ]; then rm -rf classes; mkdir classes; fi;
	@if [ -f .filelist ]; then \
		echo $(JAVAC) `cat .filelist`; \
		$(JAVAC) -d classes -classpath classes:src \
	              `cat .filelist`; \
	fi

ifeq ($(PLATFORM), solaris)
all:    unix
endif

ifeq ($(PLATFORM), linux)
all:    unix
endif

ifeq ($(PLATFORM), win32)
all:    windows
endif


unix:	 nativeFunctionTableUnix.c ROMjavaUnix.c

windows: nativeFunctionTableWin.c ROMjavaWin.c

tools: eraselists $(CLASSFILES) compilefiles 

ifneq ($(JCC_PASS_TWO), true)

ROMjava%.c: classes%.zip tools
	@cp -f src/*.properties classes
	@$(MAKE) $@ JCC_PASS_TWO=true

nativeFunctionTable%.c: classes%.zip tools
	@cp -f src/*.properties classes
	@$(MAKE) $@ JCC_PASS_TWO=true

nativeRelocation%.c: classes%.zip tools
	@cp -f src/*.properties classes
	@$(MAKE) $@ JCC_PASS_TWO=true

#Classes that aren't currently used for a particular platform
NON_Unix_CLASSES = ''

NON_Win_CLASSES  = ''

classes%.zip: classes.zip
	@echo ... $@
	@cp classes.zip $@
	
classes.zip: $(TOP)/api/classes.zip
	@echo ... $@
	@cmp -s $< $@ || (echo cp $< $@ ; cp $< $@)


$(TOP)/api/classes.zip: .FORCE
	@(cd $(TOP)/api; $(MAKE) classes.zip)

else

# One of the possible architectures when creating ROMjava*.c
UnixArch = KVM
WinArch =  KVM

UnixFlags = 
WinFlags =  



ROMjava%.c: classes%.zip 
	@echo ... $@
	$(JAVA) -classpath classes JavaCodeCompact \
 	          $($(patsubst classes%.zip,%Flags,$<)) \
	         -arch $($(patsubst classes%.zip,%Arch,$<)) -o $@ $<

nativeFunctionTable%.c: classes%.zip
	@echo ... $@
	@cp -f src/*.properties classes
	$(JAVA) -classpath classes JavaCodeCompact \
 	          $($(patsubst classes%.zip,%Flags,$<)) \
		 -arch KVM_Native -o $@ $<

nativeRelocation%.c: classes%.zip
	@echo ... $@
	$(JAVA) -classpath classes JavaCodeCompact \
	         -imageAttribute relocating \
 	          $($(patsubst classes%.zip,%Flags,$<)) \
	         -arch $($(patsubst classes%.zip,%Arch,$<)) -o $@ $<

endif


clean:
	@rm -rf .filelist
	@rm -rf classes tmpjar
	@rm -rf *.zip
	@rm -rf ROMjava*
	@rm -rf nativeFunctionTable*
	@rm -rf nativeRelocation*
	@rm -rf *~ */*~ */*/*~
	@rm -rf *# */*# */*/*#


# I have no idea why this is necessary, but otherwise gnumake deletes
# these as intermediate files
.SECONDARY:	classesUnix.zip classesWin.zip

.FORCE:
