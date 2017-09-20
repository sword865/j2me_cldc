VMCOMMON DIRECTORY

This directory contains the source code of the 
KVM core.

When porting the KVM to a new platform, you 
should rarely have to edit any of the files
in this directory.  Rather, all the port-
specific code should be placed in a directory
called VM<Port>, where <Port> is the name of
your target platform, e.g., VmWin, VmUnix, etc.

File "VmCommon/h/main.h" defines the default values 
of all the major compilation and customization 
options of the KVM.  When starting a port, you 
should avoid changing the values directly in 
"VmCommon/h/main.h".  Rather, you should override
the values in your platform-specific "machine_md.h"
file.  Files "VmWin/h/machine_md.h" or 
"VmUnix/h/machine_md.h" can be used as a starting
point for porting efforts.

Refer to KVM Porting Guide for further information.

