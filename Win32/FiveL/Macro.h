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

#if !defined (_Macro_h_)
#define _Macro_h_

#include "Card.h"

/*-----------------------------------------------------------------

CLASS
    Macro

	A set of commands that can be called by name from within a Card
	in the 5L scripting language. 

AUTHOR
    Chuck Officer

-----------------------------------------------------------------*/
class Macro : public Card 
{
	public:
		//////////
		// Constructor.
		// 
		// [in] inFile - index file associated with this macro
		// [in] inName - name of the macro
		// [in_optional] inStart - starting index (in inFile) (default 0)
		// [in_optional] inEnd2 - ending index (in inFile) (default 0)
		//
		Macro(TIndexFile *inFile, const char *inName = NULL, int32 inStart = 0, int32 inEnd2 = 0);

		//////////
		// Execute the set of commands in this macro.  A little different from 
		// Card::Execute.  It does not record CurrentCard/CurrentCommand, and it 
		// does not flush the m_Script once complete.
		//
		virtual void	Execute(void);
		
		//////////
		// Return from the Macro (i.e. end execution of this macro).
		//
		virtual void	Return(void);

	private:
		//////////
		// Is it time to return from the macro?
		//
		bool		m_Return;
		
		//////////
		// Is this macro executing?
		//
		int			m_Running;
};

/*-----------------------------------------------------------------

CLASS
    MacroManager

	Manager for Macro objects.

AUTHOR
    Chuck Officer

-----------------------------------------------------------------*/
class MacroManager : public TIndexManager 
{
	public:
		//////////
		// Create an index for a new Macro and insert it into the macro list. 
		//
		// [in] inFile - index file associated with the new Macro
		// [in] inName - name of the new Macro
		// [in] inStart - starting index (in inFile)
		// [in] inEnd - ending index (in inFile)
		//
		virtual void	MakeNewIndex(TIndexFile *inFile, const char *inName, int32 inStart, int32 inEnd);
};

//////////
// Global Macro Manager
//
extern MacroManager gMacroManager;

#endif // _Macro_h_

/*
 $Log$
 Revision 1.2.6.1  2002/06/06 05:47:30  emk
 3.3.4.1 - Began refactoring the Win5L interpreter to live behind an
 abstract interface.

   * Strictly limited the files which include Card.h and Macro.h.
   * Added TWin5LInterpreter class.
   * Made as much code as possible use the TInterpreter interface.
   * Fixed a few miscellaneous build warnings.

 Revision 1.2  2002/05/15 11:05:33  emk
 3.3.3 - Merged in changes from FiveL_3_3_2_emk_typography_merge branch.
 Synopsis: The Common code is now up to 20Kloc, anti-aliased typography
 is available, and several subsystems have been refactored.  For more
 detailed descriptions, see the CVS branch.

 The merged Mac code hasn't been built yet; I'll take care of that next.

 Revision 1.1.4.1  2002/04/30 07:57:31  emk
 3.3.2.5 - Port Win32 code to use the 20Kloc of Common code that now
 exists.  The (defstyle ...) command should work, but (textaa ...) isn't
 available yet.

 Next up: Implement the (textaa ...) command and the low-level
 GraphicsTools::Image::DrawBitMap.

 Revision 1.1  2001/09/24 15:11:01  tvw
 FiveL v3.00 Build 10

 First commit of /iml/FiveL/Release branch.

 There are now seperate branches for development and release
 codebases.

 Development - /iml/FiveL/Dev
 Release - /iml/FiveL/Release

 Revision 1.4  2000/05/11 12:54:54  chuck
 v 2.01 b2

 Revision 1.3  2000/04/07 17:05:16  chuck
 v 2.01 build 1

 Revision 1.2  1999/09/24 19:57:19  chuck
 Initial revision

*/
