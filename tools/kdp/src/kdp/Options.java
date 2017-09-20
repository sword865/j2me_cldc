/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package kdp;

import kdp.classparser.*;
import java.net.*;
import java.io.*;

class Options {

    int localport,
        remoteport;
    String remotehost;

    String classpath;
    int verbosity;
    boolean useClassParser;

    public void setLocalPort( int port ) { localport = port; }
    public int  getLocalPort() { return localport; }

    public void setRemoteHost( String host ) { remotehost = host; }
    public String getRemoteHost() { return remotehost; }

    public void setRemotePort( int port ) { remoteport = port; }
    public int  getRemotePort() { return remoteport; }

    public void setClassPath( String path ) { classpath = path; }
    public String getClassPath() { return classpath; }

    public void setVerbosity( int lvl ) { verbosity = lvl; }
    public int getVerbosity() { return verbosity; }

    public void setUseClassParser( boolean on ) { useClassParser = on; }
    public boolean getUseClassParser() { return useClassParser; };
} // Options
