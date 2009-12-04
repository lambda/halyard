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

#include <string>
#include <vector>
#include <cassert>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

#include "UpdateInstaller.h"
#include "FileSet.h"
#include "SpecFile.h"
#include "Interface.h"
#include "LogFile.h"

using namespace boost::filesystem;


//=========================================================================
//  UpdateInstaller setup
//=========================================================================

UpdateInstaller::UpdateInstaller(const path &src_root, const path &dst_root)
    : InstallTool(dst_root),
      mSrcRoot(src_root),
      mSpecFile(mSrcRoot / "Updates/release.spec"),
      mSrcManifestDir(mSrcRoot / "Updates/manifests" / mSpecFile.build()),
      mUpdateFiles(FileSet::FromManifestsInDir(mSrcManifestDir)),
      mFilesNeededForNewTree(mUpdateFiles.MinusExactMatches(mExistingFiles))
{
}

void UpdateInstaller::Prepare() {
    CalculateFileSetsForUpdates();
    BuildFileOperationVector();
}

void UpdateInstaller::CalculateFileSetsForUpdates() {
    FileSet::DigestSet::const_iterator digest_iter = 
        mFilesNeededForNewTree.Digests().begin();

    // For each of the digests that we need to put somewhere in the new tree...
    for (; digest_iter != mFilesNeededForNewTree.Digests().end(); ++digest_iter) {
        // If its already in the pool, we're all set, move on to the next file
        if (exists(PathInPool(*digest_iter)))
            continue;

        // Otherwise, there should be a file with this digest in the old tree
        FileSet::DigestMap::const_iterator entries_with_digest =
            mExistingFiles.DigestEntryMap().find(*digest_iter);
        if (entries_with_digest == mExistingFiles.DigestEntryMap().end()) {
            // Uh, oh! This doesn't appear to be in our tree or our pool.
            // Looks like something went wrong, and we can't update; flag
            // the update as impossible and return
            MarkImpossible("Couldn't find file with digest " + 
                           *digest_iter + " in pool or tree");
            return;
        }

        // Pick the first file with the given digest
        FileSet::Entry file(entries_with_digest->second);

        // If we still need this file at the same location with the
        // same contents
        if (mUpdateFiles.HasMatchingEntry(file)) {
            // Then add this to the list of files to copy into the pool
            mFilesToCopyFromTreeToPool.AddEntry(file);
        } else {
            // Otherwise, add it to the list of files to move to the pool
            mFilesToMoveFromTreeToPool.AddEntry(file);
        }
    }

    // The files left that we need to delete are the files that we have
    // in our current tree, minus the files we are going to move (since
    // by the time we do the deletes the moves will have happened), minus
    // the files that we do actually want exactly unchanged in our update.
    mFilesToDeleteFromTree = 
        mExistingFiles.MinusExactMatches(mFilesToMoveFromTreeToPool)
                      .MinusExactMatches(mUpdateFiles);
}

void UpdateInstaller::BuildFileOperationVector() {
    BuildTreeToPoolFileOperations();
    BuildPoolToTreeFileOperations();
    BuildUpdaterSpecialFileOperations();
    BuildDirectoryCleanupFileOperations();
    BuildCaseRenameFileOperations();
}

void UpdateInstaller::BuildTreeToPoolFileOperations() {
    BOOST_FOREACH(FileSet::Entry file, mFilesToCopyFromTreeToPool.Entries()) {
        FileOperation::Ptr operation(new FileTransfer(PathInTree(file), 
                                                      PathInPool(file),
                                                      /* move? */ false));
        mOperations.push_back(operation);
    }
    BOOST_FOREACH(FileSet::Entry file, mFilesToMoveFromTreeToPool.Entries()) {
        FileOperation::Ptr operation(new FileTransfer(PathInTree(file),
                                                      PathInPool(file),
                                                      /* move? */ true));
        mOperations.push_back(operation);
    }
    BOOST_FOREACH(FileSet::Entry file, mFilesToDeleteFromTree.Entries()) {
        FileOperation::Ptr operation(new FileDelete(PathInTree(file)));
        mOperations.push_back(operation);
    }
}

