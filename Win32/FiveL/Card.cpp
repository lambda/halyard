// -*- Mode: C++; tab-width: 4; -*-
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

//////////////////////////////////////////////////////////////////////////////
//
// Card.cpp : Execute commands.
//
// (see header file for additional documentation)

#include "stdafx.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "TCommon.h" 

#include "TDeveloperPrefs.h"
#include "TRect.h"
#include "Card.h"
#include "Macro.h"
#include "LUtil.h"
#include "LDiskUtils.h"
#include "Globals.h"
#include "FiveL.h"
#include "Config.h"
#include "Graphics.h"
#include "LHttp.h"
#include "FileSystem.h"
#include "Header.h"
#include "TStyleSheet.h"
#include "TWin5LInterpreter.h"

using FileSystem::GetDataFilePath;
using FileSystem::Path;

CardManager gCardManager;

//  Card - Constructor. Cards have a default origin of (0, 0)
//
Card::Card(TIndexFile *inFile, const char *name, long p1, long p2) 
	: TIndex(inFile, name, p1, p2)
{
	SetOrigin(0, 0);
}

//
//	Start - Get the card ready for execution.
//
void Card::Start(void)
{
	gDebugLog.Log("Start card <%s>", Name());

	gNeedsRefresh = true;
	
	SetScript();		// load the card's code into memory
	m_Script.reset();	// reset it to the start
	SetOrigin(0, 0);	// reset the origin
	m_stopped = false;

	gDebugLog.Log("<%s>", m_Script.GetString());

	// toss (card name
	m_Script >> open >> discard >> discard;
}

void Card::Stop(void)
{
}	

//
//	Execute - Execute commands in the card until
//		we pause or there aren't any more commands
//		or we are jumping somewhere.
//
void Card::Execute(void)
{ 
	if (m_Script.more())
	{
		while ((not gCardManager.Paused())
		   and (m_Script.more())
		   and (not m_stopped)
		   and (not gCardManager.Napping())
		   and (not gCardManager.Jumping()))
		{
			DoCommand();
		}
    }
    
    if (not m_Script.more())
    {
    	if (gNeedsRefresh)
    	{
    		gView->Draw();
    		gNeedsRefresh = false;
    		
    		gCursorManager.CheckCursor();
    	}
    }
}

// Adjust the global rect to local coordinates based on origin.
void Card::AdjustRect(TRect *r)
{
	r->Offset(m_origin);
}

// Adjust the global point to local coordinates based on origin.
void Card::AdjustPoint(TPoint *pt)
{
	pt->Offset(m_origin);
}

// Sets the card's local coordinate system.
void Card::SetOrigin(TPoint &loc)
{
    m_origin = loc;

	gVariableManager.SetLong("_originx", m_origin.X());
	gVariableManager.SetLong("_originy", m_origin.Y());
}

void Card::SetOrigin(int16 inX, int16 inY)
{
	TPoint	newOrigin(inX, inY);
	SetOrigin(newOrigin);
}

// Offsets the card's local coordinate system by the amount given.
void Card::OffsetOrigin(TPoint &delta)
{
	TPoint	newOrigin(m_origin);

	newOrigin.Offset(delta);

	SetOrigin(newOrigin);
}

// Evaluate a given command. Trim opening paren and read opword.
// Opword determines what we parse and then we call the appropriate
// routine DoTheCommand...
void Card::DoCommand(void)
{
    TString     opword;

    m_Script >> open;
    m_Script >> opword;
    
    opword.MakeLower();

	// cbo_fix - create a map of commands
    if (opword == (char *)"if") DoIf();
    else if (opword == (char *)"body") DoBody();
    else if (opword.Equal("add")) DoAdd(m_Script);
    else if (opword.Equal("audiokill")) DoAudioKill(m_Script);
    else if (opword.Equal("audio")) DoAudio(m_Script);
	else if (opword == (char *)"audiovolume") DoAudioVolume(m_Script);
    else if (opword == (char *)"audiowait") DoAudioWait(m_Script);
    else if (opword == (char *)"background") DoBackground(m_Script);
    else if (opword == (char *)"beep") DoBeep(m_Script);
    else if (opword == (char *)"blippo") DoBlippo(m_Script);
    //else if (opword == (char *)"blueramp") DoBlueramp(m_Script);
    else if (opword == (char *)"box") DoBox(m_Script);
	else if (opword.Equal("browse")) DoBrowse(m_Script);
    else if (opword == (char *)"buttpcx") DoButtpcx(m_Script);
    else if (opword == (char *)"checkdisc") DoCheckDisc(m_Script);
	else if (opword.Equal("checkurl")) DoCheckUrl(m_Script);
	else if (opword.Equal("checkvol")) DoCheckVol(m_Script);
    else if (opword == (char *)"close") DoClose(m_Script);
    else if (opword == (char *)"ctouch") DoCTouch(m_Script);
	else if (opword == (char *)"cursor") DoCursor(m_Script);
    else if (opword == (char *)"div") DoDiv(m_Script);
    else if (opword == (char *)"ejectdisc") DoEjectDisc(m_Script);
    else if (opword == (char *)"exit") DoExit(m_Script);
    else if (opword == (char *)"fade") DoFade(m_Script);
	else if (opword.Equal("globalfiles")) DoGlobalFiles(m_Script);
    else if (opword == (char *)"highlight") DoHighlight(m_Script);
    else if (opword == (char *)"hidemouse") DoHidemouse(m_Script);
    else if (opword == (char *)"input") DoInput(m_Script);
    else if (opword == (char *)"jump") DoJump(m_Script);
    else if (opword == (char *)"key") DoKey(m_Script);
    else if (opword == (char *)"keybind") DoKeybind(m_Script);
    else if (opword == (char *)"kill") DoStill(m_Script);
    else if (opword == (char *)"line") DoLine(m_Script);
    else if (opword == (char *)"loadpal") DoLoadpal(m_Script);
    else if (opword == (char *)"loadpic" || opword == (char *)"loadpick") DoLoadpic(m_Script);
	else if (opword == (char *)"lock") DoLock(m_Script);
    else if (opword == (char *)"lookup") DoLookup(m_Script);
    else if (opword == (char *)"micro") DoMicro(m_Script);
    else if (opword == (char *)"nap") DoNap(m_Script);
    else if (opword == (char *)"open") DoOpen(m_Script);
    else if (opword == (char *)"origin") DoOrigin(m_Script);
	else if (opword == (char *)"oval") DoOval(m_Script);
    else if (opword == (char *)"circle") DoOval(m_Script);
    else if (opword == (char *)"pause") DoPause(m_Script);
    else if (opword == (char *)"playqtfile") DoPlayQTFile(m_Script);
    //else if (opword == (char *)"playqtloop") DoPlayQTLoop(m_Script);
    //else if (opword == (char *)"quitqtloop") DoQuitQTLoop(m_Script);
    else if (opword == (char *)"playqtrect") DoPlayQTRect(m_Script);
	else if (opword == (char *)"preload") DoPreloadQTFile(m_Script);   
    else if (opword == (char *)"print") DoPrint(m_Script);
    else if (opword == (char *)"read") DoRead(m_Script);
	else if (opword == (char *)"redoscript") DoReDoScript(m_Script);
	else if (opword == (char *)"resetorigin") DoResetOrigin(m_Script);
    else if (opword == (char *)"resume") DoResume(m_Script);
	else if (opword.Equal("return")) DoReturn(m_Script);
    else if (opword == (char *)"rewrite") DoRewrite(m_Script);
    //else if (opword == (char *)"rnode" || opword == (char *)"rvar") DoRnode(m_Script);
    else if (opword == (char *)"screen") DoScreen(m_Script);
    //else if (opword == (char *)"search") DoSearch(m_Script);
    else if (opword == (char *)"set") DoSet(m_Script);
	else if (opword == (char *)"setwindowtitle") DoSetWindowTitle(m_Script);
    else if (opword == (char *)"showmouse") DoShowmouse(m_Script);
    else if (opword == (char *)"still") DoStill(m_Script);
    else if (opword == (char *)"sub") DoSub(m_Script);
    else if (opword == (char *)"text") DoText(m_Script);
    else if (opword == (char *)"textaa") DoTextAA(m_Script);
    else if (opword == (char *)"timeout") DoTimeout(m_Script);
    else if (opword == (char *)"touch") DoTouch(m_Script);
    else if (opword == (char *)"unblippo") DoUnblippo(m_Script);
    else if (opword == (char *)"unlock") DoUnlock(m_Script);
    else if (opword == (char *)"video") DoPlayQTFile(m_Script);	// change to DoVideo when we can take audio stuff out
    else if (opword == (char *)"wait") DoWait(m_Script);
    else if (opword == (char *)"write") DoWrite(m_Script);
    else DoMacro(opword);

    m_Script >> close;
}

// Execute a single command, perhaps in response to a touch zone or
// a timeout. Save the old m_Script, do the one command, and restore
// the m_Script.
//
// theCommand should look like "(jump aCard)", ie both parens need to
// be there.
void Card::OneCommand(TString &theCommand)
{
    TStream     saveScript(m_Script);
	try
	{
		m_Script = theCommand;
		DoCommand();
	}
	catch (...)
	{
		m_Script = saveScript;
		throw;
	}
	m_Script = saveScript;
}

/************************

    PROTECTED METHODS

************************/

enum EvalMode 
{
    FirstTime,
    And,
    Or
};

/***********************************************************************
 * Function: Card::Evaluate
 *
 *  Parameter conditional (blah AND blah OR etc..)
 * Return:
 *
 * Comments:
 *  Evaluate the given conditional and determine whether or not
 *  it is true.
 ***********************************************************************/
