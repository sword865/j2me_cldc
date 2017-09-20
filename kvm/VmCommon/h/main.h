/*
 * Copyright © 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 */

/*=========================================================================
 * SYSTEM:    KVM
 * SUBSYSTEM: Main program; compilation options
 * FILE:      main.h
 * OVERVIEW:  Compilation options setup for fine-tuning the system
 *            and for enabling/disabling various execution, configuration,
 *            debugging, profiling and tracing options.
 * AUTHOR:    Antero Taivalsaari, Sun Labs, 1998
 *            Edited by Doug Simon 11/1998
 *            (and many others since then...)
 *=======================================================================*/

/*=========================================================================
 * This file gives the >>DEFAULT<< values for various compile time flags.
 *
 * Any port can override a value given in this file by either:
 *      - Giving it that value in its port-specific "machine_md.h" file
 *      - Giving it a value in a compiler-specific manner (e.g.,
 *        as a command line parameter in a makefile.)
 *
 * The preferred way of changing the values defined in this file
 * is to edit the port-specific "machine_md.h" file rather than
 * change the default values here.
 *=======================================================================*/

/*=========================================================================
 * General compilation options
 *=======================================================================*/

/*
 * Set this to zero if your compiler does not directly support 64-bit
 * integers.
 */
#ifndef COMPILER_SUPPORTS_LONG
#define COMPILER_SUPPORTS_LONG 1
#endif

/*
 * Set this to a non-zero value if your compiler requires that 64-bit
 * integers be aligned on 0 mod 8 boundaries.
 */
#ifndef NEED_LONG_ALIGNMENT
#define NEED_LONG_ALIGNMENT 0
#endif

/* Set this to a non-zero value if your compiler requires that double
 * precision floating-pointer numbers be aligned on 0 mod 8 boundaries.
 */
#ifndef NEED_DOUBLE_ALIGNMENT
#define NEED_DOUBLE_ALIGNMENT 0
#endif

/*=========================================================================
 * General system configuration options
 * (Note: many of these flags are commonly overridden from makefiles)
 *=======================================================================*/

/* Indicates whether floating point operations are supported or not.
 * Should be 0 in those implementations that are compliant with
 * CLDC Specification version 1.0, and 1 in those implementations
 * that conform to CLDC Specification version 1.1.
 */
#ifndef IMPLEMENTS_FLOAT
#define IMPLEMENTS_FLOAT 1
#endif

/*
 * Turns class prelinking/preloading (JavaCodeCompact) support on or off.
 * If this option is on, KVM can prelink system classes into the
 * virtual machine executable, speeding up VM startup considerably.
 *
 * Note: This flag is commonly overridden from the makefile
 * in our Windows and Unix ports.
 * 
 * Note: To generate the necessary ROM image that needs to be linked
 * with the rest of the VM files, go to directory "kvm/Vm<Port>/build"
 * (e.g., "kvm/VmWin/build") and type "gnumake".
 */
#ifndef ROMIZING
#define ROMIZING 1
#endif

/*
 * Includes or excludes the optional Java Application Manager (JAM)
 * component in the virtual machine.  Refer to Porting Guide for
 * details.
 *
 * Note: This flag is commonly overridden from the makefile
 * in our Windows and Unix ports.
 */
#ifndef USE_JAM
#define USE_JAM 0
#endif

/* Indicates that this port utilizes asynchronous native functions.
 * Currently, this option is supported only on Win32.
 * Refer to KVM Porting Guide for details.
 */
#ifndef ASYNCHRONOUS_NATIVE_FUNCTIONS
#define ASYNCHRONOUS_NATIVE_FUNCTIONS 0
#endif

/*
 * This option was introduced in KVM 1.0.4.  When enabled,
 * the system will include some additional code for the
 * K Native Interface (KNI).  Refer to the KNI Specification
 * for details on KNI.
 *
 * Note: If you do not intend to use KNI, it is recommended
 * that you turn this option off.  Old-style native function
 * calls will be slightly faster when this flag is turned off.
 */
#ifndef USE_KNI
#define USE_KNI 1
#endif

/*=========================================================================
 * Palm-related / legacy system configuration options
 *=======================================================================*/

/* General comment: Most of the options here are related to 
 * the original Spotless/KVM implementation for the Palm OS
 * in 1998.  These options are no longer guaranteed to work
 * correctly, but we haven't removed them, since they can 
 * potentially be useful for some device ports.
 */ 

