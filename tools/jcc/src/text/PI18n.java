/*
 * @(#)PI18n.java	1.3 03/01/14
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

package text;

import java.util.*;

/**
 ** This class is to get the resource file with the current locale
 ** set by JDK. Use ResourceBundle objects to isolate locale-sensitive
 ** date, such as translatable text. The second argument passed to
 ** getBundle method identify which properties file we want to access.
 ** The first argument refers to the actual properties files.
 ** When the locale was created, the language code and country code
 ** were passed to its constructor. The property files are named 
 ** followed by the language and country code.   
 **/
public class PI18n {
    public ResourceBundle bundle = null;
    public String propertyName = null;
    
    public PI18n(String str) {
    propertyName = new String(str);
    }

    public String getString(String key){
       Locale currentLocale = java.util.Locale.getDefault();

       if (bundle == null) {
           try{
               bundle = ResourceBundle.getBundle(propertyName, currentLocale);
           } catch(java.util.MissingResourceException e){
               System.out.println("Could not load Resources");
               System.exit(0);
           }
        }
        String value = new String("");
        try{
            value = bundle.getString(key);
        } catch (java.util.MissingResourceException e){
            System.out.println("Could not find " + key);}
        return value;
    }
}