int Card::Evaluate(TStream& conditional)
{
    int         globalRes, localRes, result;
    EvalMode    mode = FirstTime;
	TString		cond;
    TString     op;
    TString     modeStr;
    TString     str1, str2;

    globalRes = localRes = false;

	cond = (const char *) conditional;

    while (conditional.more()) 
    {
        conditional >> str1 >> op >> str2;

        //  Returns <0, 0, or >0.
        //
		op.MakeLower();
		if (op.Equal("contains"))
        {
			if (str1.Contains(str2, false))
				localRes = true;
			else
				localRes = false;        
        }
		else
		{        
	        result = str1.TypeCompare(str2);
	
	        if (op == (char *)"=") 
	        	localRes = (result == 0);
	        else if (op == (char *)"<>") 
	        	localRes = (result != 0);
	        else if (op == (char *)">") 
	        	localRes = (result > 0);
	        else if (op == (char *)">=") 
	        	localRes = (result >= 0);
	        else if (op == (char *)"<") 
	        	localRes = (result < 0);
	        else if (op == (char *)"<=") 
	        	localRes = (result <= 0);
	        else
			{
				gLog.Log("Error: bad If statement <%s>", (const char *) cond);
	            gLog.Log("Error: IF: unknown operator %s.", (const char *) op);
			}
        }
        
        switch (mode) 
        {
            case FirstTime:
                globalRes = localRes;
                break;
            case And:
                globalRes = globalRes && localRes;
                break;
            case Or:
                globalRes = globalRes || localRes;
                break;
        }

        if (conditional.more()) 
        {
            conditional >> modeStr;
            modeStr.MakeLower();
            if (modeStr == (char *)"and") 
            {
                if (mode == Or) 
				{
					gLog.Log("Error: bad If statement <%s>", (const char *) cond);
                	gLog.Log("Error: IF: can't mix ANDs and ORs.");
				}
                mode = And;
            } 
            else if (modeStr == (char *)"or") 
            {
                if (mode == And)
				{
					gLog.Log("Error: bad If statement <%s>", (const char *) cond);
                	gLog.Log("Error: IF: can't mix ANDs and ORs.");
				}
                mode = Or;
            } 
            else
			{
				gLog.Log("Error: bad If statement <%s>", (const char *) cond);
                gLog.Log("Error: IF: expected AND or OR here, not %s.", (const char *) modeStr);
			}
        }
    }
    return globalRes;
}

/*-------------------------------------------------------------------
    (MACRONAME <X Y> <VAR>...)

    Call the macro by name. X, Y is the origin to use for the macro.
    VAR are an optional number of local variables that vary depending
    upon the particular macrodef.
---------------------------------------------------------------------*/
void Card::DoMacro(TString &name)
{
    Macro       *theMacro;
    TString     vname, contents;
    int32       vnum;
    TVariable    *local, *temp, *oldlocal;

    theMacro = (Macro *) gMacroManager.Find(name);
    
    if (theMacro == NULL)
    {
		gLog.Log("Error: Unknown macro/opword %s.", (const char *) name);
		return;
	}

    //  Get the local variables, if passed.
    //
    local = 0;
    vnum = 0;
    while (m_Script.more()) 
    {
        //  Variables are named 1, 2, 3...
        //
        vname = ++vnum;
        m_Script >> contents;

        temp = new TVariable(vname, contents);

        if (local == 0) 
        	local = temp;
        else 
        	local->Add(temp);
    }

    //  Save old local tree and set current local tree to ours.
    //
    oldlocal = gVariableManager.GetLocal();
    gVariableManager.SetLocal(local);

    //  Set the macro's coords to this one's and run the macro.
    //
    theMacro->SetOrigin(m_origin);
    theMacro->Execute();

    //  Restore old local tree and delete ours.
    //
    gVariableManager.SetLocal(oldlocal);
	if (vnum > 0)
    	local->RemoveAll();
}

/*-----------------------------------------------------------------
    (IF (CONDITIONAL) (true_CMD) <(false_CMD)>)

    Evaluate the conditional expression and execute the appropriate
    command based on the value of the expression. Only numbers may
    be compared. It's important that the conditional statement be
    enclosed in parentheses and that the operator (>, <, =) be
    separated from the operands by a space.
-------------------------------------------------------------------*/
void Card::DoIf()
{
    TStream     conditional;

    m_Script >> conditional;
    conditional.reset();

    if (Evaluate(conditional))
        DoCommand();
    else 
    {
        //  Skip true_CMD.
        m_Script >> open >> close;
        if (m_Script.more()) 
        	DoCommand();
    }
}

/*-----------------------------------------------------------------
    (BODY cmd...)

    Evaluate zero or more commands in sequence.  The BODY command
    can be used to pass a list of commands as an argument to the
    IF, BUTTPCX and TOUCH commands.
-------------------------------------------------------------------*/
void Card::DoBody()
{
	while (m_Script.more())
	{
		// Extract our command and put back the parentheses removed
		// by the parser.  This a kludge.
		TString cmd;
		m_Script >> cmd;
		cmd = TString("(") + cmd + TString(")");

		// Execute the command.
		OneCommand(cmd);
	}
}

/*************************

    5L COMMAND METHODS

*************************/

/*----------------------------------------------
    (ADD VARIABLE AMOUNT)

    Adds the given amount to the given variable.
------------------------------------------------*/
void Card::DoAdd(TArgumentList &inArgs)
{
    TString 	vname;
	TString		str_amount;

    inArgs >> vname >> str_amount;

	if (str_amount.Contains("."))
	{
		double  sum;

		sum = gVariableManager.GetDouble(vname);
		sum += (double) str_amount;

		gVariableManager.SetDouble(vname, sum);
	}
	else
	{
		int32	sum;

		sum = gVariableManager.GetLong(vname);
		sum += (int32) str_amount;
		gVariableManager.SetLong(vname, sum);
	}
} 

//
//	DoAudio - Play an audio clip.
//
//	(audio clip_name [offset] [volume] [fade_time] [kill] [loop])
//
//		default values:
//			offset 		0
//			volume 		100
//			fade_time 	0
//			kill		true
//			loop		false
//
void Card::DoAudio(TArgumentList &inArgs)
{
	TString		audio_file;
	TString		flag;
	int32		the_offset = 0;
	int32		the_volume = 100;
	int32		the_fade_time = 0;
	bool		do_loop = false;
	bool		do_kill = false;
	
	inArgs >> audio_file;
	
	if (inArgs.HasMoreArguments())
		inArgs >> the_offset;
	if (inArgs.HasMoreArguments())
		inArgs >> the_volume;
	if (inArgs.HasMoreArguments())
		inArgs >> the_fade_time;
	
	while (inArgs.HasMoreArguments())
	{
		inArgs >> flag;
		flag.MakeLower();
		
		if (flag == (char *) "kill")
			do_kill = true;
		else if (flag == (char *) "loop")
			do_loop = true;
	}
	
	gAudioManager.Play(audio_file, the_offset, the_volume,
			the_fade_time, do_loop, do_kill);
}

//
//	AudioKill - Kill all playing audio clips. Optionally give the
//		fade time in tenths of a second.
//
//	(audiokill [fade_time] [loop])
//
//		defaults values:
//			fade_time	0
//			loop		false
//
void Card::DoAudioKill(TArgumentList &inArgs)
{
	TString		loop_flag;
	int32		fade_time = 0;
    bool		do_kill_loops = false;
    
	if (inArgs.HasMoreArguments())
		inArgs >> fade_time;
	
	if (inArgs.HasMoreArguments())
	{
		inArgs >> loop_flag;
		loop_flag.MakeLower();
		
		if (loop_flag == (char *) "loop")
			do_kill_loops = true;
	}
		
	gAudioManager.Kill(fade_time, do_kill_loops);
}

//
//	AudioVolume - Set the volume of the playing clips. The volume is 
//		given as a number between 0 (off) and 100 (full volume).
//
//	(audiovolume volume [fade_time])
// 
//		default values:
//			fade_time	0
//
void Card::DoAudioVolume(TArgumentList &inArgs)
{
	int32		the_volume;
	int32		the_fade_time = 0;
	
	inArgs >> the_volume;
	
	if (inArgs.HasMoreArguments())
		inArgs >> the_fade_time;
	
	gAudioManager.Volume(the_volume, the_fade_time);
}

//
//	AudioWait - Wait for a particular frame of the playing clip.
//
//	(audiowait frame)
//
void Card::DoAudioWait(TArgumentList &inArgs)
{
	int32		the_frame;
	
	inArgs >> the_frame;
	
	gAudioManager.Wait(the_frame);
}

/*---------------------------------------------------------------
    (BACKGROUND picfile x1 y1 x2 y2)

	Draw the pic in the bounding rectangle (x1 y1 x2 y2)
-----------------------------------------------------------------*/
void Card::DoBackground(TArgumentList &inArgs)
{
    TString     picname;
    LPicture    *thePicture;
    TRect		loc;
    
    inArgs >> picname >> loc;

    thePicture = gPictureManager.GetPicture(picname);
    thePicture->Draw(loc);
}

/*---------------------------------------------------------------
    (BEEP <FREQ> <DURATION>)

    The computer beeps. Optional parameters control the frequency
    (pitch) of the beep and its duration, in tenths of seconds.
-----------------------------------------------------------------*/
 void Card::DoBeep(TArgumentList &inArgs)
{
    int16     freq = 1500;        //  Hz
    int16     duration = 80;      //  Milliseconds

    if (inArgs.HasMoreArguments()) 
    	inArgs >> freq;
    if (inArgs.HasMoreArguments()) 
    {
        inArgs >> duration;
        duration *= 100;
    }

    Beep(freq, duration);
}
/*---------------------------------------------------------------
    (BLIPPO)

    Copy the current display screen to the offscreen buffer while
    still allowing drawing on screen. Use unblippo to restore the
    saved screen.
-----------------------------------------------------------------*/
void Card::DoBlippo(TArgumentList &inArgs)
{
	gView->Blippo();
}

/*----------------------------------------------------------------
    (BLUERAMP one two three four)
    Puts the stupid blueramp inside the given rectangle.
 ----------------------------------------------------------------*/
/*
void Card::DoBlueramp(TArgumentList &inArgs)
{
    TRect bounds;

    inArgs >> bounds;
//    Blueramp(bounds);
}
*/

