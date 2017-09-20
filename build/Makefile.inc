# The PLATFORM defined in build/platform-name

UNAME=$(shell uname -a)

ARCH=i386
ifneq (,$(findstring sparc,$(UNAME)))
  export ARCH=sparc
endif

OTHER_FLAGS += -DBUILD_VERSION='"$(VERSION)"'