/* This is a special feature intended mainly for the Palm and similar
 * implementations with limited dynamic memory. If set to a non-zero 
 * value, the implementation will try to move field tables, method 
 * tables, and other immutable structures from dynamic memory to 
 * "static memory" (storage RAM on Palm), which is easy to read but more
 * difficult to write. This frees up valuable space in dynamic RAM.
 *
 * This option is pretty much useless on most target platforms
 * except the Palm OS.
 */
#ifndef USESTATIC
#define USESTATIC 0
#endif

/* Instructs KVM to use an optimization which allows KVM to allocate
 * the Java heap in multiple chunks or segments. This makes it possible
 * to allocate more Java heap space on memory-constrained target
 * platforms such as the Palm.  This option is potentially useful
 * also on other target platforms with segmented memory architecture.
 */
#ifndef CHUNKY_HEAP
#define CHUNKY_HEAP 0
#endif

/* Instructs KVM to implement a compacting garbage collector
 * or to disable compaction. NOTE: Currently compaction cannot
 * be used on those platforms that have a segmented memory
 * architecture (e.g., Palm OS).
 */
#ifndef ENABLE_HEAP_COMPACTION
#define ENABLE_HEAP_COMPACTION !CHUNKY_HEAP
#endif

/* This is a special form of ROMIZING (JavaCodeCompacting) that is used
 * only by the Palm. It allows the rom'ed image to be relocatable even
 * after it is built.  The ROMIZING flag is commonly provided
 * as a command line parameter from a makefile.
 */
#ifndef RELOCATABLE_ROM
#define RELOCATABLE_ROM 0
#endif

/*=========================================================================
 * Memory allocation settings
 *=======================================================================*/

/*=========================================================================
 * COMMENT: The default sizes for overall memory allocation and
 * resource allocation for individual threads are defined here.
 *
 * As a general principle, KVM allocates all the memory it needs
 * upon virtual machine startup.  At runtime, all memory is allocated
 * from within these preallocated areas.  However, note that on
 * many platforms the native functions linked into the VM (e.g.,
 * graphics operations) often perform dynamic memory allocation
 * outside the Java heap.
 *=======================================================================*/

/* The Java heap size that KVM will allocate upon virtual machine
 * startup (in bytes).  Note that this definition is commonly 
 * overridden from the makefile in the Windows and Unix ports.
 */
#ifndef DEFAULTHEAPSIZE
#define DEFAULTHEAPSIZE   256*1024
#endif

/* Maximum size of the master inline cache (# of ICACHE entries)
 * This value must not exceed 65535, because the length of 
 * inlined bytecode parameters is only two bytes (see cache.h).
 * This macro is meaningful only if the ENABLEFASTBYTECODES option
 * is turned on.
 */
#ifndef INLINECACHESIZE
#define INLINECACHESIZE   128
#endif

/* The execution stacks of Java threads in KVM grow and shrink
 * at runtime. This value determines the default size of a new
 * stack frame chunk when more space is needed.
 */
#ifndef STACKCHUNKSIZE
#define STACKCHUNKSIZE    128
#endif

/* The size (in bytes) of a statically allocated area that the
 * virtual machine uses internally in various string operations.
 * See global.h and global.c.
 */
#ifndef STRINGBUFFERSIZE
#define STRINGBUFFERSIZE  512
#endif

/*=========================================================================
 * Garbage collection options
 *=======================================================================*/

/*
 * The following flag is for debugging. If non-zero, it'll cause a
 * garbage collection to happen before every memory allocation.
 * This will help find those pointers that you're forgetting to
 * lock!  Be sure to turn this option off for a production build
 * because the system will run extremely slowly with this option
 * turned on.
 */
#ifndef EXCESSIVE_GARBAGE_COLLECTION
#define EXCESSIVE_GARBAGE_COLLECTION 0
#endif

/*=========================================================================
 * Interpreter execution options (KVM 1.0)
 *=======================================================================*/

/* Turns runtime bytecode replacement (and Deutsch-Schiffman style
 * inline caching) on/off.  The system will run faster with fast
 * bytecodes on, but needs a few kilobytes of extra space for
 * the additional code. Note that you can't use bytecode replacement
 * on those platforms in which bytecodes are stored in non-volatile
 * memory (e.g., ROM)..
 */