/*----------------------------------------------------------------
    (BOX LEFT TOP RIGHT BOTTOM FILL COLOR)

    Draws a box of the given color at the coordinates specified.
    If FILL == "FILL", the box is filled in. Otherwise,
    only the outline of the box is drawn.
------------------------------------------------------------------*/
void Card::DoBox(TArgumentList &inArgs)
{
    TRect    	bounds;
    TRect		dirtyRect;
    int16     	color, lineThickness = 1;
    TString 	fill;
    int			do_fill = false;

    
    inArgs >> bounds >> fill >> color;
    if (inArgs.HasMoreArguments()) 
    	inArgs >> lineThickness;

    AdjustRect(&bounds);
    fill.MakeLower();
    if (fill == (char *) "fill")
    	do_fill = true;
    
    // when filling, ignore line thickness	
    if ((lineThickness == 1) or (do_fill))
		DrawRect(bounds, color, do_fill);
    	//DrawRect(bounds.left, bounds.top, bounds.right, bounds.bottom, color, do_fill);
	else
	{
		dirtyRect = bounds;
		dirtyRect.SetBottom(dirtyRect.Bottom() + lineThickness);
		dirtyRect.SetRight(dirtyRect.Right() + lineThickness);
		gView->DirtyRect(&dirtyRect);
		
		DrawLine(bounds.Left(), bounds.Top(), bounds.Right(), bounds.Top(), color, lineThickness);
		DrawLine(bounds.Right(), bounds.Top(), bounds.Right(), bounds.Bottom(), color, lineThickness);
		DrawLine(bounds.Left(), bounds.Bottom(), bounds.Right(), bounds.Bottom(), color, lineThickness);
		DrawLine(bounds.Left(), bounds.Top(), bounds.Left(), bounds.Bottom(), color, lineThickness);
	}
}

//
//	DoBrowse - Opens the given URL in the default browser
//
void Card::DoBrowse(TArgumentList &inArgs)
{
	TString		theUrl;

	inArgs >> theUrl;

	gDebugLog.Log("browse: url <%s>", theUrl.GetString());

	if (gBrowserTool.GoToUrl(theUrl))
		PutInBackground();
}

/*---------------------------------------------------------------
    (BUTTPCX PICT X Y header text Command Cursor <second Command>)
    Puts a button with text "text" on the screen and activates a corresponding
    touchzone.  It takes care of the picture and hilited picture display.
    OPTIONAL:  set command to be executed prior to the jump.
-----------------------------------------------------------------*/

void Card::DoButtpcx(TArgumentList &inArgs)
{
    TRect		bounds1, bounds;
    TPoint		buttLoc;
    LPicture	*thePicture, *hiPicture;
    TString     HeaderName, picname, theCommand, Text;
	TCallback	*callback;
    LTouchZone  *z;
    int         dl, fontHeight;
	TString		cursorStr;
	CursorType	cursor = HAND_CURSOR;
	CursorType	tmpCursor = UNKNOWN_CURSOR;

    inArgs >> picname >> buttLoc >> HeaderName >> Text >> callback;

	if (inArgs.HasMoreArguments())
	{
		inArgs >> cursorStr;

		tmpCursor = gCursorManager.FindCursor(cursorStr);
		if (tmpCursor != UNKNOWN_CURSOR)
			cursor = tmpCursor;
		else
			gLog.Caution("Unknown cursor type: <%s>", (const char *) cursorStr);
	}

    AdjustPoint(&buttLoc);

    thePicture = gPictureManager.GetPicture(picname);
    picname += "H";
    hiPicture = gPictureManager.GetPicture(picname);

	gDebugLog.Log("Draw picture at X <%d>, Y <%d>", 
		buttLoc.X(), buttLoc.Y());

    thePicture->Draw(buttLoc, true);
	bounds = thePicture->GetBounds();

	gDebugLog.Log("Picture bounds: T <%d> L <%d>, B <%d>, R <%d>",
		bounds.Top(), bounds.Left(), bounds.Bottom(), bounds.Right());

	bounds1 = bounds;
    dl = bounds1.Bottom() - bounds1.Top();      //...and text...
    fontHeight = gStyleSheetManager.GetLineHeight(HeaderName);

    dl -= fontHeight;
    dl /= 2;
	bounds1.OffsetTop(dl);
	bounds1.OffsetBottom(dl);

	gDebugLog.Log("Draw text at T <%d> L <%d>, B <%d>, R <%d>",
		bounds1.Top(), bounds1.Left(), bounds1.Bottom(), bounds1.Right());

	gStyleSheetManager.DoText(HeaderName, bounds1, Text, gView);

	// Install our touchzone.
	z = new LTouchZone(bounds, callback, cursor, thePicture, 
					   buttLoc, (const char *) Text, HeaderName);
    gTouchZoneManager.Add(z);
    
	gCursorManager.CheckCursor();
	gView->Draw();
}

//
//	CheckDisc - See if the given disc is in a CD drive.
//
//		(checkdisc volume_name wrong_disc no_disc)
//
void Card::DoCheckDisc(TArgumentList &inArgs)
{
	TString			vol_name;
	TString			no_disc;
	TString			wrong_disc;
	TString			jump_card;
	bool			do_jump = false;

	inArgs >> vol_name >> wrong_disc >> no_disc;

	gDebugLog.Log("checkdisc: <%s>, if wrong disc <%s>, if no disc <%s>", 
		vol_name.GetString(), wrong_disc.GetString(), no_disc.GetString());
	
	if ((not do_jump) and (gConfigManager.PlayMedia()))
	{ 
		if (not CDInDrive())
		{
			do_jump = true;
			jump_card = no_disc;
		}

		if (not do_jump)
		{
			// now we either are sure to have a CD in the drive
			//	or we are on a 16 bit system
			//	- check if the correct volume is mounted
			CDStatus	status;

			status = VolIsMounted(gConfigManager.MediaDrive(),
				vol_name);
			if (status == CDMountedWrongVolume)
			{
				do_jump = true;
				jump_card = wrong_disc;
			}
			else if (status == CDNotMounted)
			{
				do_jump = true;
				jump_card = no_disc;
			}
		}
	}
	
	if (do_jump)
	{ 
	    gDebugLog.Log("CheckDisc: failed, jumping to <%s>", (const char *) jump_card);
	    gCardManager.JumpToCardByName((const char *) jump_card);
    	gView->Draw();
    }
}

//
//	DoCheckUrl - Check the validity of the given URL, returns 0 on success
//		Optionally sets a variable to the Http reply value	
//		_ERROR is set to the errorCode
//		(checkurl url [var_to_set])
//
void Card::DoCheckUrl(TArgumentList &inArgs)
{
	TString		url;
	TString		var_name;
	long		errorCode;

	inArgs >> url;

	if (inArgs.HasMoreArguments())
	{
		inArgs >> var_name;
		gVariableManager.SetString(var_name, "0");
	}

	gDebugLog.Log("checkurl: <%s> <%s>", url.GetString(), var_name.GetString());

	LHttpError error;
	error = gHttpTool.CheckURL(url, var_name);

	switch (error)
	{
		case LHTTP_OK:
			errorCode = 0;
			break;
		case LHTTP_URLNotFound:
			errorCode = -2;
			break;
		default:
			errorCode = -3;
			break;
	}

	gVariableManager.SetLong("_ERROR", errorCode);
}

//
//	DoCheckVol - 
//		(checkvol volume_name real_path [no_volume])
//
void Card::DoCheckVol(TArgumentList &inArgs)
{
	TString		vol_name;
	TString		real_path_var;
	TString		no_volume;
	TString		path_to_vol;

	inArgs >> vol_name >> real_path_var;

	if (inArgs.HasMoreArguments())
		inArgs >> no_volume;

	gDebugLog.Log("checkvol: <%s>, put path into <%s>, if volume not found <%s>",
		vol_name.GetString(), real_path_var.GetString(), no_volume.GetString());

	gVariableManager.SetLong(real_path_var, 0);

	if (FindMountPoint(vol_name, path_to_vol))
	{
		// found it, set variable
		gVariableManager.SetString(real_path_var.GetString(), 
			path_to_vol);
	}
	else if (not no_volume.IsEmpty())
	{
		// didn't find it, jump to no_volume card
		gDebugLog.Log("checkvol: failed, jumping to <%s>",
			no_volume.GetString());

		gCardManager.JumpToCardByName(no_volume.GetString());
		gView->Draw();
	}
}

/*----------------------------------------------------------------------
    (CLOSE FILENAME)

    Close the given text file.
------------------------------------------------------------------------*/
void Card::DoClose(TArgumentList &inArgs)
{
	std::string	filename;

	inArgs >> filename;
    Path path = FileSystem::GetDataFilePath(filename);
    gFileManager.Close(path.ToNativePathString().c_str());
}

/*-------------------------------------------------------------
    (CTOUCH)

    Clear all current touch zones. Typically called immediately
    before setting new touch zones.
---------------------------------------------------------------*/
 void Card::DoCTouch(TArgumentList &inArgs)
{
    int16	left, top;

    if (inArgs.HasMoreArguments())  
    {
        inArgs >> left >> top;
        gTouchZoneManager.Clear(left, top);
    }
    else 
    {   
    	gTouchZoneManager.Clear();
    }
    
    gCursorManager.CheckCursor();
}

 //
 //	DoCursor - Change the cursor, if no cursorName provided, changes to the
 //		default cursor
 //
 // (cursor [cursorName])
 //
 void Card::DoCursor(TArgumentList &inArgs)
 {
	 CursorType		theCursor = ARROW_CURSOR;
	 CursorType		tmpCursor;
	 TString		cursorStr;
	 bool			forceShow = false;

	 if (inArgs.HasMoreArguments())
	 {
		 inArgs >> cursorStr;

		 tmpCursor = gCursorManager.FindCursor(cursorStr);
		 if (tmpCursor != UNKNOWN_CURSOR)
		 {
			 theCursor = tmpCursor;
			 forceShow = true;
		 }
		 else
			 gLog.Caution("Unknown cursor type: <%s>", cursorStr.GetString());
	 }

	 gCursorManager.ChangeCursor(theCursor);
	 gCursorManager.ForceShow(forceShow);
 }

/*--------------------------------------------------------
        (DIV X Y)

        X <- X/Y,  X will be truncated to int.
 ---------------------------------------------------------*/
void Card::DoDiv(TArgumentList &inArgs)
{
    double  Divisor;
    long    Dividend;
    TString vname;

    inArgs >> vname >> Divisor;

    if (Divisor == 0.0) 
    	gLog.Log("Error: Division by zero: %s / %f.",(const char *) vname, Divisor);

	Dividend = gVariableManager.GetLong(vname);
    Dividend = (long)(Dividend / Divisor);
    gVariableManager.SetLong(vname, Dividend);
}

/*--------------------------------------------------------
        (EJECTDISC)

        Eject a CD from the CDROM drive.
 ---------------------------------------------------------*/
