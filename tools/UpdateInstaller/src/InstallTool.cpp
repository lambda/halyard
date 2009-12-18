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
#include <boost/foreach.hpp>
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
//  Install Tool interface
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
            MarkImpossible("Cannot " + (*operation)->Describe());
            return false;
        }

        // Call UpdateProgress to let Windows process events, so it
        // doesn't appear that we're frozen.
        UpdateProgress(0);
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
        PerformOperation(*operation);
    }
    UpdateProgress(total);
}

void InstallTool::PerformOperation(const FileOperation::Ptr op) {
    op->Perform();
}

//=========================================================================
//  Install Tool internals
//=========================================================================

bool InstallTool::BuildCleanupRecursive
    (path dir,
     const LowercaseFilenameMap &files_to_delete,
     const LowercaseFilenameMap &files_to_keep,
     const LowercaseFilenameMap &directories_to_keep)
{
    if (!exists(dir)) return false;

    bool contains_undeletable_files = false;

    directory_iterator dir_iter(dir);
    for (; dir_iter != directory_iterator(); ++dir_iter) {
        path full_path(dir_iter->path());
        path relative_path(PathRelativeToTree(full_path));
        std::string relative_path_string(relative_path.string());
        std::transform(relative_path_string.begin(), relative_path_string.end(),
                       relative_path_string.begin(), ::tolower);

        if (is_directory(dir_iter->status())) {
            if (BuildCleanupRecursive(full_path, files_to_delete,
                                      files_to_keep, directories_to_keep))
                contains_undeletable_files = true;
        } else if (files_to_delete.count(relative_path_string) > 0 &&
                   files_to_keep.count(relative_path_string) == 0) {
            // We want to be able to override our explicit deletes by explicitly
            // listing a file to keep; so only delete this file if it's listed
            // in our files_to_delete and not in our files_to_keep.
            FileOperation::Ptr operation(new FileDelete(full_path));
            mOperations.push_back(operation);            
        } else if (files_to_keep.count(relative_path_string) == 0) {
            // This is not in our set of known files, so delete it if
            // it has one of our own file types (as it's assumed to be
            // junk left over from a previous update), or error
            // out if it is not (since in that case we assume that it's
            // some file that the user put there, and may be important
            // to them).
            if (ShouldDeleteFileDuringCleanup(relative_path)) {
                FileOperation::Ptr operation(new FileDelete(full_path));
                mOperations.push_back(operation);
            } else {
                LogFile::GetLogFile()->Log("Unexpected file: " +
                                           full_path.string());
                contains_undeletable_files = true;
            }
        }
    }

    path relative_dir(PathRelativeToTree(dir));
    std::string relative_dir_string(relative_dir.string());
    std::transform(relative_dir_string.begin(), relative_dir_string.end(),
                   relative_dir_string.begin(), ::tolower);
    if (directories_to_keep.count(relative_dir_string) == 0) {
        // This directory should be deleted.  But we can only delete
        // this directory if it contains no undeletable files.
        if (!contains_undeletable_files) {
            FileOperation::Ptr operation(new FileDelete(dir));
            mOperations.push_back(operation);
        } else {
            // If we have undeletable files, and we're in a directory
            // that needs cleanup, then we won't be able to update.
            MarkImpossible("Cannot delete " + dir.string() +
                           "; contains unexpected files");
        }
    }

    // Call UpdateProgress to let Windows process events, so it
    // doesn't appear that we're frozen.
    UpdateProgress(0);

    return contains_undeletable_files;
}

bool InstallTool::ShouldDeleteFileDuringCleanup(const path &file) {
    // In our installer builder, we had a bug where any file which matched
    // the name of one of our manifests would be included in the install
    // but not included in the manifest, leaving it as an extraneous file
    // that wouldn't be deleted.  These files are only used in unit tests
    // that are not visible to the end user, so we can just delete them.
    // The only file that this actually affects is MANIFEST.base, so we
    // look for a file named MANIFEST.base that is not at the root, and
    // mark that for delete during cleanup.
    if (file.filename() == "MANIFEST.base" && file.string() != "MANIFEST.base")
        return true;

    std::string ext(file.extension());
    return (ext == ".zo" || ext == ".ss" || ext == ".dep");
}

InstallTool::LowercaseFilenameMap
InstallTool::DirectoriesForFiles(const FileSet &files) 
{
    // See comment in FileSet.h on named return value optimization.
    LowercaseFilenameMap directory_map;
    BOOST_FOREACH(FileSet::Entry entry, files.Entries()) {
        InsertAncestorDirecotries(directory_map, entry.path());
    }
    
    return directory_map;
}

InstallTool::LowercaseFilenameMap
InstallTool::DirectoriesForFiles(const InstallTool::LowercaseFilenameMap &map) 
{
    LowercaseFilenameMap directory_map;
    BOOST_FOREACH(LowercaseFilenameMap::value_type file, map) {
        InsertAncestorDirecotries(directory_map, file.second);
    }

    return directory_map;
}

void 
InstallTool::InsertAncestorDirecotries(InstallTool::LowercaseFilenameMap &map, 
                                       const std::string &file)
{
    path p(file);
    while (p.has_parent_path()) {
        p = p.parent_path();
        InsertIntoLowercaseFilenameMap(map, p.string());
    }    
}                                                              

InstallTool::LowercaseFilenameMap 
InstallTool::CreateLowercaseFilenameMap(const FileSet &files) {
    LowercaseFilenameMap map;
    BOOST_FOREACH(FileSet::Entry entry, files.Entries()) {
        InsertIntoLowercaseFilenameMap(map, entry.path());
    }
    
    return map;
}

void InstallTool::InsertIntoLowercaseFilenameMap(LowercaseFilenameMap &map,
                                                 const std::string &filename) {
    // Add these paths normalized to lowercase, as we are going to need to
    // compare them case insensitively against paths on the disk.
    // XXX - this is inappropriate for anything outside of US-ASCII;
    // tolower is only guaranteed to work within that range, and
    // general purpose Unicode case folding is well out of scope
    // of this program.  Don't put any characters outside of the US-ASCII
    // range into your tree.
    std::string lower(filename);
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);    
    map.insert(LowercaseFilenameMap::value_type(lower, filename));    
}


//=========================================================================
//  Install Tool utilities
//=========================================================================

/// The full path to where a file should be located in the tree.
path InstallTool::PathInTree(const FileSet::Entry &e) { 
    return mDestRoot / e.path();
}

/// Convert an abosolute path into a path relative to the program install
/// directory.
path InstallTool::PathRelativeToTree(const path &p) {
    path::iterator root_iter(mDestRoot.begin());
    path::iterator relative_iter(p.begin());

    while (++root_iter != mDestRoot.end())
        ++relative_iter;
    path relative_path;
    while (++relative_iter != p.end())
        relative_path /= *relative_iter;
    
    return relative_path;
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

std::string FileDelete::Describe() const {
    return "delete " + file.string();
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

std::string FileTransfer::Describe() const {
    std::string op(mMove ? "move" : "copy");
    return op + " from " + mSource.file_string() + 
        " to " + mDest.file_string();
}

bool CaseRename::IsPossible() const {
    return (exists(mSource) && exists(mDest));
}

void CaseRename::Perform() const {
    CopyFileWithRetries(mSource, mDest, true);
    TouchFile(mDest);
}

std::string CaseRename::Describe() const {
    return "rename from " + mSource.file_string() + " to " + mDest.file_string();
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
