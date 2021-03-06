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
#include "CaptionList.h"
#include "Element.h"
#include "MediaElement.h"
#include "MediaInfoPane.h"

using namespace Halyard;

MediaElement::MediaElement()
    : mEndPlaybackWasCalled(false), mHaveSentMediaFinishedEvent(false),
      mHasPlaybackTimer(false)
{
    // Do nothing.
}

Element *MediaElement::GetThisAsElement() {
    // We know this class is always mixed into the Element hierarchy, so
    // get a pointer to Element that we can use to access our event
    // dispatcher.
    //
    // In theory, we should use C++ virtual inheritence to make
    // MediaElement a subclass of Element, but then classes like
    // MovieElement would inherit from Element along two different routes.
    // This doesn't work unless we use virtual inheritence, and virtual
    // inheritence is notoriously broken (go see Google).  So we're just
    // going to hack it.
    Element *thisAsElement = dynamic_cast<Element*>(this);
    ASSERT(thisAsElement);
    return thisAsElement;
}

void MediaElement::AttachCaptionFile(const std::string &inCaptionFile) {
    mCaptions = shared_ptr<CaptionList>(new CaptionList(inCaptionFile));
}

void MediaElement::EndPlayback() {
    mEndPlaybackWasCalled = true;
}

bool MediaElement::HasReachedFrame(MovieFrame inFrame) {
    if (mEndPlaybackWasCalled ||
        (TInterpreterManager::IsInCommandLineMode() && !IsLooping()))
        // If we've been asked to end this movie, assume that we're at the
        // end.  We also do the same thing if we're in command-line mode,
        // because this allows the buildbot to skip through all media as
        // quickly as possible.
        return true;
    else if (inFrame == LAST_FRAME)
        return IsDone();
    else
        return IsDone() || (CurrentFrame() >= inFrame);
}

void MediaElement::MediaElementIdle() {
    Element *thisAsElement = GetThisAsElement();

    // Decide what frame we're on.  If we're finished with playback, assume
    // that we're on LAST_FRAME.
    // TODO - Code duplication with HasReachedFrame.
    MovieFrame current_frame;
    if (mEndPlaybackWasCalled || IsDone())
        current_frame = LAST_FRAME;
    else
        current_frame = CurrentFrame();

    // See if we have any captions to display, and if so, send an event.
    double current_time = current_frame / FRAMES_PER_SECOND;
    Caption cap;
    if (mCaptions && mCaptions->getCaptionIfChanged(current_time, cap))
        thisAsElement->GetEventDispatcher()->DoEventMediaCaption(cap.text());

    // Check up on our playback timer, if we have one.
    if (mHasPlaybackTimer && HasReachedFrame(mTriggerPlaybackTimerAt)) {
        mHasPlaybackTimer = false;
        thisAsElement->GetEventDispatcher()->DoEventPlaybackTimer();
    }

    // If we've reached the end of the movie, send a MediaFinished event.
    if (!mHaveSentMediaFinishedEvent && HasReachedFrame(LAST_FRAME)) {
        mHaveSentMediaFinishedEvent = true;

        // Send the actual event.
        thisAsElement->GetEventDispatcher()->DoEventMediaFinished();
    }
}

void MediaElement::SetPlaybackTimer(MovieFrame inFrame) {
    if (mHasPlaybackTimer) {
        Element *thisAsElement = GetThisAsElement();
        THROW("Cannot set playback timer on " +
              std::string(thisAsElement->GetName().mb_str()) +
              " a second time unless it is cleared first.");
    }

    mHasPlaybackTimer = true;
    mTriggerPlaybackTimerAt = inFrame;
}

void MediaElement::ClearPlaybackTimer() {
    mHasPlaybackTimer = false;
}

void MediaElement::WriteInfoTo(MediaInfoPane *inMediaInfoPane) {

    wxString info;
    info += GetThisAsElement()->GetName();

    wxString location(GetLocationInfo());
    if (!location.empty())
        info += wxT("\n") + location;

    if (!IsDone()) {
        MovieFrame frames = (int) CurrentFrame(); // May overflow (in theory).
        int seconds = frames / FRAMES_PER_SECOND;
        frames %= FRAMES_PER_SECOND;
        int minutes = seconds / 60;
        seconds %= 60;
        int hours = minutes / 60;
        minutes %= 60;
        wxString timecode;
        timecode.Printf(wxT("%d:%02d:%02d:%02d"), hours, minutes, seconds,
                        frames);
        info += wxT("\n") + timecode;
    }

    inMediaInfoPane->SetText(info);    
}