void Card::DoEjectDisc(TArgumentList &inArgs)
{  
	gVideoManager.Kill();
	gAudioManager.Kill();	

	EjectCD();
}

/*-------------------
    (EXIT)

    Exit the program.
---------------------*/
void Card::DoExit(TArgumentList &inArgs)
{
    int16 WhichSide = 0;
    
    if (inArgs.HasMoreArguments()) 
    	inArgs >> WhichSide;
    
    gCardManager.DoExit(WhichSide);
}

/*---------------------------------------------------------------
    (FADE DIR <TIME>)

    Either FADE IN or FADE OUT. Time is in tenths of seconds.
-----------------------------------------------------------------*/
void Card::DoFade(TArgumentList &inArgs)
{
    TString     direction;
    long		fadeTime = 0;
    bool		fadeIn = false;

    inArgs >> direction;
    
    if (inArgs.HasMoreArguments()) 
    	inArgs >> fadeTime;

    direction.MakeLower();

	gDebugLog.Log("fade: %s, %ld", direction.GetString(), fadeTime);

    if (direction == (char *) "in")
    	fadeIn = true;   
    else if (direction == (char *) "out")
    	fadeIn = false; 
    else
    {
        gLog.Log("Error: Can fade in or out, but can't fade %s.", (const char *) direction);
		return;
	} 

	gView->Fade(fadeIn, fadeTime, true);
}

/*---------------------------------------------------------------
    (GLOBALFILES FILE1 [FILE2]...[FILEN])

    Append file(s) to the list of global files.  Global files are
	those that may be accessed my multiple FiveL users.
-----------------------------------------------------------------*/
#ifdef USE_BUNDLE
void Card::DoGlobalFiles(TArgumentList &inArgs)
{
	TString gFile;
	TString fileList;
	int numFields = 0;

	//  Make a comma separated list
    //
    while (inArgs.HasMoreArguments()) 
    {
        inArgs >> gFile;
        if (numFields > 0)
            fileList += ',';
        numFields++;
        fileList += gFile;
    }

	if (!fileList.IsEmpty())
		gFileManager.AddGlobalFiles(fileList);
}
#endif

/*---------------------------------------------------------
    (HIGHLIGHT PICNAME)

    Will highlight a picture already displayed on screen by
    drawing PICNAMEH.PCX, pausing, and then redrawing
    PICNAME.PCX. Both of these pictures are matted.
-----------------------------------------------------------*/
void Card::DoHighlight(TArgumentList &inArgs)
{
    TString		picName;
    LPicture		*thePicture;
    TPoint	pt;

    inArgs >> picName;

    thePicture = gPictureManager.GetPicture(picName);
	pt = thePicture->GetOrigin();
    thePicture->Hilite(pt); 
}

/*----------------------------------------------------------------
    (HIDEMOUSE)
    As expected, hides the mouse. WARNING!  unknown behv. when TouchScreen!!
 ---------------------------------------------------------------*/
void Card::DoHidemouse(TArgumentList &inArgs)
{
}

/*---------------------------------------------------------------------
    (INPUT STYLE VARIABLE MASK L T R B <REQUIRE>)

    Allow user input from the keyboard. The text they input is saved
    in the given variable. It appears onscreen in the given header
    style, with upper left corner X, Y. The mask is an input mask which
    controls the kind of characters the user may input. (See GX Text
    for more details about the mask.)

    If REQUIRE = true then the entire mask must be filled. Otherwise
    any input, however short, is accepted.
-----------------------------------------------------------------------*/
void Card::DoInput(TArgumentList &inArgs)
{
    TString		vname, mask, style, required;
    TRect		bounds;
    int			fRequire = false;

    inArgs >> style >> vname >> mask >> bounds;

    if (inArgs.HasMoreArguments()) 
    {
        inArgs >> required;
        required.MakeLower();
		if (required.Equal("true"))
        	fRequire = true;
    }
    
    gView->Draw();
    
    AdjustRect(&bounds);
	gInputManager.Start(vname, style, mask, bounds);
}

/*-------------------------
    (JUMP JUMPCARD)

    Jump to the given card.
---------------------------*/
void Card::DoJump(TArgumentList &inArgs)
{
    TString     jumpcard;

    inArgs >> jumpcard;
    
	gDebugLog.Log("jump: <%s>", jumpcard.GetString());

    gCardManager.JumpToCardByName(jumpcard);
    
    gView->Draw();
}

/*--------------------------------------------------------
    (KEY COLOR)

    Switch the overlay mode and set the keycolor to COLOR.
----------------------------------------------------------*/
void Card::DoKey(TArgumentList &inArgs)
{
    int16     newKeyColor;

    inArgs >> newKeyColor;
}

/*-----------------------------------------------------------
    (KEYBIND CHAR <LINKCARD>)

    Bind the given character to the linkcard so that pressing
    ALT-CHAR jumps to the linkcard. If no linkcard is given,
    then remove the binding associated with CHAR.
-------------------------------------------------------------*/
void Card::DoKeybind(TArgumentList &inArgs)
{
    TString		keyEquiv;
	TCallback	*callback = NULL;
    char		theChar;
    
	inArgs >> keyEquiv;
    if (inArgs.HasMoreArguments())
		inArgs >> callback;
    
    keyEquiv.MakeLower();
	if (keyEquiv.Equal("esc"))
    	theChar = 0x1B;
    else
    	theChar = keyEquiv(0);
  
	if (callback)
		gCommandKeyManager.AddCommandKey(theChar, callback);
	else
		gCommandKeyManager.RemoveCommandKey(theChar);
}

/*--------------------------------------------------------------
    (LINE X1 Y1 X2 Y2 COLOR <THICKNESS>)

    Draw a line of given color and thickness (default is 1) from
    x1, y1 to x2, y2.
----------------------------------------------------------------*/
void Card::DoLine(TArgumentList &inArgs)
{
    TPoint	a, b;
    int16		color, thickness = 1;

    inArgs >> a >> b >> color;
    if (inArgs.HasMoreArguments()) 
    	inArgs >> thickness;

    AdjustPoint(&a);
    AdjustPoint(&b);
    DrawLine(a.X(), a.Y(), b.X(), b.Y(), color, thickness);
}

/*-------------------------------------------------------------
    (LOADPAL PICTURE)

    Set the screen palette to the palette of the given picture.
---------------------------------------------------------------*/
void Card::DoLoadpal(TArgumentList &inArgs)
{
    TString 	palname; 
    TString		flag;
    LPalette	*thePal = NULL;
    bool		noload = false;
    bool		lock = false;
    bool		unlock = false;

    inArgs >> palname;
    palname.MakeLower();
    
	gDebugLog.Log("loadpal: <%s>", palname.GetString());

	while (inArgs.HasMoreArguments())
	{
		inArgs >> flag;
		flag.MakeLower(); 
		
		if (flag.Equal("noload"))
			noload = true;
		else if (flag.Equal("lock"))
			lock = true;
		else if (flag.Equal("unlock"))
			unlock = true;
		else
			gLog.Log("Bad flag to loadpal command <%s>", (const char *) flag);
	}

    thePal = gPaletteManager.GetPalette(palname);
 
 	if (thePal == NULL)
 	{
		gDebugLog.Log("Couldn't find palette <%s>", palname.GetString());
		gLog.Log("Can't find graphic <%s>, please reinstall.", palname.GetString());
		return;
	}
	else
	{
		if (lock)
			thePal->Lock();
		else if (unlock)
			thePal->Unlock();
			
		if (not noload)
			gPaletteManager.SetPalette(thePal, true);
	}
}

/*---------------------------------------------------------------------
    (LOADPIC PICTURE X Y <FLAGS...>)

	Display the given picture at the given location (x, y). As mentioned
	above, there are several flags which modify this command's behavior. 
	Standard behavior is to load the picture from the graphics library on
	disk and then display it on screen using the current palette.

    Flags:
		noshow  - Load into memory only, do not display.
		pal		- Display the picture in its own pallete.	
		matte	- Draw only colored (non-white) pixels.
		lock	- Lock the pic in memory
		unlock	- Unlock a pic, allows it to be purged.
-----------------------------------------------------------------------*/
void Card::DoLoadpic(TArgumentList &inArgs)
{
    TString		picname;
    TString		flag;
    LPicture	*thePicture;
    LPalette	*thePal;
    TPoint		loc;
    bool		matte = false;
    bool		noshow = false;
    bool		lock = false;
    bool		unlock = false;
    bool		do_pal = false;
    bool		have_pic = true;
    
    inArgs >> picname >> loc; 
    picname.MakeLower();
    
    AdjustPoint(&loc);

	gDebugLog.Log("loadpic: <%s>, X <%d>, Y <%d>", 
		picname.GetString(), loc.X(), loc.Y());
	
    while (inArgs.HasMoreArguments()) 
    {
		inArgs >> flag;
        flag.MakeLower();
        
        if (flag.Equal("noshow"))
        	noshow = true;
        else if (flag.Equal("pal"))
        	do_pal = true;
        else if (flag.Equal("matte"))
        	matte = true;
        else if (flag.Equal("lock"))
        	lock = true;
        else if (flag.Equal("unlock"))
        	unlock = true;
        else
        	gDebugLog.Log("Bad flag to loadpic command <%s>", flag.GetString());
	} 

	thePicture = gPictureManager.GetPicture(picname);

	if (thePicture == NULL)
	{
		gDebugLog.Log("loadpic: can't find <%s>.", picname.GetString());
		gLog.Log("Error: Can't find graphic <%s>. Please reinstall.", (char *) picname.GetString());
		return;
	}

    if (thePicture != NULL)
    {    	
    	if (lock)
    		thePicture->Lock();
    	else if (unlock)
    		thePicture->Unlock();
    }
        	
    if (do_pal)
    {
		thePal = gPaletteManager.GetPalette(picname);
        		
    	if (thePal != NULL)
    		gPaletteManager.SetPalette(thePal, true);
    }
        
    if ((thePicture != NULL) and (not noshow))
    	thePicture->Draw(loc, matte);
}

