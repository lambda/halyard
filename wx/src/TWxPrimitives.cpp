// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; -*-

// Needed for RegisterWxPrimitives.
#include <wx/wx.h>
#include <wx/image.h>
#include <wx/html/htmlwin.h> // TODO - Temporary?

#include "TCommon.h"
#include "TPrimitives.h"
#include "TWxPrimitives.h"

// Needed to implement the primitives.
#include "TCommonPrimitives.h"
#include "TStyleSheet.h"
#include "FiveLApp.h"
#include "Stage.h"
#include "Zone.h"
#include "Widget.h"

USING_NAMESPACE_FIVEL
using GraphicsTools::Color;


//=========================================================================
//  RegisterWxPrimitives
//=========================================================================
//  Install our wxWindows-specific primitives.

void FIVEL_NS RegisterWxPrimitives()
{
    REGISTER_5L_PRIMITIVE(DeleteStageObjects);
	REGISTER_5L_PRIMITIVE(EditBox);
	REGISTER_5L_PRIMITIVE(HTML);
	REGISTER_5L_PRIMITIVE(Input);
	REGISTER_5L_PRIMITIVE(Loadpic);
	REGISTER_5L_PRIMITIVE(NotifyEnterCard);
	REGISTER_5L_PRIMITIVE(NotifyExitCard);
    REGISTER_5L_PRIMITIVE(Screen);
    REGISTER_5L_PRIMITIVE(SetWindowTitle);
	REGISTER_5L_PRIMITIVE(TextAA);
    REGISTER_5L_PRIMITIVE(Zone);
}


//=========================================================================
//  Utility Functions
//=========================================================================

static wxRect ConvRect(const TRect &inRect)
{
	return wxRect(wxPoint(inRect.Left(), inRect.Top()),
				  wxPoint(inRect.Right(), inRect.Bottom()));
}

static wxPoint GetPos(const TRect &inRect)
{
	return wxPoint(inRect.Left(), inRect.Top());
}

static wxSize GetSize(const TRect &inRect)
{
	return wxSize(1 + inRect.Right() - inRect.Left(),
				  1 + inRect.Bottom() - inRect.Top());
}

static wxColour ConvColor(GraphicsTools::Color inColor)
{
	// Translate a color using the official translator function.
	return wxGetApp().GetStage()->GetColor(inColor);
}


//=========================================================================
//  Implementation of wxWindows Primitives
//=========================================================================

DEFINE_5L_PRIMITIVE(DeleteStageObjects)
{
	if (!inArgs.HasMoreArguments())
		wxGetApp().GetStage()->DeleteStageObjects();
	else
	{
		while (inArgs.HasMoreArguments())
		{
			std::string name;
			inArgs >> name;
			bool found =
				wxGetApp().GetStage()->DeleteStageObjectByName(name.c_str());
			if (!found)
				gDebugLog.Caution("Deleting non-existant stage object %s.",
								  name.c_str());
		}
	}
}

DEFINE_5L_PRIMITIVE(EditBox)
{
	std::string name, text;
	TRect bounds;

	inArgs >> name >> bounds >> text;

	wxTextCtrl *edit =
		new wxTextCtrl(wxGetApp().GetStage(), -1, text.c_str(),
					   GetPos(bounds), GetSize(bounds),
					   wxBORDER | wxTE_MULTILINE);
	new Widget(wxGetApp().GetStage(), name.c_str(), edit);
}

DEFINE_5L_PRIMITIVE(HTML)
{
	std::string name, file_or_url;
	TRect bounds;

	inArgs >> name >> bounds >> file_or_url;

	wxHtmlWindow *html =
		new wxHtmlWindow(wxGetApp().GetStage(), -1,
						 GetPos(bounds), GetSize(bounds),
						 wxHW_SCROLLBAR_AUTO | wxBORDER);
	html->LoadPage(file_or_url.c_str());
	new Widget(wxGetApp().GetStage(), name.c_str(), html);
}

DEFINE_5L_PRIMITIVE(Input)
{
	TRect bounds;
	uint32 size;
	Color fore, back;

	inArgs >> bounds >> size >> fore >> back;
	wxGetApp().GetStage()->ModalTextInput(ConvRect(bounds), size,
										  ConvColor(fore),
										  ConvColor(back));
}

/*---------------------------------------------------------------------
    (LOADPIC PICTURE X Y <FLAGS...>)

	Display the given picture at the given location (x, y).

	XXX - Flags not implemented!
-----------------------------------------------------------------------*/
DEFINE_5L_PRIMITIVE(Loadpic)
{
	std::string	picname;
    TPoint		loc;

	inArgs >> picname >> loc;

	// Process flags.
	// XXX - We're just throwing them away for now.
	if (inArgs.HasMoreArguments())
	{
		std::string junk;
		while (inArgs.HasMoreArguments())
			inArgs >> junk;

		::SetPrimitiveError("unimplemented",
							"loadpic flags are not implemented");
		return;
	}

	// Convert our image to a bitmap.
	wxImage image;
	image.LoadFile((".\\Graphics\\" + picname + ".png").c_str());
	wxBitmap bitmap(image);

	// Draw the bitmap.
	wxGetApp().GetStage()->DrawBitmap(bitmap, loc.X(), loc.Y());

	// Update our special variables.
	// XXX - TRect constructor uses height/width order!  Ayiee!
	TRect bounds(TRect(loc.Y(), loc.X(),
					   loc.Y() + bitmap.GetHeight(),
					   loc.X() + bitmap.GetWidth()));
	UpdateSpecialVariablesForGraphic(bounds);
}

DEFINE_5L_PRIMITIVE(NotifyEnterCard)
{
	wxGetApp().GetStage()->NotifyEnterCard();
}

DEFINE_5L_PRIMITIVE(NotifyExitCard)
{
	wxGetApp().GetStage()->NotifyExitCard();
}

/*---------------------------------------------------------------------
    (SETWINDOWTITLE TITLE)

    Set the application window title
 ---------------------------------------------------------------------*/
DEFINE_5L_PRIMITIVE(SetWindowTitle)
{
    std::string title;
    inArgs >> title;
    wxGetApp().GetStageFrame()->SetTitle(title.c_str());
}

/*---------------------------------------------------------------
    (SCREEN COLOR)

    A fast way to fill the entire screen with a particular color.
-----------------------------------------------------------------*/
DEFINE_5L_PRIMITIVE(Screen)
{
    Color color;
    inArgs >> color; 
	wxGetApp().GetStage()->ClearStage(ConvColor(color));
}

DEFINE_5L_PRIMITIVE(TextAA)
{
	TRect		bounds;
	std::string style, text;

    inArgs >> style >> bounds >> text;
    gOrigin.AdjustRect(&bounds);
	gStyleSheetManager.Draw(style, text,
							GraphicsTools::Point(bounds.Left(),
												 bounds.Top()),
							bounds.Right() - bounds.Left(),
							wxGetApp().GetStage());
}

DEFINE_5L_PRIMITIVE(Zone)
{
	std::string name;
	TRect bounds;
	TCallback *action;
	
	inArgs >> name >> bounds >> action;
	new Zone(wxGetApp().GetStage(), name.c_str(), ConvRect(bounds), action);
}
