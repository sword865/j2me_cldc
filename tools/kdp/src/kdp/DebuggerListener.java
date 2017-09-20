/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package kdp;

import kdp.classparser.*;
import kdp.classparser.attributes.*;
import java.util.*;
import java.net.*;
import java.io.*;

class DebuggerListener extends ProxyListener implements VMConstants {

    static final int UseClassParser = 0x01;

    SocketConnection connDebugger;
    ProxyListener KVMListener;
    ClassManager manager = null;
    Packet replyPacket;
    Options options;
    boolean Ready = false;
    ServerSocket serverSocket = null;
    Socket acceptSocket = null;

    public DebuggerListener(Options opt) {
        super();
        this.options = opt;
    }

                     
    public void set(ProxyListener KVMListener, ClassManager manager) {
        this.KVMListener = KVMListener;
        this.manager = manager;
    }

        /*
         * Sends packet to our output socket
         */
        public synchronized void send(Packet p) throws IOException{
                synchronized(this) {
                        while (!Ready) {
                                try {
                                        this.wait();
                                } catch (InterruptedException e) {
                                }
                        }
                }
                String id = String.valueOf(p.id);
                synchronized(waitingQueue) {
                        if ((p.flags & Packet.Reply) == 0 && p.id < 0)
                                waitingQueue.put(id, p);
                        }
                connDebugger.send(p);
        }

 
    public void quit() {
        boolean oldtimeToQuit = timeToQuit;
        timeToQuit = true;
        if (oldtimeToQuit != timeToQuit) {
            KVMListener.quit();
            try {
                if (acceptSocket != null) {
//                      acceptSocket.shutdownInput();
//                      acceptSocket.shutdownOutput();
                        acceptSocket.close();
                }
                if (serverSocket != null)
                        serverSocket.close();
            } catch (IOException e) {}
        }
    }

