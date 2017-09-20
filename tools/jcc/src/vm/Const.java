/*
 *    Const.java    1.22%    00/02/19 SMI
 *
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*
 * Constants used by the Java linker which are not exported in any
 * direct way from the Java runtime or compiler.
 * These include the opcode values of the quick bytecodes, the instruction
 * lengths of same, and the size of certain data structures in the .class
 * file format.
 * These have been split to more appropriate places, but are collected here
 * for the convenience of old code that doesn't know which parts are which.
 * util.ClassFileConst has constants from the Red book, such as assess flags,
 *    type numbers and signature characters.
 * vm.VMConst has VM-specific things, such as internal flag bits used in
 *    romizer output
 * OpcodeConst is a file generated from the current opcode list. This is
 *    pretty stable in JVM, very unstable in EVM.
 */

package vm;

public interface Const extends util.ClassFileConst, vm.VMConst, OpcodeConst
{
}
