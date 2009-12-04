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

#include <windows.h>
#include <sys/utime.h>
#include "InstallTool.h"
#include "Interface.h"
#include "LogFile.h"

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

const char *InstallTool::LOCK_NAME = "UPDATE.LCK";

static bool IsWriteable(const path &name);
static void CopyFileWithRetries(const path &source, const path &dest, 
                                bool move);
static void TouchFile(const path &name);


//=========================================================================
//  Install Tool
//=========================================================================

InstallTool::InstallTool(const path &dst_root) 
  : mDestRoot(dst_root), 
    mExistingFiles(FileSet::FromManifestsInDir(mDestRoot)),
    mIsPossible(true)
{
}

void InstallTool::MarkImpossible(const std::string &reason) {
    LogFile::GetLogFile()->Log("Update is impossible: " + reason + ".");
    mIsPossible = false;
}

bool InstallTool::IsPossible() {
    // Basic sanity check: Don't install to a target directory that lacks
    // a release.spec.  This reduces the chance the updater could
    // accidentally be used to mess up a user's system.
    if (!exists(mDestRoot / "release.spec")) {
        MarkImpossible("Cannot find release.spec");
        return false;
    }
    
    // If we ran across something weird earlier, bail.
    if (!mIsPossible) {
        return false;
    }

    // If one of our file operations appears to be impossible, because
    // the necessary files don't exist or are not readable or writable,
    // bail.
    FileOperation::Vector::const_iterator operation = mOperations.begin();
    for (; operation != mOperations.end(); ++operation) {
        if (!(*operation)->IsPossible()) {
            return false;
        }
    }

    // Otherwise, we're golden.
    return true;
}

void InstallTool::Run() {
    size_t total = mOperations.size();
    UpdateProgressRange(total);
    FileOperation::Vector::const_iterator operation = mOperations.begin();
    for (size_t i = 0; operation != mOperations.end(); ++operation, ++i) {
        UpdateProgress(i);
        (*operation)->Perform();
    }
    UpdateProgress(total);
}


//=========================================================================
//  FileOperation subclasses
//=========================================================================

bool FileDelete::IsPossible() const {
    return IsWriteable(file);
}

void FileDelete::Perform() const {
    if (exists(file))
        remove(file);
}

bool FileTransfer::IsPossible() const {
    // Bug #1107: We've been having trouble with mysterious locks on our
    // source files, on at least one machine.  In an effort to avoid this,
    // we're experimentally adding a "IsWriteable(source)" to this
    // condition, in hopes of detecting any such locks early enough to
    // abort.
    return (!mShouldExist || exists(mSource)) && 
        IsWriteable(mSource) && IsWriteable(mDest);
}

void FileTransfer::Perform() const {
    create_directories(mDest.branch_path());
    if (exists(mDest))
        remove(mDest);
    CopyFileWithRetries(mSource, mDest, mMove);
    TouchFile(mDest);
}

bool CaseRename::IsPossible() const {
    return (exists(mSource) && exists(mDest));
}

void CaseRename::Perform() const {
    CopyFileWithRetries(mSource, mDest, true);
    TouchFile(mDest);
}


//=========================================================================
//  File system helper functions
//=========================================================================

bool IsWriteable(const path &name) {
    if (exists(name) && !is_directory(name)) {
        // If we can't open the file, keep trying every 1/5th of a second
        // for 10 seconds.
        for (int i = 0; i < 50; i++) {
            FILE *file = fopen(name.native_file_string().c_str(), "a");
            if (file != NULL) {
                fclose(file);
                return true;
            }
            Sleep(200);
        }
        return false;
    }

    return true;
}

/// Copy or move a file, retrying several times if the operation fails
void CopyFileWithRetries(const path &source, const path &dest, bool move) {
    // Bug #1107: We've been having trouble with mysterious locks on our
    // source files, on at least one machine.  In an effort to avoid this,
    // we try to copy files to their final destination repeatedly before
    // giving up.
    //
    // Since a failed copy_file will leave the program in an unusable and
    // corrupted state, we try several times before giving up, just in case
    // any failures are transient.  Note that we use a 1-based loop here
    // for clarity in the 'catch' statement.
    size_t max_retries = 5;
    bool succeeded = false;
    for (size_t i = 1; !succeeded && i <= max_retries; ++i) {
        try {
            if (move) {
                rename(source, dest);
            } else {
                copy_file(source, dest);
            }
            succeeded = true;
        } catch (filesystem_error &) {
            // If we've exhausted our last retry, then rethrow this error.
            // Otherwise, sleep and try again.
            if (i == max_retries)
                throw;
            else
                Sleep(500);
        }
    }
}

void TouchFile(const path &name) {
    utime(name.native_file_string().c_str(), NULL);
}
