JAVACODECOMPACT FOR KVM

This directory contains the source code
of the JavaCodeCompact (JCC) tool that
can be used for prelinking/preloading
Java classes into the K Virtual Machine.
The prelinking/preloading process is often
referred to informally as "romizing",
even though the generated binary images
of Java classes do not necessarily
have to be stored in ROM or any other
specific type memory.

The JavaCodeCompact tool is most commonly
used for processing system classes (java.*,
javax.*, ...) so that those classes can be
linked statically into the virtual machine,
and the runtime costs associated with dynamic 
loading of those classes can be avoided.

The JavaCodeCompact tool was originally
written for the Embedded Java(tm) platform.
A modified version for KVM is provided in
this directory.

