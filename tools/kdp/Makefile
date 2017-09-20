# 
# Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
# SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
# 
#

all: tools

JAVA          = java
JAVAC         = javac
JAR           = jar
JAVAFILES =  $(shell find src -name "*.java"|grep -v SCCS)
 
CLASSFILES = $(subst src,classes,$(JAVAFILES:java=class))

ifeq ($(DEBUG),true)
DEBUGFLAG=
else
DEBUGFLAG=":none"
endif

# $< is dependency
# $@ is target
#
$(CLASSFILES): classes/%.class : src/%.java
	@echo $< >> .filelist

eraselists:
	@rm -f .filelist 

compilefiles:
	@if [ '!' -d classes ]; then rm -rf classes; mkdir classes; fi;
	@if [ -f .filelist ]; then \
		echo $(JAVAC) `cat .filelist`; \
		$(JAVAC) -g$(DEBUGFLAG) -d classes -classpath classes \
	              `cat .filelist`; \
		fi

tools: eraselists $(CLASSFILES) compilefiles

classes.zip: tools
	@rm -rf classes.zip
	@$(JAR) cfM0 classes.zip -C classes .

clean:
	rm -rf *.zip
	rm -rf .filelist
	rm -rf classes
	rm -rf *~ */*~ */*/*~
	rm -rf *# */*# */*/*#