void UpdateInstaller::BuildPoolToTreeFileOperations() {
    FileSet::DigestMap digest_tracker(mFilesNeededForNewTree.DigestEntryMap());
    BOOST_FOREACH(FileSet::Entry file, mFilesNeededForNewTree.Entries()) {
        FileSet::DigestMap::const_iterator 
            file_for_digest(digest_tracker.find(file.digest()));
        // If this is the last file with this digest, we should do a move
        if (digest_tracker.count(file.digest()) == 1) {
            assert(file_for_digest->second == file);
            digest_tracker.erase(file_for_digest);
            FileOperation::Ptr
                operation(new FileTransfer(PathInPool(file), PathInTree(file),
                                           /* move? */ true,
                                           FileShouldBeInPool(file)));
            mOperations.push_back(operation);
        } else {
            while (file_for_digest != digest_tracker.end() &&
                   !(file_for_digest->second == file)) {
                ++file_for_digest;
            }
            assert(file_for_digest != digest_tracker.end());
            digest_tracker.erase(file_for_digest);
            
            FileOperation::Ptr
                operation(new FileTransfer(PathInPool(file), PathInTree(file),
                                           /* move? */ false,
                                           FileShouldBeInPool(file)));
            mOperations.push_back(operation);
        }
    }
}

void UpdateInstaller::BuildUpdaterSpecialFileOperations() {
    // Add copy for all of the manifests we need to copy from
    // our update manifest dir to the root of the tree.
    // TODO - add test case for subset of manifests installed
    directory_iterator dir_iter(mDestRoot);
    for(; dir_iter != directory_iterator(); ++dir_iter) {
        if (dir_iter->leaf().substr(0, 9) == "MANIFEST.") {
            path src_path = mSrcManifestDir / dir_iter->leaf();
            FileOperation::Ptr operation(new FileTransfer(src_path, *dir_iter));
            mOperations.push_back(operation);
        }
    }

    // Add a copy for our updated release.spec
    FileOperation::Ptr 
        operation(new FileTransfer(mSrcRoot / "Updates/release.spec", 
                                   mDestRoot / "release.spec"));
    mOperations.push_back(operation);
}

void UpdateInstaller::BuildDirectoryCleanupFileOperations() {
    // Don't touch any files that we know about, in the existing files or
    // update files lists.  Those files will be dealt with by other portions
    // of the updater.
    FileSet::LowercaseFilenameMap 
        known_files(mExistingFiles.LowercaseFilenameEntryMap());
    known_files.insert(mUpdateFiles.LowercaseFilenameEntryMap().begin(), 
                       mUpdateFiles.LowercaseFilenameEntryMap().end());

    // The set of directories that we should have after we finish updating.
    // We should delete any extra directories that aren't in this set that
    // are in our script dirs, after having deleted any extra files within
    // them.  Note that unlike above, we are only interested in the
    // directories that appear in the update manifest, as those that were
    // in the base manifest will not be affected by any other portion
    // of the update.
    // NOTE - it is important for these to be lowercase, and for us to
    // downcase the paths on disk that we pass in to compare, because
    // this set contains names only from the update manifest, which may
    // differ in case from the files on disk.
    FilenameSet files;
    BOOST_FOREACH(FilenameEntryPair file, 
                  mUpdateFiles.LowercaseFilenameEntryMap()) {
        files.insert(file.first);
    }

    DirectoryNameMap directories_to_keep(DirectoriesForFiles(files));

    // The directories that we would like to clean up.
    std::vector<path> dirs;
    dirs.push_back(mDestRoot / "scripts");
    dirs.push_back(mDestRoot / "collects");
    dirs.push_back(mDestRoot / "engine/win32/collects");
    dirs.push_back(mDestRoot / "engine/win32/plt");

    BOOST_FOREACH(path dir, dirs) {
        BuildCleanupRecursive(dir, known_files, directories_to_keep);
        if (!mIsPossible) return;
    }
}

