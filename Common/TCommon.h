//////////////////////////////////////////////////////////////////////////////
//
//   (c) Copyright 1999, Trustees of Dartmouth College, All rights reserved.
//        Interactive Media Lab, Dartmouth Medical School
//
//			$Author$
//          $Date$
//          $Revision$
//
//////////////////////////////////////////////////////////////////////////////

#if !defined (_TCommon_h_)
#define _TCommon_h_


/*-----------------------------------------------------------------

LIBRARY
    Common

OVERVIEW
	The Common library contains a set of utility classes that are
	used throughout the FiveL codebase.

TCOMMON MACRO DEFINITIONS
	#define ASSERT<br>

	#define not		!<br>
	#define and		&&<br>
	#define or		||<br> 

	#ifndef NULL<br>
	#ifdef __cplusplus<br>
	#define NULL 0<br>
	#else<br>
	#define NULL ((void *) 0)<br>
	#endif<br>
	#endif<br>

	#define int8	char<br>
	#define int16	short<br>
	#define int32	long<br>

	#define uint8	unsigned char<br>
	#define uint16	unsigned short<br>
	#define uint32	unsigned long<br>

	#define MAX_INT8  ((int8)  0x007F)<br>
	#define MAX_INT16 ((int16) 0x7FFF)<br>
	#define MAX_INT32 ((int32) 0x7FFFFFFF)<br>

	#define MAX_UINT8 ((uint8) 0x00FF)<br>
	#define MAX_UINT16 ((uint16) 0xFFFF)<br>
	#define MAX_UINT32 ((uint32) 0xFFFFFFFF)<br>

	#define Max(x, y)   ((x) > (y) ? (x) : (y))<br>
	#define Min(x, y)   ((x) < (y) ? (x) : (y))<br>
	#define Absolute(x) (((x) > 0)? (x): ((x) * -1))<br>

	#define TESTABLE_PRIVATE   private
	#define TESTABLE_PROTECTED protected

AUTHOR
    Chuck Officer<br>

-----------------------------------------------------------------*/

#include "TPlatform.h"

//////////
// We have our own, portable assertion-checking routine because
// we want to log assertion failures (because the users *never*
// write them down).
//
// This routine is defined in TLogger.cpp.
// 
// [in] inTest - If this value is zero, trigger the assertion.
// [in] inDescription - Text describing the assertion.
// [in] inFile - The file in which the assertion appears.
// [in] inLine - The line number of the assertion.
//
extern void FiveLCheckAssertion(int inTest, const char *inDescription,
								const char *inFile, int inLine);

#ifdef DEBUG
#	define ASSERT(test) \
		FiveLCheckAssertion((int) (test), #test, __FILE__, __LINE__);
#else // DEBUG
#	define ASSERT(test) ((void) 0)
#endif // DEBUG

// TODO - These macro names should go away as soon as somebody gets
// a chance to dig through the rest of the source.  It's a big job.
#if FIVEL_PLATFORM_WIN32 || FIVEL_PLATFORM_MACINTOSH
#	define not	!
#	define and	&&
#	define or	|| 
#elif FIVEL_PLATFORM_OTHER
	// These names are reserved C++ operators, and g++ 3.0 doesn't like them.
#else
#	error "Unknown platform."
#endif // FIVEL_PLATFORM_*

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *) 0)
#endif
#endif 

#define int8	char
#define int16	short
#define int32	long

#define uint8	unsigned char
#define uint16	unsigned short
#define uint32	unsigned long

#define MAX_INT8  ((int8)  0x007F)
#define MAX_INT16 ((int16) 0x7FFF)
#define MAX_INT32 ((int32) 0x7FFFFFFF)

#define MAX_UINT8 ((uint8) 0x00FF)
#define MAX_UINT16 ((uint16) 0xFFFF)
#define MAX_UINT32 ((uint32) 0xFFFFFFFF)

#define Max(x, y)   ((x) > (y) ? (x) : (y))
#define Min(x, y)   ((x) < (y) ? (x) : (y))
#define Absolute(x) (((x) > 0)? (x): ((x) * -1))

