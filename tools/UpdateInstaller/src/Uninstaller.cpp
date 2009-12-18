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
#include "FileSet.h"
#include "LogFile.h"

void Uninstaller::Prepare() {
    // We're uninstalling, so keep the files that we're currently
    // using.  These files haven't ever moved, so InnoSetup should be
    // able to deal with them (except for the log file and lock file,
    // which we need to deal with ourselves).
    SpecFile spec(mDestRoot / "release.spec");
    LowercaseFilenameMap 
        files_to_keep(CreateLowercaseFilenameMap(spec.manifest()));
    InsertIntoLowercaseFilenameMap(files_to_keep, "release.spec");
    InsertIntoLowercaseFilenameMap(files_to_keep, "UpdateInstaller.exe");
    InsertIntoLowercaseFilenameMap(files_to_keep, "temp/log");
    InsertIntoLowercaseFilenameMap(files_to_keep, LOCK_NAME);

    // Delete everything in our existing manifest.
    LowercaseFilenameMap 
        files_to_delete(CreateLowercaseFilenameMap(mExistingFiles));

    // Base our directories to keep on the set of files to keep from above.
    LowercaseFilenameMap directories_to_keep(DirectoriesForFiles(files_to_keep));
    // We don't need to delete the top-level directory; InnoSetup should do
    // that for us if we delete all of the files it doesn't know about, and
    // the top-level directory will contain some InnoSetup files that we
    // can't delete anyhow.  As our DirectoriesForFiles computation hasn't
    // included the root directory in the past, and I don't want to figure
    // out if it's safe to include that, we'll add the root directory 
    // manually.
    InsertIntoLowercaseFilenameMap(directories_to_keep, "");

    BuildCleanupRecursive(mDestRoot, files_to_delete, files_to_keep,
                          directories_to_keep);
}

void Uninstaller::Run() {
    path lock(mDestRoot / LOCK_NAME);
    // Lock the directory, unless we already have a lock (since we're
    // uninstalling, we still want to uninstall even if the directory is
    // already locked, since it's probably locked due to a failed update).
    if (!exists(lock)) {
        FILE *f = fopen(lock.native_file_string().c_str(), "w");
        // Don't worry if we can't create the lock file; it shouldn't matter
        // too much since we're just trying to blow the whole directory away,
        // and it would be better to try to continue than give up now.
        if (f)
            fclose(f);
    }

    InstallTool::Run();

    // Clean up any leftover lock files.  Without this, a failed
    // update would make it effectively impossible for ordinary users
    // to uninstall and reinstall the program.
    if (exists(lock))
        remove(lock);
}

void Uninstaller::PerformOperation(const FileOperation::Ptr op) {
    if (mBestEffort) {
        // If we're in best-effort mode (which means we're operating as
        // --uninstall and not --cleanup), we just log all errors we get
        // while performing operations and move on, instead of crashing.
        try {
            op->Perform();
        } catch (std::exception &e) {
            LogFile::GetLogFile()->Log(std::string("Error: ") + e.what());
        } catch (...) {
            LogFile::GetLogFile()->Log("Unknown error.");
        }
    } else {
        op->Perform();
    }
}