void UpdateInstaller::BuildCaseRenameFileOperations() {
    FilenameSet existing_files, update_files;
    BOOST_FOREACH(FileSet::Entry file, mExistingFiles.Entries()) {
        existing_files.insert(file.path());
    }
    BOOST_FOREACH(FileSet::Entry file, mUpdateFiles.Entries()) {
        update_files.insert(file.path());
    }
    // Calculate maps from lowercase filenames to filenames with case,
    // for both the update and existing directories.
    DirectoryNameMap existing_dirs(DirectoriesForFiles(existing_files));
    DirectoryNameMap update_dirs(DirectoriesForFiles(update_files));

    // For every directory we expect to have after the update
    BOOST_FOREACH(DirectoryNameMap::value_type update_dir, update_dirs) {
        DirectoryNameMap::const_iterator existing_dir_iter;
        existing_dir_iter = existing_dirs.find(update_dir.first);
        // If we don't have a directory on disk that matches case insensitively,
        // we're all set.
        if (existing_dir_iter == existing_dirs.end()) continue;
        // If we do, but it mateches case sensitively as well, we're all set.
        if (existing_dir_iter->second == update_dir.second) continue;
        // Otherwise, add a case rename operation from the old name to the new.
        FileOperation::Ptr 
            operation(new CaseRename(mDestRoot / existing_dir_iter->second,
                                     mDestRoot / update_dir.second));
        mOperations.push_back(operation);
    }
}

bool UpdateInstaller::FileShouldBeInPool(const FileSet::Entry &e) {
    bool found_in_copies = 
        mFilesToCopyFromTreeToPool.Digests().find(e.digest()) 
        != mFilesToCopyFromTreeToPool.Digests().end();
    bool found_in_moves = 
        mFilesToMoveFromTreeToPool.Digests().find(e.digest())
        != mFilesToMoveFromTreeToPool.Digests().end();
    return !found_in_copies && !found_in_moves;
}


//=========================================================================
//  UpdateInstaller installation
//=========================================================================

void UpdateInstaller::Run() {
    LockDestinationDirectory();
    InstallTool::Run();
    UnlockDestinationDirectory();
}

/// Lock our destination directory.  This function actually contains a race
/// condition, because we don't make use of automatic create/open
/// operations.  But in practice, this is a sufficient level of robustness
/// for dealing with the failure modes we've observed so far.  For the
/// moment, we see this lock more as an advisory lock to improve the UI
/// than a correctness lock.
void UpdateInstaller::LockDestinationDirectory() {
    // If we have an existing lock, complain.
    path lock(mDestRoot / LOCK_NAME);
    if (exists(lock))
        throw std::exception("Destination directory locked by "
                             "previous update.");

    // Create the new lock file.  Race condition: If we go to sleep right
    // here, somebody else could create a lock file in between our check
    // above and the code below.
    FILE *f = fopen(lock.native_file_string().c_str(), "w");
    if (!f)
        throw std::exception("Can't create lock file.");
    fclose(f);
}

/// Unlock the destination directory.
void UpdateInstaller::UnlockDestinationDirectory() {
    path lock(mDestRoot / LOCK_NAME);
    if (!exists(lock))
        throw std::exception("Destination directory was unlocked "
                             "unexpectedly.");
    remove(lock);
}


//=========================================================================
//  UpdateInstaller utilities
//=========================================================================

/// The full path to where a file is supposed to be located in the pool,
/// based on its digest.
path UpdateInstaller::PathInPool(const FileSet::Entry &e) {
    return PathInPool(e.digest());
}

/// The full path to a file located in the pool, based on the digest passed
/// in.
path UpdateInstaller::PathInPool(const std::string &s) {
    return mSrcRoot / "Updates/pool" / s;
}


