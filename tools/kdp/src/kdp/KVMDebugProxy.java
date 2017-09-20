/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package kdp;

import kdp.classparser.*;
import java.net.*;
import java.io.*;

public class KVMDebugProxy {

    Options options = null;

    public KVMDebugProxy() {
    }

    public boolean parseArgs( String args[] ) {
        int i = 0;

        options = new Options();

        options.setLocalPort(0);
        options.setRemotePort(2800);
        options.setRemoteHost("localhost");
        options.setVerbosity(0);
        options.setClassPath("./");
        options.setUseClassParser(false);

        if ( args.length == 0 ) 
            options = null;

        try {
            while ( i < args.length ) {
                if ( "-l".equals( args[ i ] ) ) {
                    options.setLocalPort( Integer.parseInt( args[ ++i ] ) ); 
                } else if ( "-r".equals( args[ i ] ) ) {
                    options.setRemoteHost( args[ ++i ] );  
                    options.setRemotePort( Integer.parseInt( args[ ++i ] ) );  
                } else if ( "-v".equals( args[ i ] ) ) {
                    options.setVerbosity( Integer.parseInt( args[ ++i ] ) );  
                } else if ( "-cp".equals( args[ i ] ) ||
                            "-classpath".equals( args[ i ] ) ) {
                    options.setClassPath( args[ ++i ] );
                } else if ( "-p".equals( args[ i ] ) ) {
                    options.setUseClassParser( true );
                }
    
                i++;
            }
        } catch ( Exception e ) {
            options = null;
        }

        return options != null;
    }

    public void help() { 
        System.out.println( "J2ME Debug Agent Copyright (c) 2000 Sun Microsystems, Inc. All rights reserved." );
        System.out.println();
        System.out.println( "Usage: KVMDebugProxy -l <localport> -r <remotehost> <remoteport> [-p]");
    System.out.println("        [-v <level>] [-cp | - classpath <classpath" + File.pathSeparator + "classpath...>]" );
    System.out.println("Where:");
    System.out.println("  -l <localport> specifies the local port number that the debug agent will");
    System.out.println("     listen on for a connection from a debugger.");
    System.out.println("  -r <remotehost> <remoteport> is the hostname and port number that the debug");
    System.out.println("     agent will use to connect to the Java VM running the application");
    System.out.println("     being debugged");
    System.out.println("  -p tells the debug agent to process classfiles on behalf of the Java VM ");
    System.out.println("     running the application being debugged.  If not present, all commands");
    System.out.println("     from the debugger are passed down to the Java VM running the application");
    System.out.println("  -v <level> turns on verbose mode.  'level' specifies the amount of output,");
    System.out.println("     the larger the number the more output that is generated.");
    System.out.println("     'level' can be from 1-9.");
    System.out.println("  -cp or -classpath specifies a list of paths separated by " + File.pathSeparator + " where the");
    System.out.println("     debug agent can find copies of the class files.  Only needed if -p is set.");
        System.out.println();
    }

    public int go() {

         if ( options == null ) 
             return( 1 );

    Log.SET_LOG(options.getVerbosity());

         try { 

//             System.out.print( "Building class information..." );
             ClassManager manager = new ClassManager(
                                    new SearchPath( options.getClassPath() ) );

//             manager.build( new SearchPath( options.getClassPath() ) );

             System.out.println( "Connecting to " + options.getRemoteHost() +
                                 " on port "      + options.getRemotePort() );

             /* The JDI code will now be trying to send handshaking information. 
              * We must just pass this string over to the kvm. The kvm will then
              * send the same string back, which we must then pass to the 
              * debugger
              */
//             String handshake = "JDWP-Handshake";

             // debugger -> kvm
 //            for ( int i=0; i < handshake.length(); i++ )
  //               kvm.sendByte( ( byte )debugger.receiveByte() );

             // debugger <- kvm
   //          for ( int i=0; i < handshake.length(); i++ )
    //             debugger.sendByte( ( byte )kvm.receiveByte() );

             //
             // Two threads are required here so that we can listen to 
             // the debugger and the kvm at the same time. Each thread
             // knows how to talk to the other so that information can flow
             // over the proxy.
             //

             DebuggerListener dlisten = new DebuggerListener(options);
             KVMListener klisten = new KVMListener(options);

             dlisten.set(klisten, manager );

             klisten.set(dlisten, manager );

             dlisten.verbose( options.getVerbosity() );
             klisten.verbose( options.getVerbosity() );

             //
             // At this point we have successfully connected the debugger 
             // through this proxy to the kvm. we can now sit back and
             // wait for packets to start flowing.
             //
             new Thread( dlisten ).start();
             new Thread( klisten ).start();

         } catch ( SecurityException e ) {
             System.out.println( "KVMDebugProxy: " + e.getMessage() );
         }
    
         return( 0 );
    }

    public static void main( String args[] ) {

        KVMDebugProxy kdp = new KVMDebugProxy();

        if ( !kdp.parseArgs( args ) ) {
            kdp.help();
            System.exit( 1 ); 
        }

        kdp.go();
    }

} // KVMDebugProxy
