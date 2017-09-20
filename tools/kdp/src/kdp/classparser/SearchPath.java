/*
 * @(#)SearchPath.java    1.4 99/05/21
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms. IN NO EVENT WILL SUN OR ITS
 * LICENSORS BE LIABLE FOR ANY LOST REVENUE, PROFIT OR DATA, OR FOR DIRECT,
 * INDIRECT, SPECIAL, CONSEQUENTIAL, INCIDENTAL OR PUNITIVE DAMAGES, HOWEVER
 * CAUSED AND REGARDLESS OF THE THEORY OF LIABILITY, ARISING OUT OF THE USE OF
 * OR INABILITY TO USE SOFTWARE, EVEN IF SUN HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 * 
 * This software is not designed or intended for use in on-line control of
 * aircraft, air traffic, aircraft navigation or aircraft communications; or in
 * the design, construction, operation or maintenance of any nuclear
 * facility. Licensee represents and warrants that it will not use or
 * redistribute the Software for such purposes.
 */

package kdp.classparser;

import kdp.Log;
import java.io.*;
import java.util.*;
import java.lang.*;

public class SearchPath {

    private String pathString;

    private String[] pathArray;

    public SearchPath(String searchPath) {
    //### Should check searchpath for well-formedness.
    StringTokenizer st = new StringTokenizer(searchPath, File.pathSeparator);
    List dlist = new ArrayList();
    while (st.hasMoreTokens()) {
        dlist.add(st.nextToken());
    }
    pathString = searchPath;
        Log.LOGN(3,  "!!!" + pathString );
    pathArray = (String[])dlist.toArray(new String[dlist.size()]);
    Log.LOGN(3, "Path array length is :"+pathArray.length);
    for(int i=0; i<pathArray.length; i++) {
        Log.LOGN(3, pathArray[i]);
    }
    }

    public boolean isEmpty() {
    return (pathArray.length == 0);
    }

    public String asString() {
    return pathString;
    }
    
    public String[] asArray() {
    return (String[])pathArray.clone();
    }
    
    public int path_array_length() {
        return pathArray.length;
    }

    public FileReference resolve(String relativeFileName) {
        Log.LOGN(4, "relative filename = " + relativeFileName);
        Log.LOGN(4, "path array length in resolve is " +pathArray.length);
    if (!relativeFileName.endsWith(".class"))
        relativeFileName += ".class";
    Log.LOGN(4, "relative filename now = " + relativeFileName);
        for (int i = 0; i < pathArray.length; i++) {
//            Log.LOGN(3, "path array = " + pathArray[i]);
//            Log.LOGN(3, "relativeFileName);
            Log.LOGN(5,  "pa=" + pathArray[ i ] + " " +
                                "rfa=" + relativeFileName );
            FileReference path = FileReference.create(pathArray[i], relativeFileName);
            if (path.exists()) {
                Log.LOGN(4,  "  exists" );
                return path;
            }
        }
        return null;
    }

    //### return List?

    public String[] children(String relativeDirName, FilenameFilter filter) {
    // If a file appears at the same relative path
    // with respect to multiple entries on the classpath,
    // the one corresponding to the earliest entry on the
    // classpath is retained.  This is the one that will be
    // found if we later do a 'resolve'.
    SortedSet s = new TreeSet();  // sorted, no duplicates
        for (int i = 0; i < pathArray.length; i++) {
            FileReference path = FileReference.create(pathArray[i], relativeDirName);
            if (path.exists()) {
        String[] childArray = path.list(filter);
        if (childArray != null) {
            for (int j = 0; j < childArray.length; j++) {
            if (!s.contains(childArray[j])) {
                s.add(childArray[j]);
            }
            }
        }
        }
    }
        return (String[])s.toArray(new String[s.size()]);
    }

}