/*--------------------------------------------------------
    (LOCK <CLEAR>)

    CLEAR   If given, clear the offscreen buffer. Otherwise,
            copy the current screen contents offscreen.

    Lock the screen so that all drawing takes place in the
    offscreen buffer.
----------------------------------------------------------*/
void Card::DoLock(TArgumentList &inArgs)
{
    TString     clear;
    bool		doClear = false;

    if (inArgs.HasMoreArguments()) 
    {
        inArgs >> clear;
        clear.MakeLower();
        
        if (clear.Equal("true"))
        	doClear = true;
    }
    
    gView->Lock(doClear);
}

/*------------------------------------------------------------------
    (LOOKUP FILENAME FIELD1 <FIELD2> ... <FIELDN>)

    FILENAME    The text file to lookup the record in.

    FIELD1..N   The fields which must match (case not important) the
                record.

    Assumes the text file is a tab delimited, return marked database
    and tries to find the record that matches the first N fields. If
    it succeeds, the file pointer is positioned after the first N
    fields and the scriptor can start doing (read..until tab) calls.
    If it fails, the pointer is at end of file.
--------------------------------------------------------------------*/
void Card::DoLookup(TArgumentList &inArgs)
{
    TString     searchString, param, filename;
    int         numFields = 0;

    inArgs >> filename;

    //  Append all the fields together into a search string that looks
    //  like "field1 TAB field2 TAB ... fieldN"
    //
    while (inArgs.HasMoreArguments()) 
    {
        inArgs >> param;
        if (numFields > 0)
            searchString += '\t';
        numFields++;
        searchString += param;
    }

	Path path = FileSystem::GetDataFilePath(filename.GetString());
	gDebugLog.Log("lookup: look for <%s>, num fields <%d>", searchString.GetString(), numFields);

    gFileManager.Lookup(path.ToNativePathString().c_str(), searchString, numFields);
}

/*-------------------------------------------------------------------
    (MICRO EFFECT)

    Switch to graphics only (micro) mode. The effect defines how this
    transition is accomplished.
---------------------------------------------------------------------*/
void Card::DoMicro(TArgumentList &inArgs)
{
}

/*------------------------------------------------
    (NAP TIME)

    Pause execution for TIME tenths of seconds.
    The user can abort a long nap via the ESC key.
--------------------------------------------------*/
void Card::DoNap(TArgumentList &inArgs)
{
    int32    tenths;

    inArgs >> tenths;

	gDebugLog.Log("nap: for <%ld>", tenths);
    
    gCursorManager.CheckCursor();
    gView->Draw();
    gCardManager.Nap(tenths);
}

/*----------------------------------------------------------------
    (OPEN FILENAME KIND)

    Open a text file. KIND specifies the kind of access one
    will have to the file. It may be APPEND (write only, appending
    to the end of the file), NEW (write only, overwriting
    anything in the file), or READONLY.
------------------------------------------------------------------*/
void Card::DoOpen(TArgumentList &inArgs)
{ 
    TString     filename, kind;
    FileKind    fKind; 
    bool		open_file = true;
    
    inArgs >> filename >> kind;
    kind.MakeLower();

    if (kind.Equal("append"))
    	fKind = fWriteAppend;
    else if (kind.Equal("new"))
    	fKind = fWriteNew;
    else if (kind.Equal("readonly"))
    	fKind = fReadOnly;
    else
	{
        gLog.Log("Error: Unknown open file kind: %s.", (const char *) kind);
		return;
	}

	gDebugLog.Log("open: filename <%s>, mode <%s>", filename.GetString(), kind.GetString());

	Path path = GetDataFilePath(filename.GetString());
    gVariableManager.SetString("_ERROR", "0");
    
/*	Does not seem to work, also leaves an empty file when non-existent -SS
	
	//check for existence of file:
    if ((fKind == fReadOnly) || (fKind == fWriteAppend))
    {
		ifstream theFile;

		theFile.open(tempname, ios::binary);    
        if (!theFile.is_open())
    	{
        	gVariableManager.SetString("_ERROR", "-1");
			gDebugLog.Log("open: file doesn't exist, setting _ERROR to -1");
			open_file = false;
        }
        else
        	theFile.close();
    }	
*/    
    // if not new and we couldn't find it, don't try to open it
    if (open_file)
    	gFileManager.Open(path.ToNativePathString().c_str(), fKind);
}

/*------------------------------------------------------------
    (ORIGIN DX DY)

    Move the local coordinates for this particular card (or
    macro) by the delta values given. This change is an offset
    from whatever the current coordinates are. There is no
    way to set the absolute coordinates for a macro or card!
--------------------------------------------------------------*/
void Card::DoOrigin(TArgumentList &inArgs)
{
    TPoint   delta;

    inArgs >> delta;
    OffsetOrigin(delta);
}

/*----------------------------------------------------------------
    (CIRCLE x y Radius COLOR)

    Draws an oval of the given color at the coordinates specified.
    If FILL == "FILL", the oval is filled in. Otherwise,
    only the outline of the oval is drawn.
------------------------------------------------------------------*/

void Card::DoOval(TArgumentList &inArgs)
{
	TRect		bounds;
	int16     	color, x,y, radius;
	TString 	fill;

	inArgs >> x >> y >> radius >> color;

	DrawCircle(x, y, radius, color);
}


void Card::DoPause(TArgumentList &inArgs)
{
    long    tenths = 0L;

    inArgs >> tenths;

	gDebugLog.Log("pause: time <%ld>", tenths);

    gVideoManager.Pause(tenths);
    gAudioManager.Pause(tenths);
    
    gCursorManager.CheckCursor();
    gView->Draw();
}


/*-----------------------------------------------------------
    (playqtfile file [frame] [pal] [origin])

    Begin playing the QuickTime file from FRAME1..FRAME2.
    
    3/13/96 Changed to (PLAYQTFILE FILE [FRAME] [pal])
    where frame is a global offset for succeeding wait
    statements that depend on a laser disc frame and not
    a QT frame (i.e., non-zero based plays and waits).
    pal loads the palette from the graphics file of the name
    file.pic and uses it as the global video palette.
-------------------------------------------------------------*/
void Card::DoPlayQTFile(TArgumentList &inArgs)
{ 
	LPalette	*thePal;
	TString 	QTfile;
	TString 	PalFile;
	TPoint		movieOrigin(0, 0);
    int32		theOffset = 0;
    bool		audio_only = false;
    bool		have_pal = false;
    
    inArgs >> QTfile;

    if (inArgs.HasMoreArguments())
        inArgs >> theOffset;

    if (inArgs.HasMoreArguments()) 
    {
        inArgs >> PalFile; 
        have_pal = true;
    }   
	
	gDebugLog.Log("playqtfile: file <%s>, offset <%ld>, pal <%s>", 
		QTfile.GetString(), theOffset, PalFile.GetString());

	if (inArgs.HasMoreArguments())
	{
		inArgs >> movieOrigin;

		if ((movieOrigin.X() >= 0) and (movieOrigin.Y() >= 0))
		{
			gDebugLog.Log("playqtfile: set origin to T <%d>, L <%d>",
				movieOrigin.Y(), movieOrigin.X());

			gVideoManager.SetOrigin(movieOrigin);
		}
	}

	if (QTfile.Contains(".a2", false))
    	audio_only = true;
    
    if ((not audio_only) and (not gVideoManager.HaveOrigin()))
	{
		gDebugLog.Log("playing full screen movie, fade out");
		if (gView->BitDepth() > 8)
			gView->BlackScreen();
		else
			gView->Fade(false, 2, false);
			
		gView->ClearScreen(0); 
	}

	// set the palette if we have one	
	if (have_pal)
	{
		char	*chptr;
		
		// strip off the extension
		chptr = ::strstr(PalFile.GetBuffer(), ".");
		if (chptr != NULL)
		{
			*chptr = '\0';
			PalFile.Update();
		}
		
		thePal = gPaletteManager.GetPalette(PalFile);
		if (thePal != NULL)
			gPaletteManager.SetPalette(thePal, false);
	}
    
    if (not audio_only)
    	gVideoManager.Play(QTfile, theOffset);
    else
    	gAudioManager.Play(QTfile, theOffset, 100, 0, false, true);
}

/*-----------------------------------------------------------
    (PLAYQTLOOP AUDIOFILE [FADEIN])

	9-25-96 Looping audio files with fade-in
-------------------------------------------------------------*/

void Card::DoPlayQTLoop(TArgumentList &inArgs)
{
	TString		QTfile;
    long		theFadeTime = 0;
    bool		audio_file;
	
    inArgs >> QTfile;
    if (inArgs.HasMoreArguments())
        inArgs >> theFadeTime;

	gDebugLog.Log("playqtloop: file <%s>, time <%d>", QTfile.GetString(), theFadeTime);

	if (QTfile.Contains(".a2", false))
    	audio_file = true;
    else
    {
		gDebugLog.Log("playqtloop: not an audio file <%s>", QTfile.GetString());
    	return;
    }
    
    gAudioManager.Play(QTfile, 0, 100, theFadeTime, true, true); 
}

/*-----------------------------------------------------------
    (QUITQTLOOP [FADEOUT])

	9-25-96 Kills looping audio files with fade-out
-------------------------------------------------------------*/

void Card::DoQuitQTLoop(TArgumentList &inArgs)
{
	int32	fadeTime = 0;
	 
	if (not gAudioManager.Looping())
		return;
    
    if (inArgs.HasMoreArguments())
        inArgs >> fadeTime;

	gAudioManager.Kill(fadeTime, true);
}

/*-----------------------------------------------------------
    (PLAYQTRECT X1 Y1)

	Sets the origin for the movie display. This applies only
	to the next movie, then is reset. Uses a global grMovieOrigin,
	where x,y = -1 if the default origin is to be used (centers
	the movie on the screen).
	
	Setting the origin also disables autofading of graphics
	and drawing black at the beginning and end of movies. This
	allows graphics to appear and persist with movies on the
	screen.
	
	Coordinates
-------------------------------------------------------------*/

void Card::DoPlayQTRect(TArgumentList &inArgs)
{
    TPoint vidLoc;
    
    inArgs >> vidLoc;

    ASSERT((vidLoc.X() >= 0) and (vidLoc.X() <= 640));
    ASSERT((vidLoc.Y() >= 0) and (vidLoc.Y() <= 480));
    
    gVideoManager.SetOrigin(vidLoc);
}