#ifndef ENABLEFASTBYTECODES
#define ENABLEFASTBYTECODES 1
#endif

/* This option can be used for turning on/off constant pool integrity
 * checking. When turned on, the system checks that the given constant
 * pool references point to appropriate constant pool entries at
 * runtime, thus verifying the structural integrity of the class.
 * As obvious, the system runs slightly faster with the integrity
 * checking turned off. This check is not required by the JVM
 * spec, but provides some additional assurance that the VM is
 * behaving properly and that classfiles don't contain illegal code.
 */
#ifndef VERIFYCONSTANTPOOLINTEGRITY
#define VERIFYCONSTANTPOOLINTEGRITY 1
#endif

/* This macro makes the virtual machine sleep when it has
 * no better things to do. The default implementation is
 * a busy loop. Most ports usually require a more efficient
 * implementation that allow the VM to utilize the host-
 * specific battery power conservation features.
 *
 */
#ifndef SLEEP_UNTIL
#   define SLEEP_UNTIL(wakeupTime)                              \
        for (;;) {                                              \
                ulong64 now = CurrentTime_md();                 \
                if (ll_compare_ge(now, wakeupTime)) {           \
                        break;                                  \
                }                                               \
    }
#endif

/*=========================================================================
 * KVM 1.0.2 interpreter execution options (redesigned interpreter)
 *=======================================================================*/

/* Enabling this option will cause the interpreter to permit
 * thread rescheduling only at branch and return statements.
 * Disabling the options allows thread rescheduling to occur
 * at the start of every bytecode (as in KVM 1.0).  Turning
 * this option on will speed up the interpreter by about 5%
 * (simply because the time slice counter will be updated
 * less frequently.)
 */
#ifndef RESCHEDULEATBRANCH
#define RESCHEDULEATBRANCH 1
#endif

/* The time slice factor is used to calculate the base time slice and to
 * set up a timeslice when a thread has its priority changed. This
 * value determines how much execution time each thread receives
 * before a thread switch occurs. Default value 1000 is used when  
 * the RESCHEDULEATBRANCH option is on (meaning that the VM will
 * switch threads when 1000 branch instructions have been executed.)
 * If RESCHEDULEATBRANCH option is off, we use value 10000 (meaning
 * that thread switch occurs after this many bytecodes have been
 * executed.)
 */
#ifndef TIMESLICEFACTOR
#if RESCHEDULEATBRANCH
#define TIMESLICEFACTOR 1000
#else
#define TIMESLICEFACTOR 10000
#endif
#endif

/* The value of this macro determines the basic frequency (as a number
 * of bytecodes executed) by which the virtual machine performs thread
 * switching, event notification, and some other periodically needed
 * operations. A smaller number reduces event handling and thread
 * switching latency, but causes the interpreter to run slower.
 */
#ifndef BASETIMESLICE
#define BASETIMESLICE     TIMESLICEFACTOR
#endif

/* This option will cause the infrequently called Java bytecodes to be 
 * split into a separate interpreter loop. It has been found that 
 * doing so gives a small space and performance benefit on many systems.
 * 
 * IMPORTANT: This option works well only if ENABLEFASTBYTECODES
 * option is also turned on.
 */
#ifndef SPLITINFREQUENTBYTECODES
#define SPLITINFREQUENTBYTECODES 1
#endif

/* This option when enabled will cause the main switch() statement in the
 * interpreter loop to be padded with entries for unused bytecodes. It has
 * been found that doing so on some systems will cause the code for the
 * switch table to be larger, but the resulting code runs substantially 
 * faster. Use this option only when space is not absolutely critical.
 */
#ifndef PADTABLE
#define PADTABLE 0
#endif

/* Turning this option on will allow the VM to allocate all the
 * virtual machine registers (ip, fp, sp, lp and cp) in native
 * registers inside the Interpret() routine.  Enabling this feature
 * significantly speeds up the interpreter (actual performance 
 * improvement depends on how good a job your C compiler does 
 * when compiling the interpreter loop.)  This option is specific
 * to KVM 1.0.2. When turned off, the interpreter speed is about 
 * the same as in KVM 1.0.
 */
#ifndef LOCALVMREGISTERS
#define LOCALVMREGISTERS 1
#endif

