// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; -*-

#include <wx/wx.h>

#include "AppGlobals.h"
#include "LocationBox.h"
#include "TInterpreter.h"

USING_NAMESPACE_FIVEL


//=========================================================================
//  LocationBox Methods
//=========================================================================

BEGIN_EVENT_TABLE(LocationBox, LOCATION_BOX_PARENT_CLASS)
    EVT_UPDATE_UI(FIVEL_LOCATION_BOX, LocationBox::UpdateUiLocationBox)
	EVT_CHAR(LocationBox::OnChar)

// We only process this event if we're a ComboBox.
#if CONFIG_LOCATION_BOX_IS_COMBO
	EVT_COMBOBOX(FIVEL_LOCATION_BOX, LocationBox::OnComboBoxSelected)
#endif

END_EVENT_TABLE()

#if CONFIG_LOCATION_BOX_IS_COMBO

LocationBox::LocationBox(wxToolBar *inParent)
	: wxComboBox(inParent, FIVEL_LOCATION_BOX, "",
				 wxDefaultPosition, wxSize(200, -1),
				 0, NULL, wxWANTS_CHARS|wxCB_DROPDOWN|wxCB_SORT)
{
}

#else // !CONFIG_LOCATION_BOX_IS_COMBO

LocationBox::LocationBox(wxToolBar *inParent)
	: wxTextCtrl(inParent, FIVEL_LOCATION_BOX, "", wxDefaultPosition,
				 wxSize(200, -1), wxTE_PROCESS_ENTER)
{
}

#endif // !CONFIG_LOCATION_BOX_IS_COMBO

void LocationBox::NotifyEnterCard()
{
	ASSERT(TInterpreter::HaveInstance());
	std::string card = TInterpreter::GetInstance()->CurCardName();
	SetValue(card.c_str());
	RegisterCard(card.c_str());
}

void LocationBox::RegisterCard(const wxString &inCardName)
{
#if CONFIG_LOCATION_BOX_IS_COMBO
	// Update our drop-down list of cards.
	if (FindString(inCardName) == -1)
		Append(inCardName);	
#endif // CONFIG_LOCATION_BOX_IS_COMBO
}

void LocationBox::TryJump(const wxString &inCardName)
{
    if (TInterpreter::HaveInstance())
	{
		TInterpreter *interp = TInterpreter::GetInstance();
		if (interp->IsValidCard(inCardName))
		{
			// Add the specified card to our list and jump to it.
			RegisterCard(inCardName);
			interp->JumpToCardByName(inCardName);
		}
		else
		{
			wxLogError("The card \"" + inCardName + "\" does not exist.");
		}
	}
}

void LocationBox::UpdateUiLocationBox(wxUpdateUIEvent &inEvent)
{
	inEvent.Enable(TInterpreter::HaveInstance());
}

void LocationBox::OnChar(wxKeyEvent &inEvent)
{
	if (inEvent.GetKeyCode() == WXK_RETURN)
		TryJump(GetValue());
	else
		inEvent.Skip();
}

#if CONFIG_LOCATION_BOX_IS_COMBO

void LocationBox::OnComboBoxSelected(wxCommandEvent &inEvent)
{
	TryJump(inEvent.GetString());
}

#endif // CONFIG_LOCATION_BOX_IS_COMBO

void LocationBox::Prompt()
{
	SetFocus();
}