//
//	(PRELOAD QTFILE [NAP] [SYNC])
//
//	Preload a Quicktime file into memory.			
//
void Card::DoPreloadQTFile(TArgumentList &inArgs)
{
	TString		QTfile;
	TString		syncFlag;
	int32		tenths = 0;
	int32		nap_time = 0;
	int32		start_time = 0;
	int32		end_time = 0;
	bool		audio_file = false;
	bool		doSync = false;

    inArgs >> QTfile;

	if (inArgs.HasMoreArguments())
		inArgs >> tenths;

	if (inArgs.HasMoreArguments())
	{
		inArgs >> syncFlag;

		if (syncFlag.Equal("sync", false))
			doSync = true;
		else
			gDebugLog.Log("preload: bad flag <%s>, looking for sync", syncFlag.GetString());
	}
   
	if (QTfile.Contains(".a2", false))
		audio_file = true;

//	start_time = ::timeGetTime();
//
//	if (tenths > 0)
//	{
//		nap_time = tenths * 100L;
//
//		// figure out the time the nap should stop
//		nap_time += start_time;
//	}

	gDebugLog.Log("preload: file <%s>, tenths <%d>, %s", 
		QTfile.GetString(), tenths, (doSync ? "sync" : "async"));

	if (not QTfile.Equal("0"))
	{
		if (audio_file)
		{
			gAudioManager.Preroll(QTfile, tenths, doSync);
			gVariableManager.SetString("_PreloadedVideo", QTfile.GetString());
		}
		else
		{
			gVideoManager.Preroll(QTfile, tenths, doSync);
			gVariableManager.SetString("_PreloadedAudio", QTfile.GetString());
		}
	}

//	end_time = ::timeGetTime();
//
//	gDebugLog.Log("preload: the preload took <%d> milliseconds", end_time - start_time);
//
//	if (tenths > 0)
//	{
//		// see if we have passed our nap time
//		if (nap_time > end_time)
//		{
//			// have to wait some more
//			tenths = (nap_time - end_time) / 100;
//
//			gDebugLog.Log("preload: going to nap for <%d> tenths", tenths);
//
//			gCursorManager.CheckCursor();
//			gView->Draw();
//			gCardManager.Nap(tenths);
//		}
//	}
}
                                       
void Card::DoPrint(TArgumentList &inArgs)
{
}

/*-----------------------------------------------------------------
    (READ FILENAME VARIABLE <UNTIL DELIM>)

    Read data from a text file and put it into the variable.
    Normally this will read the next word as defined by whitespace.
    Use the UNTIL DELIM construct to read until some other
    delimiter.

    Valid delimiters are TAB, RETURN, EOF, or any single character.

    TODO - Replace the bogus "UNTIL" argument with a keyword
	argument, or remove it entirely.
-------------------------------------------------------------------*/
void Card::DoRead(TArgumentList &inArgs)
{
	TString 		tempname;
    TString         filename, vname, until, delimstr;
    unsigned char   delim;
    TString         res;

    inArgs >> filename >> vname;
 
	Path path = GetDataFilePath(filename.GetString());

    if (inArgs.HasMoreArguments()) 
    {
        inArgs >> until >> delimstr;
        delimstr.MakeLower();
        if (delimstr.Equal("tab")) 
        	delim = '\t';
        else if (delimstr.Equal("return")) 
        	delim = '\n';
        else if (delimstr.Equal("eof"))
        	delim = 0;
        else 
        	delim = delimstr(0);

		gDebugLog.Log("read: into <%s>, until <%s>", vname.GetString(), delimstr.GetString());
        gFileManager.ReadUntil(path.ToNativePathString().c_str(), res, delim);
        
    } 
	else
	{
		gDebugLog.Log("read: into <%s>", res.GetString());
		gFileManager.Read(path.ToNativePathString().c_str(), res);
	}

    gVariableManager.SetString(vname, res);
}

//
//	ReDoScript 
//
void Card::DoReDoScript(TArgumentList &inArgs)
{
	// make sure redoscript functionality is enabled
	if (gDeveloperPrefs.GetPref(REDOSCRIPT) == REDOSCRIPT_OFF)
		return;
	
	TString		theCard;
	inArgs >> theCard;

	gDebugLog.Log("redoscript: <%s>", theCard.GetString());
	gCardManager.DoReDoScript(theCard);
}

//
//	ResetOrigin - Reset the origin to 0,0 or set it to something new.
//
void Card::DoResetOrigin(TArgumentList &inArgs)
{
	TPoint		newOrigin(0, 0);

	if (inArgs.HasMoreArguments())
		inArgs >> newOrigin;

	SetOrigin(newOrigin);
}

/*---------------------------------------------------------------
    (RESUME)

    If the user touched a touch zone and paused a playing segment
    (audio or video) then this command will resume playback.
-----------------------------------------------------------------*/
void Card::DoResume(TArgumentList &inArgs)
{
    gVideoManager.Resume();
    gAudioManager.Resume();
}

//
//	DoReturn - Stop processing and return from the current script
//
void Card::DoReturn(TArgumentList &inArgs)
{
	Return();
}

void Card::Return()
{
	gDebugLog.Log("return: in card <%s>, stop processing", Key());
	m_stopped = true;
}

/*----------------------------------------------------------------------
    (REWRITE FILENAME FIELD1 <FIELD2> ... <FIELDN>)

    FILENAME    The text file which contains the record to be rewritten.

    FIELD1..N   The fields which define the record.

    Given a file that is open for appending, this command will look up
    the given record, as specified by the fields. It will move that
    record to the end of the file and position the file pointer at the
    end of the file (appending) so that the specific data may be
    written.
------------------------------------------------------------------------*/
void Card::DoRewrite(TArgumentList &inArgs)
{ 
    TString     searchString, param, filename;
    int         numFields = 0;

    inArgs >> filename;

    //  Append all the fields together into a search string that looks
    //  like "field1 TAB field2 TAB ... fieldN"
    //
    while (inArgs.HasMoreArguments()) 
    {
        inArgs >> param;
        if (numFields > 0)
            searchString += '\t';
        numFields++;
        searchString += param;
    }

	Path path = GetDataFilePath(filename.GetString());

	gDebugLog.Log("rewrite: <%s>, num fields <%d>", searchString.GetString(), numFields);

    gFileManager.Rewrite(path.ToNativePathString().c_str(), searchString, numFields);
}

/*---------------------------------------------------------------
    (SCREEN COLOR)

    A fast way to fill the entire screen with a particular color.
-----------------------------------------------------------------*/
void Card::DoScreen(TArgumentList &inArgs)
{
    int16 color;

    inArgs >> color; 
    
    gView->ClearScreen(color);
}

/*-----------------------------------
    (SEARCH FRAME [FLAG])

    Search to the given frame number.
        If FLAG present (anything) then wait for completion.
-------------------------------------*/
//void Card::DoSearch(TArgumentList &inArgs)
//{
//    long    frame;
//
//    inArgs >> frame;
//
//}

/*---------------------------------------
    (SET VARIABLE NEWVALUE [flag])

    Sets the variable to the given value.
-----------------------------------------*/
void Card::DoSet(TArgumentList &inArgs)
{
    TString     vname;
    TString		value;
    TString		flag; 
    uint32		date_value;
    int32		date_type;

    inArgs >> vname >> value;
    
    if (inArgs.HasMoreArguments())
    {
    	inArgs >> flag;
    	flag.MakeLower();
    	
    	if (flag.Equal("longdate"))
    		date_type = DT_LONGDATE;
    	else if (flag.Equal("date"))
    		date_type = DT_DATE;
    	else if (flag.Equal("time"))
    		date_type = DT_TIME;
    	else if (flag.Equal("year"))
    		date_type = DT_YEAR;
    	else if (flag.Equal("month"))
    		date_type = DT_MONTH;
    	else if (flag.Equal("longmonth"))
    		date_type = DT_LONGMONTH;
    	else if (flag.Equal("day"))
    		date_type = DT_DAY;
    	else if (flag.Equal("longday"))
    		date_type = DT_LONGDAY;
    	else
    		gDebugLog.Log("bad flag to set command <%s>", flag.GetString());
    	
    	date_value = (uint32) value;
    		
    	gVariableManager.SetDate(vname, date_value, date_type); 
    	
		gDebugLog.Log("set date: <%s> to <%s>", 
			vname.GetString(), gVariableManager.GetString(vname.GetString()));
    } 	
    else 
    {
    	gVariableManager.SetString(vname, value); 
		gDebugLog.Log("Set: <%s> to <%s>", vname.GetString(), value.GetString());
	}
}

/*---------------------------------------------------------------------
    (SETWINDOWTITLE TITLE)

    Set the application window title
 ---------------------------------------------------------------------*/
void Card::DoSetWindowTitle(TArgumentList &inArgs)
{
	TString theTitle;

	inArgs >> theTitle;
	::SetWindowText(hwndApp, theTitle.GetString());
}

/*---------------------------------------------------------------------
    (SHOWMOUSE)

    Shows the mouse (shouldn't be needed, maybe only in conjunction w/ hide~)
 ---------------------------------------------------------------------*/
void Card::DoShowmouse(TArgumentList &inArgs)
{
}

void Card::DoStill(TArgumentList &inArgs)
{
	gVideoManager.Pause(0);
	gAudioManager.Pause(0);
}

/*----------------------------------------------
    (SUB VARIABLE AMOUNT)

    Subtract the given amount from the variable.
------------------------------------------------*/
void Card::DoSub(TArgumentList &inArgs)
{
    TString 	vname;
	TString		str_amount;

    inArgs >> vname >> str_amount;

	if (str_amount.Contains("."))
	{
		double  sum;

		sum = gVariableManager.GetDouble(vname);
		sum -= (double) str_amount;

		gVariableManager.SetDouble(vname, sum);
	}
	else
	{
		int32	sum;

		sum = gVariableManager.GetLong(vname);
		sum -= (int32) str_amount;
		gVariableManager.SetLong(vname, sum);
	}
}

