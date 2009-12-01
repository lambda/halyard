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

#ifndef InstallTool_H
#define InstallTool_H

#include <vector>
#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>
#include "FileSet.h"
#include "SpecFile.h"

using namespace boost::filesystem;

class FileOperation {
public:
    virtual bool IsPossible() const = 0;
    virtual void Perform() const = 0;

    typedef boost::shared_ptr<FileOperation> Ptr;
    typedef std::vector<Ptr> Vector;
};

class FileDelete : public FileOperation {
public:
    FileDelete(path inFile) : file(inFile) { }
    virtual bool IsPossible() const;
    virtual void Perform() const;

protected:
    path file;
};

// Represents a copy or a move from the source to the dest, depending
// on the inMove flag passed in to the constructor.  inShouldExist
// indicates whether the file is expected to exist at the source location
// before we begin our update.
class FileTransfer : public FileOperation {
public:
    FileTransfer(const path &inSource, const path &inDest, 
                 bool inMove = false, bool inShouldExist = true) 
        : mSource(inSource), mDest(inDest), mMove(inMove),
          mShouldExist(inShouldExist) { }
    virtual bool IsPossible() const;
    virtual void Perform() const;

protected:
    path mSource, mDest;

    // Should we do this as a move?  Default is to copy.
    bool mMove;

    // True if we expect the source file to exist when checking if the
    // update is possible.  This will be false when we are depending on
    // a prior transfer to put the source file into place.
    bool mShouldExist;
};

// Represents a rename "in-place"; that is, a renaming of a file
// name to the same name in different case.  This differs from
// FileTransfer above, which deletes any file at the destination
// before running, which we can't do for what should be obvious
// reasons.
class CaseRename : public FileOperation {
public: 
    CaseRename(const path &inSource, const path &inDest) 
        : mSource(inSource), mDest(inDest) { }
    virtual bool IsPossible() const;
    virtual void Perform() const;
protected:
    path mSource, mDest;
};

// This is our superclass for all of our tools which do installation related
// tasks.  This includes our update installer, as well as our uninstaller
// that can be used by the UpdateInstaller.exe --uninstall command line to
// clean out all of the files that we control.
class InstallTool {
public:
    InstallTool(const path &dst_root);

protected:
    // This is the directory our program is installed in.
    path mDestRoot;

    // This is the union of the manifests in our installed copy of the
    // program.
    FileSet mExistingFiles;

    // The name of the lock file indicating whether we have a half-
    // updated program.
    static const char *LOCK_NAME;
};

#endif // InstallTool_H
