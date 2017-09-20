# 
# Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
# 
#

TOP=..
include $(TOP)/build/Makefile.inc

ifeq ($(DEBUG),true)
DEBUGFLAG=
else
DEBUGFLAG=":none"
endif

JAVAC     = javac

ifneq ($(findstring win, $(PLATFORM)),)
PREVERIFY = ../tools/preverifier/build/$(PLATFORM)/preverify.exe
else
PREVERIFY = ../tools/preverifier/build/$(PLATFORM)/preverify
endif

JAR = jar

all: tools



JAVAFILES =  $(shell find src -name "*.java"|grep -v SCCS|grep -v j2se)

CLASSFILES = $(subst src,classes,$(JAVAFILES:java=class))

# $< is dependency
# $@ is target
#

ifeq ("$(USE_JAM)","true")
  JAM_JAVAFILES  = $(shell find src_jam -name "*.java"|grep -v SCCS)
  JAM_CLASSFILES = $(subst src_jam,classes,$(JAM_JAVAFILES:java=class))
  $(JAM_CLASSFILES): classes/%.class : src_jam/%.java
	@echo $< >> .filelist
else
  JAM_CLASSFILES:

endif

$(CLASSFILES):: classes/%.class : src/%.java
	@echo $< >> .filelist
	
	
eraselists:
	@rm -f .filelist

compilefiles:$(PREVERIFY)
	@if [ '!' -d tmpclasses ]; then rm -rf tmpclasses; mkdir tmpclasses; fi;
	@if [ -f .filelist ]; then \
		echo $(JAVAC) -g$(DEBUGFLAG) -d tmpclasses -bootclasspath classes \
		      -classpath classes `cat .filelist`; \
		$(JAVAC) -g$(DEBUGFLAG) -d tmpclasses -bootclasspath classes \
		      -classpath classes `cat .filelist` || exit 1; \
		echo $(PREVERIFY) -d classes tmpclasses; \
		$(PREVERIFY) -d classes tmpclasses || exit 1; \
		fi

$(PREVERIFY):
	if [ '!' -f $@ ];then;         \
	    $(error "Please build $@); \
	fi

tools: eraselists $(CLASSFILES) $(JAM_CLASSFILES) compilefiles

classes.zip: tools
	@rm -rf classes.zip
	@$(JAR) cfM0 classes.zip -C classes .

clean:
	rm -rf *.zip
	rm -rf .filelist
	rm -rf classes
	rm -rf tmpclasses
	rm -rf *~ */*~ */*/*~
	rm -rf *# */*# */*/*#