/*--------------------------------------------------------------
    (TEXT HEADER LEFT TOP RIGHT BOTTOM COLOR SHADOW TEXTSTRING)

    Display the given textstring, using the given header style,
    within the given rect. Note that the bottom of the rectangle
    is elastic... it will actually be as much or as little as
    necessary to display all the text.
----------------------------------------------------------------*/
void Card::DoText(TArgumentList &inArgs)
{
    TRect		bounds;
    TString 	header, text;

    inArgs >> header >> bounds >> text;
    
    AdjustRect(&bounds);

	gDebugLog.Log("Text: string <%s>, header <%s>, location (%d, %d, %d, %d)", 
		text.GetString(), header.GetString(), 
		bounds.Left(), bounds.Top(), bounds.Right(), bounds.Bottom());

    gHeaderManager.DoText(header, bounds, text, 0, 0);
}

/*--------------------------------------------------------------
    (TEXTAA STYLESHEET LEFT TOP RIGHT BOTTOM TEXTSTRING)

    Display the given textstring, using the given stylesheet,
    within the given rect. Note that the bottom of the rectangle
    is elastic... it will actually be as much or as little as
    necessary to display all the text.
----------------------------------------------------------------*/
void Card::DoTextAA(TArgumentList &inArgs)
{
	TRect		bounds;
	std::string style, text;

    inArgs >> style >> bounds >> text;

    AdjustRect(&bounds);
	gDebugLog.Log("textaa: style <%s>, text <%s>",
				  style.c_str(), text.c_str());

	try
	{
		gStyleSheetManager.Draw(style, text,
								GraphicsTools::Point(bounds.Left(),
													 bounds.Top()),
								bounds.Right() - bounds.Left(),
								gView);
	}
	catch (std::exception &error)
	{
		gDebugLog.Error("ERROR: %s", error.what());
	}
	catch (...)
	{
		gDebugLog.Error("ERROR: Unknown exception");
	}
}

/*-----------------------------------------------------------
    (TIMEOUT DELAY CARD)

    If the user doesn't respond in DELAY seconds, jump to the
    given card.
-------------------------------------------------------------*/
void Card::DoTimeout(TArgumentList &inArgs)
{
    TString 	cardName;
    int32     	secs;

    inArgs >> secs >> cardName;
    
    gCardManager.Timeout(cardName.GetString(), secs);
}

/*--------------------------------------------------------------
    (TOUCH LEFT TOP RIGHT BOTTOM CMD [CURSOR [PICT [X Y]]])

    Create a touch zone bounded by the given rectangle. Touching
    this touch zone will make the program go to card CARD. If a
    PICT is specified than that picture will highlight in
    reponse to the touch. If there are more than one copy of
    a given picture on the screen, specify an X and Y coordinate
    for the one you want to highlight.
----------------------------------------------------------------*/
void Card::DoTouch(TArgumentList &inArgs)
{
    TRect       bounds;
    TPoint      loc;
	TCallback	*callback;
    TString     picname;
    LPicture    *thePicture = 0;
    LTouchZone  *theZone;
	CursorType	cursor = HAND_CURSOR;
	CursorType	tmpCursor = UNKNOWN_CURSOR;
	TString		cursorStr;

    inArgs >> bounds >> callback;
 
    AdjustRect(&bounds);

	// Get our cursor, if any.
	if (inArgs.HasMoreArguments())
	{
		inArgs >> cursorStr;

		tmpCursor = gCursorManager.FindCursor(cursorStr);
		if (tmpCursor != UNKNOWN_CURSOR)
			cursor = tmpCursor;
	}

	// Get our picture-related arguments.
    if (inArgs.HasMoreArguments()) 
    {
        inArgs >> picname;
        thePicture = gPictureManager.GetPicture(picname);
        if (inArgs.HasMoreArguments()) 
        {
            inArgs >> loc;
            AdjustPoint(&loc);
        } 
        else 
			loc = thePicture->GetOrigin();
    }

    theZone = new LTouchZone(bounds, callback, cursor, thePicture, loc);
    gTouchZoneManager.Add(theZone);
    
    gCursorManager.CheckCursor();
}

/*----------------------------------------------------------
    (UNBLIPPO <EFFECT> <DELAY>)

    Copies the offscreen buffer to the display with a given
    special effect. An optional delay will allow the user to
    control the speed of this effect. DELAY is the length in
    tenths of seconds that the effect should take.
------------------------------------------------------------*/
void Card::DoUnblippo(TArgumentList &inArgs)
{
    TString		effect;
    long		delay = 0;
    
    if (inArgs.HasMoreArguments())
    {
    	inArgs >> effect;
    	if (inArgs.HasMoreArguments())
    		inArgs >> delay;
    }
    
    gView->UnBlippo(effect, delay);
}


/*----------------------------------------------------------
    (UNLOCK <EFFECT> <DELAY>)

    Copies the offscreen buffer to the display with a given
    special effect. An optional delay will allow the user to
    control the speed of this effect. DELAY is the length in
    tenths of seconds that the effect should take.
------------------------------------------------------------*/
void Card::DoUnlock(TArgumentList &inArgs)
{
    TString 	effect;
    long    	delay = 0;

    if (inArgs.HasMoreArguments()) 
    {
        inArgs >> effect;
        if (inArgs.HasMoreArguments()) 
        	inArgs >> delay;
    }

	gView->Unlock(effect, delay);
}

/*------------------------------------------------------
    (VIDEO EFFECT)

    Switch the video only mode with the given effect.

    Note: this effect is accomplished, in most cases, by
    temporarily switching to overlay (key) mode for the
    effect.
--------------------------------------------------------*/
//void Card::DoVideo(TArgumentList &inArgs)
//{
//    TString effect;
//
//    inArgs >> effect;
//}


/*-----------------------------------------------------------
    (WAIT <FRAME>)

	Suspends the execution of the 5L script until
	the specified frame is reached by the videodisc player.
-------------------------------------------------------------*/
void Card::DoWait(TArgumentList &inArgs)
{
    long    frame = 0L;

    if (inArgs.HasMoreArguments()) 
    	inArgs >> frame;

	gDebugLog.Log("wait: <%ld>", frame);
    
    if (gVideoManager.Playing())
    	gVideoManager.Wait(frame);
    else if (gAudioManager.PlayNoLoop())
    	gAudioManager.Wait(frame);
    
    gCursorManager.CheckCursor();
    gView->Draw();
}

/*-----------------------------------------------------------
    (WRITE FILENAME DATA)

    Write the given data to the file.
-------------------------------------------------------------*/
void Card::DoWrite(TArgumentList &inArgs)
{
	TString 	tempname;
    TString     filename, data;

    inArgs >> filename >> data;

	Path path = GetDataFilePath(filename.GetString());

	gDebugLog.Log("write: <%s>", data.GetString());

    gFileManager.Write(path.ToNativePathString().c_str(), data);
}

/***************************

    CARD MANAGER METHODS

***************************/

CardManager::CardManager() : TIndexManager()
{ 
	m_HaveJump = false;
	m_Napping = false; 
	m_Paused = false;
	m_TimeoutTime = 0;
	m_NapTime = 0;
    m_CurrentCard = NULL;
    m_JumpCard = NULL; 
    m_TimeoutCard = NULL;
    m_ExitNow = false;
    m_ExitScript = 0;
	m_ReDoScript = false;
}

void CardManager::RemoveAll(void)
{
	m_HaveJump = false;
	m_Napping = false; 
	m_Paused = false;
	m_TimeoutTime = 0;
	m_NapTime = 0;
    m_CurrentCard = NULL;
    m_JumpCard = NULL; 
    m_TimeoutCard = NULL;
    m_ExitNow = false;
    m_ExitScript = 0;
	m_ReDoScript = false;

 	m_CardList.RemoveAll();   
	TIndexManager::RemoveAll();
}

void CardManager::Idle(void)
{ 
	TVariable	*theAfterVar;
	TVariable	*theBeforeVar;
	Card		*theCard;
	int32		index;
	
	if (m_Paused)
		return;
			
	if (m_Napping)
	{
		// see if it is time to wakeup
		if (m_NapTime < ::timeGetTime())
		{ 
			gDebugLog.Log("CardManager: wake up from nap");
			m_NapTime = 0;
			m_Napping = false;
		}
		else
			return;		// nothing else to do
	}
		 
	if (m_TimeoutTime > 0)
	{
		// see if the timeout timer has expired
		if (m_TimeoutTime < ::timeGetTime())
		{
			m_HaveJump = true;
			m_JumpCard = m_TimeoutCard;
			
			m_TimeoutTime = 0;
			m_TimeoutCard = NULL;
		}
	}

	if (m_HaveJump)
	{
		// process the jump
		if (m_CurrentCard != NULL)
		{ 
			m_PrevCard = m_CurrentCard->Name();
						
			m_CurrentCard->Stop();
		}
		
		// a jump kills the timeout timer
		m_TimeoutTime = 0;
		m_TimeoutCard = NULL; 
		
		// a jump will kill video
		if (gVideoManager.Playing())
			gVideoManager.Kill(); 
			
		// and non-looping audio
		if (gAudioManager.Playing())
			gAudioManager.Kill(0, false);
		
		m_CurrentCard = m_JumpCard;

		// cbo - set the before and after variables
		theBeforeVar = gVariableManager.FindVariable("_BeforeCard", false);
		theBeforeVar->SetLong(0);
		theAfterVar = gVariableManager.FindVariable("_AfterCard", false);
		theAfterVar->SetLong(0);

		index = m_CurrentCard->GetIndex();
		if (m_CardList.ValidIndex(index))
		{
			if (m_CardList.ValidIndex(index - 1))
			{
				theCard = (Card *) m_CardList.Item(index - 1);
				if (theCard != NULL)
					theBeforeVar->SetString(theCard->Name());
			}

			if (m_CardList.ValidIndex(index + 1))
			{
				theCard = (Card *) m_CardList.Item(index + 1);
				if (theCard != NULL)
					theAfterVar->SetString(theCard->Name());
			}
		}
			
		m_JumpCard = NULL;
		m_HaveJump = false;
		
		// load the current card to get the ball rolling
		m_CurrentCard->Start();
	}
	
	if (m_CurrentCard != NULL)
	{
		// process some commands
		m_CurrentCard->Execute();
	}
	
	// see if we need to leave
	if (m_ExitNow)
	{
		SwitchScripts(m_ExitScript);
	}

	// see if we need to do redoscript
	if (m_ReDoScript)
	{
		m_ReDoScript = false;
		ReDoScript(m_ReDoCardName);
	}
}

void CardManager::DoExit(int32 inScript)
{
	m_ExitNow = true;
	m_ExitScript = inScript;
}

