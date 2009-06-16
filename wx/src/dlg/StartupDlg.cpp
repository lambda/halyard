// -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// @BEGIN_LICENSE
//
// Halyard - Multimedia authoring and playback system
// Copyright 1993-2009 Trustees of Dartmouth College
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// @END_LICENSE

#include "AppHeaders.h"
#include <wx/config.h>
#include "../HalyardApp.h"
#include "../StageFrame.h"
#include "StartupDlg.h"

using namespace Halyard;

BEGIN_EVENT_TABLE(StartupDlg, XrcDlg)
    EVT_BUTTON(XRCID("DLG_STARTUP_NEW"), StartupDlg::OnNew)
    EVT_BUTTON(XRCID("DLG_STARTUP_OPEN"), StartupDlg::OnOpen)
    EVT_BUTTON(wxID_OK, StartupDlg::OnRecent)
END_EVENT_TABLE()

StartupDlg::StartupDlg(wxWindow *inParent)
    : XrcDlg(inParent, wxT("DLG_STARTUP"))
{
    Bind(mRecentList, XRCID("DLG_STARTUP_LIST"));
    Bind(mNew, XRCID("DLG_STARTUP_NEW"));
    Bind(mRecent, wxID_OK);

    // Load our file history.
    shared_ptr<wxConfigBase> config(new wxConfig);
    config->SetPath(wxT("/Recent"));
    mHistory.Load(*config);
    if (mHistory.GetCount() == 0) {
        mRecent->Disable();
        mRecentList->Disable();
        mNew->SetDefault();
    } else {
        // Insert our files into our dialog box.
        std::vector<wxString> files;
        for (size_t i = 0; i < mHistory.GetCount(); i++)
            files.push_back(mHistory.GetHistoryFile(i));
        mRecentList->InsertItems(files.size(), &files[0], 0);
        mRecent->SetDefault();
        mRecentList->Select(0);
    }
}

void StartupDlg::OnNew(wxCommandEvent &inEvent) {
    Hide();

    BEGIN_EXCEPTION_TRAPPER()
        wxGetApp().GetStageFrame()->NewDocument();
    END_EXCEPTION_TRAPPER(TException::ReportException)
    
    EndModal(XRCID("DLG_STARTUP_NEW"));
}

void StartupDlg::OnOpen(wxCommandEvent &inEvent) {
    Hide();

    BEGIN_EXCEPTION_TRAPPER()
        wxGetApp().GetStageFrame()->OpenDocument();
    END_EXCEPTION_TRAPPER(TException::ReportException)

    EndModal(XRCID("DLG_STARTUP_OPEN"));
}

void StartupDlg::OnRecent(wxCommandEvent &inEvent) {
    Hide();

    BEGIN_EXCEPTION_TRAPPER()
        // Get the selected directory name.
        int selected(mRecentList->GetSelection());
        wxString dir(mHistory.GetHistoryFile(selected));

        if (!wxFileName::DirExists(dir)) {
            gLog.Error("halyard", "This program cannot be found");
        } else {
            // Open the program in dir.
            wxGetApp().GetStageFrame()->OpenDocument(dir);
        }
    END_EXCEPTION_TRAPPER(TException::ReportException)

    EndModal(XRCID("DLG_STARTUP_RECENT"));
}