// Allow our test suites to access some private and protected methods.
// At the top of a test file, write '#define WANT_FIVEL_TEST_INTERFACES 1'.
#ifdef WANT_FIVEL_TEST_INTERFACES
#	define TESTABLE_PRIVATE    public
#	define TESTABLE_PROTECTED  public
#else
#	define TESTABLE_PRIVATE    private
#	define TESTABLE_PROTECTED  protected
#endif

BEGIN_NAMESPACE_FIVEL

//////////
enum TriState
{
	TriStateOff,
	TriStateOn,
	TriStateLatent
};

//////////
enum Alignment 
{
    AlignLeft,
    AlignCenter,
    AlignRight
};

END_NAMESPACE_FIVEL

#endif // _TCommon_h_


/*
 $Log$
 Revision 1.7  2002/05/29 09:28:09  emk
 Warning fix for ASSERT on Linux.

 Revision 1.6  2002/05/15 11:05:17  emk
 3.3.3 - Merged in changes from FiveL_3_3_2_emk_typography_merge branch.
 Synopsis: The Common code is now up to 20Kloc, anti-aliased typography
 is available, and several subsystems have been refactored.  For more
 detailed descriptions, see the CVS branch.

 The merged Mac code hasn't been built yet; I'll take care of that next.

 Revision 1.5.2.3  2002/05/15 09:07:56  emk
 Cast ASSERT test values to (int) to avoid MSVC++ errors.

 Revision 1.5.2.2  2002/05/15 08:13:15  emk
 3.3.2.8 - Overhauled assertion handling to call FatalError and log problems in 5L.log.  Also added hooks for unfading the screen before displaying errors (this is needed to play nicely with the Mac gamma fader).

 Made tweaks to support the migration of Mac (buttpcx ...) to the new anti-aliased typography library.

 The TBTree destructor is still a broken nightmare, especially on FatalError's forced shutdowns.  Expect *both* FiveL's to do something childish immediately after fatal errors and assertion failures.

 Revision 1.5.2.1  2002/04/22 08:17:57  emk
 Updated Common code to build on Macintosh and pass all unit tests.

 Revision 1.5  2002/04/01 19:24:20  emk
 Preliminary style sheet code!

   - All text drawing routines now take Typography::Style arguments instead
     of AbstractFace* values.

   - Typography::Style provides fairly sophisticated (character) style
     sheets--you can copy style sheets, base one style sheet off another,
     and use style sheets to look up faces.  The style-sheet class uses
     an internal STL-style 'rep' object for reference-counting and copy-on-
     write behavior, so these objects are *extremely* light weight.

   - Text colors now work correctly.

   - TextRenderingEngine can still only handle one style at a time.  This
     is next on my TODO list.

 The test suites all pass, and pngtest produces output identical to what we
 had before.

 Revision 1.4  2002/03/08 13:33:41  emk
 Support for testing private and protected class interfaces.

 Revision 1.3  2002/03/04 15:15:57  hamon
 Added support for compiler's namespaces. Namespaces are only enabled on macintosh.
Moved OS specific configuration to TPlatform.h
Changes by Elizabeth and Eric, okayed by Eric.

 Revision 1.2  2002/02/27 16:38:21  emk
 Cross-platform code merge!

 * Merged back in support for the Macintosh platform.  This is an ongoing
   process, and we'll need to do more work.

 * Separated out platform specific configuration with big block comments.

 * Merged in a few changes from KBTree which appeared to fix bugs.

 * Merged in IntToString, UIntToString, DoubleToString changes from the
   Macintosh engine, and improved the error handling.  NOTE: doubles now
   print using "%f" (the way the Mac engine always did it).  This means
   that "tstr = 0.0" will set 'tstr' to "0.000000", not "0" (as it
   did in the Win32 engine).

 This code might not build on Windows.  As soon as I hear from ssharp
 that he's updated the project files, I'll test it myself.

 Revision 1.1  2001/09/24 15:11:00  tvw
 FiveL v3.00 Build 10

 First commit of /iml/FiveL/Release branch.

 There are now seperate branches for development and release
 codebases.

 Development - /iml/FiveL/Dev
 Release - /iml/FiveL/Release

 Revision 1.1  2000/04/06 17:06:10  chuck
 Initial check-in

 Revision 1.2  1999/09/24 19:57:18  chuck
 Initial revision

*/