void CardManager::OneCommand(TString &inCmd)
{
	if (m_CurrentCard != NULL)
		m_CurrentCard->OneCommand(inCmd);
}

void CardManager::WakeUp(void)
{
	gDebugLog.Log("CardManager: WakeUp");
	m_Paused = false;
}

void CardManager::Pause(void)
{
	gDebugLog.Log("CardManager: Pause");
	m_Paused = true;
}

void CardManager::Timeout(const char *inCardName, int32 inTime)
{
	if (inTime > 0)
	{
		m_TimeoutCard = GetCard(inCardName);
    
    	if (m_TimeoutCard != NULL)
    	{
    		m_TimeoutTime = inTime * 1000L;
    		m_TimeoutTime += ::timeGetTime();
    	}
    }
}

void CardManager::Nap(int32 inTime)
{
	if (inTime > 0)
	{
		gDebugLog.Log("CardManager: Nap for <%ld> tenths", inTime);
		m_Napping = true;
		m_NapTime = inTime * 100L; 
		m_NapTime += ::timeGetTime();
	}
}

void CardManager::KillNap(void)
{
	gDebugLog.Log("CardManager: Kill the nap");

	m_Napping = false;
	m_NapTime = 0;
}

const char *CardManager::CurCardName(void)
{
    if (m_CurrentCard != NULL)
		return ((const char *) (m_CurrentCard->Name()));
	return (NULL);
}

const char *CardManager::PrevCardName(void)
{
	return (m_PrevCard.GetString());
}

void CardManager::JumpToCardByName(const char *inName)
{
	Card	*theCard = NULL;
	
	theCard = GetCard(inName);
	if (theCard != NULL)
		JumpToCard(theCard);
	else
		gLog.Log("Trying to jump to <%s>, couldn't find it", inName);
}

void CardManager::JumpToCard(Card *inCard)
{

	// Make sure inCard is a valid Card
	//ASSERT
	
	if (inCard != NULL)
	{
		m_JumpCard = inCard;
		m_HaveJump = true;
		
		m_Paused = false;		// when jump always wake up manager
	}
	else
		gDebugLog.Log("Trying to jump to a null card");
}

void CardManager::DoReDoScript(TString &inCardName)
{
	m_ReDoScript = true;
	m_ReDoCardName = inCardName;
}

void CardManager::MakeNewIndex(TIndexFile *inFile, const char *inName, 
							   int32 inStart, int32 inEnd)
{
    Card    *newCard;  
    int32	index;

    newCard = new Card(inFile, inName, inStart, inEnd);

	//  when redoscript functionality is enabled, read the script into memory so that 
	//	the file can be changed without invalidating the index information
	if (gDeveloperPrefs.GetPref(REDOSCRIPT) == REDOSCRIPT_ON)
		newCard->SetScript();

    Add(newCard); 
    
    // add the card to our list of cards
	index = m_CardList.Add(newCard);
	newCard->SetIndex(index);
}

/*
 $Log$
 Revision 1.6.2.5  2002/06/11 18:15:42  emk
 3.3.4.4 - Partial separation of primitives from interpreter, and
 various 5L language enhancements related to callbacks.

   - Finished fleshing out TArgumentList, added support for callbacks.
   - Made all built-in primitives access their arguments through the
     TArgument interface.
   - Implemented a BODY command.
   - Changed how the TOUCH, BUTTPCX and KEYBIND commands parse their
     callback arguments.  See below for details; you'll have to change
     some code.  This was necessary to move callback parsing into
     TStream's implementation of the TArgumentList interface.

 5L Language Changes
 -------------------

   * (KEYBIND ...) now takes an arbitrary command instead of a card name.
     As with TOUCH and BUTTPCX, variables are evaluated when the
     keybind is installed, not when it is invoked.  Examples:

       (keybind f (jump foo))
       (keybind a (add x 10))

   * You can now run a series of zero or more commands using (BODY cmd...).
     This should work with IF, TOUCH, BUTTPCX and KEYBIND.  Example:

       (body
         (set x 10)
         (set y 20))

     Commands such as WAIT, JUMP, NAP, etc., will not do what you expect
     unless they're the last statement in a BODY.  This is caused by the
     low-level design of the interpreter, and is non-trivial to fix.

     RETURN is also not BODY-friendly.

     When you pass a body to IF, TOUCH, BUTTPCX or KEYBIND, all the
     variables in the body will be evaluated *before* any code is run!

   * The arguments to BUTTPCX and TOUCH have been rationalized after
     consultation with Douglas.  The commands now work as follows:

       (TOUCH rect cmd [cursor [picture [point]]])
       (BUTTPCX picture point header label cmd [cursor])

     Note that the second callback has disappeared from both TOUCH and
     BUTTPCX; use BODY instead.

 Revision 1.6.2.4  2002/06/05 20:42:38  emk
 3.3.4.2 - Broke Win5L dependencies on TIndex file by moving various pieces
 of code into TWin5LInterpreter.  Windows 5L now accesses the interpreter
 through a well-defined API.  Changes:

   * Removed many direct and indirect #includes of TIndex.h.
   * Added a TInterpreter method ReloadScript, which can be called by the
     higher-level ReDoScript command.
   * Checked in some files which should have been included in the 3.3.4.1
     checkin--these files contain the initial refactorings of Card and Macro
     callsites to go through the TInterpreter interface.

 Up next: Refactor various Do* methods out of Card and into a procedural
 database.

 Revision 1.6.2.3  2002/06/06 05:47:30  emk
 3.3.4.1 - Began refactoring the Win5L interpreter to live behind an
 abstract interface.

   * Strictly limited the files which include Card.h and Macro.h.
   * Added TWin5LInterpreter class.
   * Made as much code as possible use the TInterpreter interface.
   * Fixed a few miscellaneous build warnings.

 Revision 1.6.2.2  2002/06/05 08:50:52  emk
 A small detour - Moved responsibility for script, palette and data directories
 from Config.{h,cpp} to FileSystem.{h,cpp}.

 Revision 1.6.2.1  2002/06/05 07:05:30  emk
 Began isolating the 5L-language-specific code in Win5L:

   * Created a TInterpreter class, which will eventually become the
     interface to all language-related features.
   * Moved ssharp's developer preference support out of Config.{h,cpp}
     (which are tighly tied to the language) and into TDeveloperPrefs.{h,cpp},
     where they will be isolated and easy to port to other platforms.

 Revision 1.6  2002/05/29 13:58:17  emk
 3.3.4 - Fixed various crash-on-exit problems (including those in TBTree,
 TIndex and TLogger::FatalError), and reverted the Win32 _INCR_Y code
 to the behavior that shipped with Genetics.

 Revision 1.5  2002/05/15 11:05:33  emk
 3.3.3 - Merged in changes from FiveL_3_3_2_emk_typography_merge branch.
 Synopsis: The Common code is now up to 20Kloc, anti-aliased typography
 is available, and several subsystems have been refactored.  For more
 detailed descriptions, see the CVS branch.

 The merged Mac code hasn't been built yet; I'll take care of that next.

 Revision 1.4.2.3  2002/05/15 09:23:55  emk
 3.3.2.8 - Last typography branch checkin before merge.

 * Fixed (wait ...) bug which caused all (wait ...) commands to wait
 until the end of the movie.

 * (buttpcx ...) now uses anti-aliased text.

 * Miscellaneous other tweaks and fixes--just getting things into shape
 for the merge.

 Revision 1.4.2.2  2002/05/01 03:27:07  emk
 3.3.2.6 - First Windows engine with (textaa ...) command.

 - Implemented a primitive, slow Image::DrawPixMap command that uses
 ::GetPixel and ::SetPixel to do alpha blending (shudder).  Strangely
 enough, it's about as fast as the somewhat optimized Mac routines.
 Anyone got a good GDI book?

 - Fixed several assertion failures.

 Known problems:

 - Occasional assertion failure on exit.  The reference-counting on
 TIndexFile claims it's getting dereferenced too many times.  This is
 an old bug; all the TBTree and TBNode classes are pretty dodgy.

 - Assertion failure on "Special Variables" screen in 5Ltest.  This is
 caused by overlong lines.

 Revision 1.4.2.1  2002/04/30 07:57:30  emk
 3.3.2.5 - Port Win32 code to use the 20Kloc of Common code that now
 exists.  The (defstyle ...) command should work, but (textaa ...) isn't
 available yet.

 Next up: Implement the (textaa ...) command and the low-level
 GraphicsTools::Image::DrawBitMap.

 Revision 1.4  2002/02/28 15:31:06  tvw
 Fixes subtraction to detect the type of number being subtracted.

 Revision 1.3  2002/02/28 15:13:18  tvw
 Fixes addition to detect the type of number being added.

 Revision 1.2  2002/02/19 12:35:12  tvw
 Bugs #494 and #495 are addressed in this update.

 (1) 5L.prefs configuration file introduced
 (2) 5L_d.exe will no longer be part of CVS codebase, 5L.prefs allows for
     running in different modes.
 (3) Dozens of compile-time switches were removed in favor of
     having a single executable and parameters in the 5L.prefs file.
 (4) CryptStream was updated to support encrypting/decrypting any file.
 (5) Clear file streaming is no longer supported by CryptStream

 For more details, refer to ReleaseNotes.txt

 Revision 1.1  2001/09/24 15:11:00  tvw
 FiveL v3.00 Build 10

 First commit of /iml/FiveL/Release branch.

 There are now seperate branches for development and release
 codebases.

 Development - /iml/FiveL/Dev
 Release - /iml/FiveL/Release

 Revision 1.10  2000/08/08 19:03:40  chuck
 no message

 Revision 1.9  2000/05/11 12:54:53  chuck
 v 2.01 b2

 Revision 1.8  2000/04/07 17:05:15  chuck
 v 2.01 build 1

 Revision 1.7  2000/02/02 15:15:32  chuck
 no message

 Revision 1.6  1999/11/16 13:46:31  chuck
 no message

 Revision 1.5  1999/11/02 17:16:37  chuck
 2.00 Build 8

 Revision 1.4  1999/10/27 19:42:40  chuck
 Better cursor management

 Revision 1.3  1999/10/22 20:29:09  chuck
 New cursor management.

 Revision 1.2  1999/09/24 19:57:18  chuck
 Initial revision

*/
