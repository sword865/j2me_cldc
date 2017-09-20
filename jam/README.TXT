JAVA APPLICATION MANAGER (JAM)

This directory contains the source code
of the Java Application Manager (JAM)
application that can be linked in with
the KVM.  The JAM provides support for
dynamic downloading of Java applications 
from the Internet to the KVM, and it also 
provides rudimentary facilities for the 
management and local storage of Java 
applications once they have been downloaded.

The implementation provided in this directory 
is known as the "generic JAM" that is used 
primarily for automated testing of the KVM 
and for running the CLDC compatibility 
toolkit (CLDC TCK).

JAM implementations for a particular device 
or embedded platform are typically quite 
different from this generic implementation.
For instance, device-specific JAMs are often 
integrated with a microbrowser to support
flexible downloading of Java applications
from the Web, they utilize hardware-specific 
storage mechanisms for storing the downloaded 
applications, and they may have a graphical 
user interface for managing (inspecting, 
launching, deleting, etc.) the Java 
applications that have been stored
in the device.

