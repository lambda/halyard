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

#ifndef UpdateInstaller_H
#define UpdateInstaller_H

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include "FileSet.h"
#include "SpecFile.h"
#include "InstallTool.h"

class UpdateInstaller : public InstallTool {
public:
    UpdateInstaller(const path &src_root, const path &dst_root);
    
    virtual void Prepare();
    virtual void Run();

protected:
    // This is one of the inputs we get when we are launched, representing
    // the directory containing our update.  The directory the existing
    // program is installed in is defined in our superclass.
    path mSrcRoot;

    // This is the parsed contents of the spec file for the new version
    // of the program (the update, read out of mSrcRoot).
    SpecFile mSpecFile;

    // This is the directory that we will be reading our new (update)
    // manifessts from.  Our old manifests should just be in mDestRoot.
    path mSrcManifestDir;

    // This is the union of the manifests in our new (update)
    // manifest.  This, and the set of base files from our superclass,
    // are the inputs to our algorithm that determines all of the
    // copies and moves we will need to do.
    FileSet mUpdateFiles;

    // This is the set of files that we need to add to our new tree.
    // Some of them may be in the pool, while some may be in the tree
    // already at different locations.
    FileSet mFilesNeededForNewTree;

    // This is the set of files that we need to copy from our tree
    // to our pool.  That is, it is the set of files that have contents
    // which are needed elsewhere in the tree, but which also need to
    // remain in place.
    FileSet mFilesToCopyFromTreeToPool;
    
    // This is the set of files that we need to move from our tree
    // to our pool.  It consists of files which no longer exist with
    // the same name and contents in the tree, but which contents should
    // exist elsewhere in the tree.
    FileSet mFilesToMoveFromTreeToPool;

    // This is the set of files which we are simply going to delete from
    // our tree; we do not need these files anywhere any more.
    FileSet mFilesToDeleteFromTree;

    void CalculateFileSetsForUpdates();
    void BuildFileOperationVector();
    void BuildTreeToPoolFileOperations();
    void BuildPoolToTreeFileOperations();
    void BuildUpdaterSpecialFileOperations();
    void BuildDirectoryCleanupFileOperations();
    void BuildCaseRenameFileOperations();

    void LockDestinationDirectory();
    void UnlockDestinationDirectory();

    bool FileShouldBeInPool(const FileSet::Entry &e);
    path PathInPool(const FileSet::Entry &e);
    path PathInPool(const std::string &s);
};

#endif // UpdateInstaller_H