/* The following definitions determine which virtual machine registers
 * should be made local to the main interpreter loop.  These macros
 * allow you to more precisely control the allocation of these VM 
 * registers to hardware registers, yet in a portable fashion. This
 * is valuable especially on those target platforms that have a 
 * limited number of hardware registers. Note that these macros
 * have effect only if the LOCALVMREGISTERS option above is turned on.
 *
 * The macros below are defined in the order of "most-to-least
 * bang-for-the-buck". In order to improve Java interpreter
 * performance, IP and SP are most critical, then LP, FP and CP.
 */
/* IP = Instruction Pointer */
#ifndef IPISLOCAL
#define IPISLOCAL 1
#endif

/* SP = Stack Pointer */
#ifndef SPISLOCAL
#define SPISLOCAL 1
#endif

/* LP = Locals Pointer */
#ifndef LPISLOCAL
#define LPISLOCAL 0
#endif

/* FP = Frame Pointer */
#ifndef FPISLOCAL
#define FPISLOCAL 0
#endif

/* CP = Constant Pool Pointer */
#ifndef CPISLOCAL
#define CPISLOCAL 0
#endif

/*=========================================================================
 * Java-level debugging options
 *=======================================================================*/

/* KVM includes an interface that allows the virtual machine
 * to be plugged into a third-party source-level Java debugger
 * such as Forte, CodeWarrior or JBuilder.
 *
 * The macros below are related to the use of the new
 * Java-level debugger.  Actual debugger code is located
 * in directory VmExtra.
 */

/* Include code that interacts with an external Java debugger.
 */
#ifndef ENABLE_JAVA_DEBUGGER
#define ENABLE_JAVA_DEBUGGER 0
#endif

/*=========================================================================
 * Debugging and tracing options
 *=======================================================================*/

/* The macros below enable/disable the debugging of the KVM itself
 * (at the C code level).  These macros, inherited from KVM 1.0,
 * are independent of the Java-level debugging macros defined above.
 */

/* Includes/eliminates a large amount of optional debugging code
 * (such as class and stack frame printing operations) in/from
 * the system.  By eliminating debug code, the system will be smaller.
 * Inclusion of debugging code increases the size of the system but
 * does not introduce any performance overhead unless calls to the
 * debugging routines are added to the source code or various tracing
 * options are turned on.  This option is normally turned off in 
 * production builds.
 */
#ifndef INCLUDEDEBUGCODE
#define INCLUDEDEBUGCODE 0
#endif

/* Turns VM execution profiling and additional safety/sanity
 * checks on/off.  VM will run slightly slower with this option on.
 * This option should normally be turned off in production builds.
 */
#ifndef ENABLEPROFILING
#define ENABLEPROFILING 0
#endif

/*=========================================================================
 * Compile-time flags for choosing different tracing/debugging options.
 * These options can make the system very verbose. Turn them all off
 * for normal VM operation.  When turning them on, it is useful to
 * turn INCLUDEDEBUGCODE on, too, since many of the routines utilize
 * debugging code to provide more detailed information to the user.
 *=======================================================================*/

/* Prints out less verbose messages on small machines */
#ifndef TERSE_MESSAGES
#define TERSE_MESSAGES 0
#endif

/* Turns exception handling backtrace information on/off */
#ifndef PRINT_BACKTRACE
#define PRINT_BACKTRACE INCLUDEDEBUGCODE
#endif

/*=========================================================================
 * Miscellaneous macros and options
 *=======================================================================*/

/* Some functions in the reference implementation take arguments
 * that they do not use. Some compilers will issue warnings; others
 * do not. For those compilers that issue warnings, this macro
 * can be used to indicate that the corresponding variable,
 * though an argument to the function, isn't actually used.
 */
#ifndef UNUSEDPARAMETER
#define UNUSEDPARAMETER(x)
#endif

/* Some ROMized implementations may want to forbid any new classes 
 * from being loaded into any system package.  The following macro
 * defines whether a package name is one of these restricted packages.
 * By default, we prevent dynamic class loading to java.* and javax.*.
 */
#ifndef IS_RESTRICTED_PACKAGE_NAME
#define IS_RESTRICTED_PACKAGE_NAME(name) \
     ((strncmp(name, "java/", 5) == 0) || (strncmp(name, "javax/", 6) == 0))  
#endif

/*=========================================================================
 * VM startup function prototypes
 *=======================================================================*/

int StartJVM(int argc, char* argv[]);
int KVM_Initialize(void);
int KVM_Start(int argc, char* argv[]);
void KVM_Cleanup(void);

