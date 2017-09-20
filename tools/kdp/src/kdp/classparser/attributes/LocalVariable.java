/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package kdp.classparser.attributes;
import kdp.Log;

public class LocalVariable
{
    private String name;
    private String type ;
    private int length;
    private int slot;
    private long codeIndex;
    
    public LocalVariable(String name, String type, int codeIndex, 
                         int length, int slot ){
        this.name = name;
        this.type = type;
        this.codeIndex = codeIndex;
        this.length = length;
        this.slot = slot;
    }
    
    
    public String getName(){
        return name;
    }
    
    public String getType(){
        return type;   
    }
    
    public int getLength(){
        return length;
    }
    
    public int getSlot() { 
        return slot;
    }
    
    public long getCodeIndex(){
        return codeIndex;
    }
    
    public void print(){
        Log.LOGN(5, "Name: " + name + "\nClass: " + type);  
        Log.LOGN(5, "CodeIndex: " + codeIndex + "\nLength: " + length);
         Log.LOGN(5, "Slot: " + slot);
    }

}