    public void run() {
        boolean handled;
        Packet p=null;
        String classname=null, methodname=null;
        PacketStream ps=null;
        PacketStream in=null;

        try {
            System.out.println("Waiting for debugger on port " +
                 options.getLocalPort());
            serverSocket = new ServerSocket(options.getLocalPort());
            acceptSocket = serverSocket.accept();
            connDebugger = new SocketConnection(this,  acceptSocket);
            System.out.println("Connection received.");
        } catch (IOException e) {
                System.out.println("DebuggerListener: " + e.getMessage());
                Runtime.getRuntime().exit(1);
        } catch (SecurityException e) {
                System.out.println("DebuggerListener: " + e.getMessage());
                Runtime.getRuntime().exit(1);
        }
        byte [] handshake = new String("JDWP-Handshake").getBytes();

        // debugger -> kvm
        try {
                for (int i=0; i < handshake.length; i++)
                        connDebugger.receiveByte();

                // debugger <- kvm
                for (int i=0; i < handshake.length; i++)
                        connDebugger.sendByte(handshake[i]);
        } catch (IOException e) {
        }
        synchronized(this) {
                Ready = true;
                this.notify();
        }

        new Thread(connDebugger).start();

        try {
        while (!timeToQuit) {

            ClassFile cf;
            int cid;
 
            handled = false;
            p = waitForPacket();        /* wait for a packet */
            if (p == null)
                break;                  // must be time to quit

            if ((p.flags & Packet.Reply) == 0 &&
                 (options.getUseClassParser()) == true) { // must want to 
            in = new PacketStream(this, p);
            switch (p.cmdSet) {
            case VIRTUALMACHINE_CMDSET:

                switch(p.cmd) {
                case SENDVERSION_CMD:
                    ps = new PacketStream(this,
                        in.id, Packet.Reply, Packet.ReplyNoError);
                    ps.writeString("Version 1.0");
                    ps.writeInt(1);             /* major version */
                    ps.writeInt(0);             /* minor version */
                    ps.writeString("1.0.3");
                    ps.writeString("KVM");
                    ps.send(); 

                    handled = true;
                    break;

                case CLASSESBYSIG_CMD:
                    String classToMatch = in.readString();
                    Log.LOGN(3, "ClassBySig: class " + classToMatch);
                    ps = new PacketStream(this,
                        in.id, Packet.Reply, Packet.ReplyNoError);
                    Iterator iter =
                         ClassManager.classMap.values().iterator();

                    while (iter.hasNext()) {
                        try {
                            cf = (ClassFile)iter.next();
                        if (cf == null)
                            throw new Exception("Couldn't find classFile object");
                        if (classToMatch.compareTo(cf.getClassSignature()) == 0) {
                            Log.LOGN(3, "ClassBySig matched " + cf.getClassName());
                            Log.LOGN(6, "Class Signature: "+ cf.getClassSignature());
                            ps.writeInt(1);
                            ps.writeByte(cf.getJDWPTypeTag());
                            ps.writeInt(cf.getClassID());
                            ps.writeInt(cf.getClassStatus());
                            ps.send();
                            handled = true;
                            break;
                        }
                        } catch (Exception e) {
                            System.out.println("ClassesBySig command failed with " + e);
                            ps = new PacketStream(this, in.id, Packet.Reply, NOTFOUND_ERROR);
                            ps.send();
                            handled = true;
                        }
                    }
                    if (!handled) {
                        ps = new PacketStream(this, in.id, Packet.Reply, Packet.ReplyNoError);
                        ps.writeInt(0);
                        ps.send();
                        handled = true;
                    }
                    break;

                case ALL_CLASSES_CMD:
                    {
                    String className;
                    int classStatus;
                    byte typeTag;

                    /* we intercept all the statuses and then just pass it on */
                    Log.LOGN(3, "All_Classes command");
                    ps = new PacketStream(KVMListener,
                        VIRTUALMACHINE_CMDSET, ALL_CLASSES_CMD);
                    PacketStream ps2 = new PacketStream(this, in.id,
                        Packet.Reply, Packet.ReplyNoError);
                    ps.send();
                    try {
                        ps.waitForReply();
                    } catch (Exception e) {
                        ps2.writeInt(0);
                        ps2.send();
                        handled = true;
                        break;
                    }
                    int numClasses = ps.readInt();    // get number of classes
                    ps2.writeInt(numClasses);
                    for (int i = 0; i < numClasses; i++) {
                        typeTag = ps.readByte();        // typeTag
                        cid = ps.readInt();             // ID
                        className = ps.readString();
                        classStatus = ps.readInt();
                        // pass the info on up to the debugger
                        ps2.writeByte(typeTag);
                        ps2.writeInt(cid);
                        ps2.writeString(className);
                        ps2.writeInt(classStatus);
                        if (typeTag != TYPE_TAG_ARRAY) {
                            // strip off leading 'L' and trailing ';'
                            className =
                                new String(className.substring(1,
                                className.length() - 1));
                        }
                        Log.LOGN(3, "AllClasses:  " + className + ", ID = "
                            + Integer.toHexString(cid));
                        if ((cf = (ClassFile)ClassManager.classMap.get(new Integer(cid))) == null) {
                            cf = manager.findClass(typeTag, className);
                            if (cf != null) {
                                ClassManager.classMap.put(new Integer(cid), cf);
                                cf.setClassID(cid);
                                cf.setClassStatus(classStatus);
                            } else {
                                System.out.println("ALL_CLASSES_CMD: couldn't find classfile object");
                                ps2 = new PacketStream(this, in.id, Packet.Reply,NOTFOUND_ERROR);
                                ps2.send();
                                handled = true;
                                break;
                            }
                        } else {
                                cf.setClassStatus(classStatus);
                        }
                    }
                    ps2.send();
                    handled = true;
                    }
                    break;

                case TOPLEVELTHREADGROUP_CMD:
                    ps = new PacketStream(this,
                        in.id, Packet.Reply, Packet.ReplyNoError);
                    ps.writeInt(1);
                    ps.writeInt(ONLY_THREADGROUP_ID);
                    ps.send();
                    handled = true;
                    break;

                case DISPOSE_CMD:
                    ps = new PacketStream(KVMListener,
                        VIRTUALMACHINE_CMDSET, EXIT_CMD);
                    ps.writeInt(0);
                    ps.send();
                    PacketStream ps2 = new PacketStream(this, in.id,
                        Packet.Reply, Packet.ReplyNoError);
                    ps2.send();
                    handled = true;
                    quit();
                    break;
                    

                case IDSIZES_CMD:
                    ps = new PacketStream(this,
                        in.id, Packet.Reply, Packet.ReplyNoError);
                    ps.writeInt(8);             // sizeof field ID in KVM
                    ps.writeInt(4);             // sizeof method ID in KVM
                    ps.writeInt(4);             // sizeof object ID in KVM
                    ps.writeInt(4);             // sizeof reference ID in KVM
                    ps.writeInt(4);             // sizeof frame ID in KVM
                    ps.send();
                    handled = true;
                    break;

                case CAPABILITIES_CMD:
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    ps.writeBoolean(false);
                    ps.writeBoolean(false);
                    ps.writeBoolean(true);    // can get bytecodes
                    ps.writeBoolean(false);
                    ps.writeBoolean(false);
                    ps.writeBoolean(false);
                    ps.writeBoolean(false);
                    ps.send();
                    handled = true;
                    break;

                case CLASSPATHS_CMD:
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    ps.writeString("");
                    ps.writeInt(0);             // # of paths in classpath
                    ps.writeInt(0);             // # of paths in bootclasspath
                    ps.send();
                    handled = true;
                    break;

                case DISPOSE_OBJECTS_CMD:
                    Log.LOGN(3, "Dispose Objects: ");
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    ps.send();
                    handled = true;
                    break;
                }
                break;

            case REFERENCE_TYPE_CMDSET:
                switch(p.cmd) {

                case SIGNATURE_CMD:
                    /*
                     * Implementation note:  If you fail to find the ClassFile
                     * object here, it could be because the debugger has not
                     * requested ClassPrepare events.  In that case the classes
                     * loaded in the VM may be out of synch with the classes that
                     * the debug agent thinks are loaded.  One way to potentially
                     * fix this would be to do an AllClasses command to the VM
                     * in the event of failure to refresh our list of classes
                     */
                    cid = in.readInt();
                    Log.LOGN(3, "Signature cmd: class id = " + Integer.toHexString(cid));
                    try {
                       if ((cf = (ClassFile)ClassManager.classMap.get(new Integer(cid))) == null) {
                            throw new Exception("Couldn't get ClassFile object for signature cmd");
                        }
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            Packet.ReplyNoError);
                        if (cid == ONLY_THREADGROUP_ID) {
                            ps.writeString("Lkvm_threadgroup;");
                        } else {
                            ps.writeString(cf.getClassSignature());
                        }
                        ps.send();
                        handled = true;
                        break;
                    } catch (Exception e) {
                        System.out.println("Signature cmd: exception " + e);
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            INVALID_OBJECT);
                        ps.send();
                        handled = true;
                    }
                    break;

                case CLASSLOADER_CMD:
                    in.readInt();       // ignore the reference ID
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    ps.writeInt(0);     // the null classloader object
                    ps.send();
                    handled = true;
                    break;

                case MODIFIERS_CMD:
                    cid = in.readInt();
                    try {
                       if ((cf = (ClassFile)ClassManager.classMap.get(new Integer(cid))) == null) {
                            throw new Exception("Couldn't get ClassFile object for Modifiers cmd");
                        }
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            Packet.ReplyNoError);
                        ps.writeInt(cf.getRawAccessFlags());
                        ps.send();
                        handled = true;
                        break;

                    } catch (Exception e) {
                        System.out.println("Modifiers cmd: exception " + e);
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            INVALID_OBJECT);
                        ps.send();
                        handled = true;
                    }
                    break;

                case FIELDS_CMD:
                    cid = in.readInt();
                    Log.LOGN(3, "field command: cid = " + Integer.toHexString(cid));
                    if ((cf = (ClassFile)ClassManager.classMap.
                        get(new Integer(cid))) == null) {
                        Log.LOGN(3, "field_cmd: cf == null");
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            Packet.ReplyNoError);
                        ps.writeInt(0);
                        ps.send();
                        handled = true;
                        break;
                    }
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    if (processFields(cf, ps, in.id)) {
                        ps.send();
                    }
                    handled = true;
                    break;

                case METHODS_CMD:
                    cid = in.readInt();
                    Log.LOGN(3, "methods command: cid = " + Integer.toHexString(cid));
                    if ((cf = (ClassFile)ClassManager.classMap.
                        get(new Integer(cid))) == null ||
                        cf.getJDWPTypeTag() == TYPE_TAG_ARRAY) {
                        Log.LOGN(3, "methods_cmd: cf == null or cf == arrayclass");
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            Packet.ReplyNoError);
                        ps.writeInt(0);
                        ps.send();
                        handled = true;
                        break;
                    }
                    Log.LOGN(3, "methods for " + cf.getClassFileName());
                    try {
                        MethodInfo mi;
                        int index = method_index_base;
                        List miList = cf.getAllMethodInfo();
                        Iterator iter = miList.iterator();
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            Packet.ReplyNoError);
                        Log.LOGN(5, "Methods: " + miList.size() + " methods");
                        ps.writeInt(miList.size());
                        while (iter.hasNext()) {
                                mi = (MethodInfo)iter.next();
                                if (mi == null) {
                                    throw new Exception("methodinfo null");
                                }
                                Log.LOGN(5, "Method: id = " + Integer.toHexString(index));
                                ps.writeInt(index++);
                                Log.LOGN(5, "Method: name = " + mi.getName());
                                ps.writeString(mi.getName());
                                Log.LOGN(5, "Method: sig = " + mi.getSignatureRaw());
                                ps.writeString(mi.getSignatureRaw());
                                Log.LOGN(5, "Method: flags = " + mi.getAccessFlags());
                                ps.writeInt(mi.getAccessFlags());
                        }
                        ps.send();
                        handled = true;
                        break;
                    } catch (Exception e) {
                        System.out.println("Methods cmd: exception " + e);
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            NOTFOUND_ERROR);
                        ps.send();
                        handled = true;
                    }
                    break;

                case SOURCEFILE_CMD:
                    SourceFileAttribute sfAttr;
                    Log.LOGN(3, "Source file cmd");
                    cid = in.readInt();
                    try {
                       if ((cf = (ClassFile)ClassManager.classMap.get(new Integer(cid))) == null) {
                            throw new Exception("Couldn't get ClassFile object for Modifiers cmd");
                        }
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            Packet.ReplyNoError);
                        if ((sfAttr = cf.getSourceAttribute()) != null) {
                            String s = sfAttr.getSourceFileName();
                            Log.LOGN(3, "Returning from attribute: " + s);
                            ps.writeString(s);
                        } else {
                            Log.LOGN(3, "Creating source name: " + cf.getBaseName() + ".java");
                            ps.writeString(cf.getBaseName() + ".java");
                        }
                        ps.send();
                        handled = true;
                        break;
                    } catch (Exception e) {
                        System.out.println("Sourcefile cmd: exception " + e);
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            INVALID_OBJECT);
                        ps.send();
                        handled = true;
                    }
                    break;

                case INTERFACES_CMD:
                    cid = in.readInt();
                    try {
                       if ((cf = (ClassFile)ClassManager.classMap.get(new Integer(cid))) == null) {
                            throw new Exception("Couldn't get ClassFile object for Interfaces cmd");
                        }
                        List iList = cf.getAllInterfaces();
                        Iterator iIter = iList.iterator();
                        ps = new PacketStream(this, in.id, Packet.Reply,
                            Packet.ReplyNoError);
                        Log.LOGN(3, "Interfaces: class " + cf.getClassName() +
                            " " + iList.size() + " interfaces");
                        ps.writeInt(iList.size());
                        while (iIter.hasNext()) {
                            String className = (String)iIter.next();
                            Log.LOGN(3, "interfaces: classname: " + className);
                            if (className == null) {
                                throw new Exception("interface name null");
                            }
                            if ((cf =
                                manager.findClass((byte)'L', className)) ==
                                null) {
                                ps.writeInt(0);
                            } else {
                                ps.writeInt(cf.getClassID());
                            }
                        }    
                        ps.send();
                        handled = true;
                        break;
                    } catch (Exception e) {
                        System.out.println("Interfaces cmd: exception " + e);
                        ps = new PacketStream(this, in.id, Packet.Reply,
                                              INVALID_OBJECT);
                        ps.send();
                        handled = true;
                    }
                    break;
                }
                
                break;

            case METHOD_CMDSET:

                    /* Method */
                switch(p.cmd) {

                case METHOD_LINETABLE_CMD:  // LineTable
                    handleLineTable(in);
                    handled = true;
                    break;

                case METHOD_VARIABLETABLE_CMD:  // VariableTable
                        handleVariableTable(in);
                        handled = true;
                        break;

                case METHOD_BYTECODES_CMD:   // Bytecodes
                        handleByteCode(in);
                        handled = true;
                        break;
                }
                break;

            case THREADREFERENCE_CMDSET:

                switch(p.cmd) {

                case THREADGROUP_CMD:
                    Log.LOGN(3, "Threadreference: threadgroup");
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    ps.writeInt(ONLY_THREADGROUP_ID);
                    ps.send();
                    handled = true;
                    break;
                }
                break;

            case THREADGROUPREFERENCE_CMDSET:

                switch(p.cmd) {

                case THREADGROUP_NAME_CMD:
                    in.readInt();       // threadgroup ID
                    Log.LOGN(3, "ThreadGroup: name");
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    ps.writeString(KVM_THREADGROUP_NAME);
                    ps.send();
                    handled = true;
                    break;

                case THREADGROUP_PARENT_CMD:
                    int tgID = in.readInt();    // threadgroup ID
                    Log.LOGN(3, "ThreadGroup: parent");
                    ps = new PacketStream(this, in.id, Packet.Reply,
                        Packet.ReplyNoError);
                    ps.writeInt(0);             // we're the top level
                    ps.send();
                    handled = true;
                    break;

                case THREADGROUP_CHILDREN_CMD:
                    tgID = in.readInt();
                    Log.LOGN(3, "ThreadGroup: children");
                    PacketStream ps2 = new PacketStream(this, in.id,
                        Packet.Reply, Packet.ReplyNoError);
                    if (tgID == ONLY_THREADGROUP_ID) {
                        ps = new PacketStream(KVMListener,
                            VIRTUALMACHINE_CMDSET, ALL_THREADS_CMD);
                        ps.send();
                        try {
                            ps.waitForReply();
                        } catch (Exception e) {
                                ps2.writeInt(0);
                                ps2.writeInt(0);
                                ps2.send();
                                handled = true;
                                break;
                        }
                        int numThreads = ps.readInt();
                        Log.LOGN(3, "threadgroup: " + numThreads + " children");
                        ps2.writeInt(numThreads);
                        while (numThreads-- > 0) {
                                ps2.writeInt(ps.readInt());
                        }
                    } else {
                        ps2.writeInt(0);        // number of child threads;
                    }
                    ps2.writeInt(0);    // number of child threadgroups
                    ps2.send();
                    handled = true;
                    break;
                }
                break;

                case STACKFRAME_CMDSET:

                switch(p.cmd) {

                case STACKFRAME_THISOBJECT_CMD:
                        Log.LOGN(3, "Stackframe: thisobject");
                        int tID = in.readInt();         // get thread id;
                        int fID = in.readInt();         // get frame id;
                        ps = new PacketStream(KVMListener,
                            STACKFRAME_CMDSET, STACKFRAME_GETVALUES_CMD);
                        PacketStream ps2 = new PacketStream(this, in.id,
                            Packet.Reply, Packet.ReplyNoError);
                        ps.writeInt(tID);
                        ps.writeInt(fID);
                        ps.writeInt(1);         // just want the 'this' object
                        ps.writeInt(0);         // it's at slot 0
                        ps.writeByte((byte)'L');    // it's an object type
                        ps.send();
                        try {
                            ps.waitForReply();
                        } catch (Exception e) {
                                ps2.writeByte((byte)'L');
                                ps2.writeInt(0);
                                ps2.send();
                                handled = true;
                                break;
                        }
                        int num = ps.readInt();         // get number of values
                        byte tag = ps.readByte();       // get tag type
                        int objectID = ps.readInt();    // and get object ID
                        Log.LOGN(3, "Stackframe: thisobject tag: " + tag +
                            " objectID " + objectID);
                        ps2.writeByte(tag);
                        ps2.writeInt(objectID);
                        ps2.send();
                        handled = true;
                        break;
                }
                break;
            case CLASSOBJECTREFERENCE_CMDSET:

                switch (p.cmd) {

                case REFLECTEDTYPE_CMD:
                    int oID = in.readInt(); // get object ID
                    PacketStream ps2 = new PacketStream(this, in.id,
                        Packet.Reply, Packet.ReplyNoError);
                    if ((cf = (ClassFile)ClassManager.classMap.get(
                        new Integer(oID))) == null) {
                            // this can't possibly happen :-)
                        ps2.writeByte(TYPE_TAG_CLASS);
                    } else {
                        ps2.writeByte(cf.getJDWPTypeTag());
                    }
                    ps2.writeInt(oID);
                    ps2.send();
                    handled = true;
                    break;
                }
                break;

            }
            }
               
            if (!handled) {
                Log.LOG(3, "DebuggerListener: ");
                disp(p);
                if (p.cmdSet == 15 && p.cmd == 1) {
                    Log.LOG(3, "EventKind == " + p.data[ 0 ] + "\n");
                }

                KVMListener.send(p);
            }
        }
        } catch (IOException e) {
            PacketStream error = new PacketStream(this, in.id, Packet.Reply, NOTFOUND_ERROR);
            error.send();
        } catch (ArrayIndexOutOfBoundsException e) {
            System.out.println(p.cmdSet + "/" + p.cmd + " caused: " + e);
            PacketStream error = new PacketStream(this, in.id, Packet.Reply, NOTFOUND_ERROR);
            error.send();
        }
        
    }

    public String toString() {
        return (new String("DebuggerListener: "));
    }

    /**
     * Retrieves the name of the class associated
     * with the specified id in a KVM defined manner
     */
    protected String getClassName(byte[] classid) {

        return new String("");
   
    }

    /**
     * Retrieves the name of the method associated
     * with the specified id in a KVM defined manner
     */
    protected String getMethodName(byte[] methodid) {

        return new String("");
    }

    protected boolean processFields(ClassFile cf, PacketStream ps, int id) {

        ClassFile scf;

        Log.LOGN(3, "processFields for " + cf.getClassFileName());
        try {
            FieldInfo fi;
            List fiList = cf.getAllFieldInfo();
            Iterator fiIter = fiList.iterator();
            long fieldID = ((long)cf.getClassID()) << 32;
            Log.LOGN(5, "field class id is " +
                Integer.toHexString(cf.getClassID()) +
                " fieldid is " + Long.toHexString(fieldID));
            ps.writeInt(fiList.size());
            while (fiIter.hasNext()) {
                fi = (FieldInfo)fiIter.next();
                if (fi == null) {
                    throw new Exception("fieldinfo null");
                }
                Log.LOGN(5, "Field: id = " + Long.toHexString(fieldID));
                ps.writeLong(fieldID++);
                Log.LOGN(5, "Field: name = " + fi.getName());
                ps.writeString(fi.getName());
                Log.LOGN(5, "Field: sig = " + fi.getType());
                ps.writeString(fi.getType());
                Log.LOGN(5, "Field: flags = " + fi.getAccessFlags());
                ps.writeInt(fi.getAccessFlags());
            }
        } catch (Exception e) {
            System.out.println("Fields cmd: exception " + e);
            ps = new PacketStream(this, id, Packet.Reply,
                INVALID_OBJECT);
            ps.send();
            return false;
        }
        return true;
    }

    public void handleLineTable(PacketStream in) throws IOException {
        int mid, cid;
        String classname, methodname, methodsig;
        PacketStream ps;
        ClassFile   cf = null;
        MethodInfo  mi;
        int code[][] = null;
        long startingOffset, endingOffset;
        int lineCount;

        // we need to know the class and method to which the id's refer 

        cid = in.readInt(); // class id - we won't need it

        mid = in.readInt() - method_index_base;
        Log.LOGN(4, "linetable: class id= " + Integer.toHexString(cid) + ", method id= " + Integer.toHexString(mid + method_index_base));

        if ((cf = (ClassFile)ClassManager.classMap.
            get(new Integer(cid))) == null) {
            Log.LOGN(3, "Linetable cmd: not found");
            ps = new PacketStream(this, in.id, Packet.Reply, NOTFOUND_ERROR);
            ps.send();
            return;
        }
        Log.LOGN(4, "linetable: class: " + cf.getClassFileName());
        ps = new PacketStream(this, in.id, Packet.Reply, Packet.ReplyNoError);
        mi = cf.getMethodInfoByIndex(mid);
        if (mi == null) {
            Log.LOGN(1, "Couldn't find methodinfo for index " + mid + method_index_base);
            ps = new PacketStream(this, in.id, Packet.Reply, INVALID_METHODID);
            ps.send();
            return;
        }
        CodeAttribute ca = mi.getCodeAttribute();
        if (ca == null) {
            /* I don't think this can happen; no code, return -1 as start and end */
            startingOffset = endingOffset = -1;
        } else {
            startingOffset = 0;
            endingOffset = ca.getCodeLength() - 1;
        }
        code = mi.getBreakableLineNumbers();
        if (code == null) {
            Log.LOGN(1, "No linenumber table found for class "+ cf.getClassName());
            lineCount = 0;
        } else {
            lineCount = code.length;
        }

        ps.writeLong(startingOffset);  // starting offset
        ps.writeLong(endingOffset);

        Log.LOGN(5, "Starting code offset = " + startingOffset + ", Ending code offset = " + endingOffset);
        Log.LOGN(5, "Code Length = " + lineCount);

        ps.writeInt(lineCount);
        for (int i=0; i < lineCount; i++) {
            ps.writeLong(code[ i ][ 1 ]); 
            ps.writeInt(code[ i ][ 0 ]); 
            Log.LOGN(5, "  index=" + code[ i ][ 1 ] + " l#=" + code[ i ][ 0 ]);
        } 
        ps.send();
    }

    public void handleVariableTable(PacketStream in) throws IOException {

        int mid, cid;
        PacketStream ps;
        List table = null;
        MethodInfo mi = null;
        ClassFile cf;

        cid = in.readInt(); // class id - we won't need it
        Log.LOGN(3, "variable: class id = " + Integer.toHexString(cid));

        mid = in.readInt() - method_index_base;
        Log.LOGN(3, "variable: method id = " + Integer.toHexString(mid + method_index_base));
        if ((cf = (ClassFile)ClassManager.classMap.get(new
            Integer(cid))) == null) {
            Log.LOGN(3, "Variabletable cmd: not found");
            ps = new PacketStream(this, in.id, Packet.Reply, NOTFOUND_ERROR);
            ps.send();
            return;
        }

        try {
            table = cf.getVariableTableForMethodIndex(mid);
            mi = cf.getMethodInfoByIndex(mid);
            if (mi == null) {
                throw new Exception("couldn't find method info for class " +
                    cf.getClassFileName());

            }
        } catch (Exception e) {
            System.out.println("class " + cf.getClassName() + " caused " + e);
            ps = new PacketStream(this, in.id, Packet.Reply, INVALID_OBJECT);
            ps.send();
            return;
        }
        ps = new PacketStream(this, in.id, Packet.Reply, Packet.ReplyNoError);

        ps.writeInt(mi.getArgCount());
        if (table != null) {
            ps.writeInt(table.size());
            Iterator iter = table.iterator();
            while (iter.hasNext()) {
                LocalVariable var = (LocalVariable)iter.next();
                ps.writeLong(var.getCodeIndex());
                ps.writeString(var.getName());
                ps.writeString(getJNISignature(var.getType()));
                ps.writeInt(var.getLength());
                ps.writeInt(var.getSlot());
            }
        } else {
            ps.writeInt(0);
        }
        ps.send();
    }

   private void handleByteCode(PacketStream in) {
        int cid, mid;
        PacketStream ps;
        MethodInfo mi = null;
        ClassFile cf;

        Log.LOGN(1, "Method: Bytecodes");
        String sig=null;

        cid = in.readInt(); // class id
        Log.LOGN(3, "class id=" + Integer.toHexString(cid));

        mid = in.readInt() - method_index_base; // method id
        Log.LOGN(3, "method id=" + Integer.toHexString(mid + method_index_base));

        //
        // now we will build a packet to send 
        // to the debugger
        //

        if ((cf = (ClassFile)ClassManager.classMap.get(new
            Integer(cid))) == null) {
                Log.LOGN(3, "Bytecode cmd: not found");
                ps = new PacketStream(this, in.id,
                    Packet.Reply, INVALID_OBJECT);
                ps.send();
                return;
        }
        try {
            mi = cf.getMethodInfoByIndex(mid);
            if (mi == null) {
                throw new Exception("couldn't find method info for class " +
                    cf.getClassFileName());
            }
        } catch (Exception e) {
            System.out.println(" class" + cf.getClassFileName() + " caused " +
            e);
            ps = new PacketStream(this, in.id, Packet.Reply, INVALID_METHODID);
            ps.send();
            return;
        }

        // grab the byte codes
        //
        byte[] code = mi.getCodeAttribute().getByteCodes();

        //
        // build the reply packet and send it off
        //
        ps = new PacketStream(this,
                in.id, Packet.Reply, Packet.ReplyNoError);
        ps.writeInt((int)code.length);
        ps.writeByteArray(code);
        ps.send(); 
    }

    private String getJNISignature(String type) {
        int index = type.length();
        int preindex = index;
        String ret = new String();
        String str;

        Log.LOGN(6, "getJNISignature()  type == " + type);
        while ((index = type.lastIndexOf("[]", index)) != -1) {
            ret += "[";
            preindex = index;
            index--;
        }

        str = type.substring(0, preindex);

        if      ("int".equalsIgnoreCase(str))     ret += "I";
        else if ("boolean".equalsIgnoreCase(str)) ret += "Z";
        else if ("short".equalsIgnoreCase(str))   ret += "S";
        else if ("byte".equalsIgnoreCase(str))    ret += "B";
        else if ("char".equalsIgnoreCase(str))    ret += "C";
        else if ("long".equalsIgnoreCase(str))    ret += "J";
        else if ("float".equalsIgnoreCase(str))   ret += "F";
        else if ("double".equalsIgnoreCase(str))  ret += "D";
        else ret += "L" + str.replace('.', '/') + ";";

        return ret;
    }

} // DebuggerListener
