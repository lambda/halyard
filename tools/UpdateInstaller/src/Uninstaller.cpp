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

#include "Uninstaller.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>


/// This function is called at uninstallation time to clean up any leftover
/// lock files.  Without this, a failed update would make it effectively
/// impossible for ordinary users to uninstall and reinstall the program.
void Uninstaller::DeleteLockFileForUninstall(const path &root) {
    path lock(root / LOCK_NAME);
    if (exists(lock))
        remove(lock);
}

