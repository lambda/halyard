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

#include <stdio.h>
#include <windows.h>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include "CommandLine.h"
#include "UpdateInstaller.h"
#include "Uninstaller.h"
#include "LogFile.h"
#include "Interface.h"

using namespace boost::filesystem;
using boost::format;

void LaunchProgram(bool update_succeeded, size_t argc, const char **argv) {
    if (argc > 3) {
        // If we're running on Vista, we'll have elevated privileges, and
        // possibly by running in a different user account.  So if it seems
        // advisable, we ask the user to relaunch the program for us.
        if (!IsSafeToRelaunchAutomatically()) {
            AskUserToRelaunch(update_succeeded);
            return;
        }

        // PORTABILITY - needs to be factored to work on platforms other
        // than Windows.
        path executable(argv[3], native);
        path base(argv[2], native);
        if (!exists(executable)) {
            char *paths[] = {"Tamale.exe", "engine/win32/Halyard.exe"};
            for (int i = 0; i < 2; ++i) {
                path exe(base / paths[i]);
                if (exists(exe)) {
                    executable = exe;
                    break;
                }
            }
        }

        CommandLine cl(argc-4, const_cast<char**>(argv+4));
        if (!CommandLine::ExecAsync(executable.file_string().c_str(), cl)) {
            printf("Error: Couldn't launch external process: %s\n",
                   cl.WindowsQuotedString().c_str());
            exit(1);
        }
    }   
}

void UpdaterMain(size_t argc, const char **argv) {
    if (argc < 3) {
        printf("Usage: UpdateInstaller srcpath dstpath [command ...]\n");
        exit(1);
    } 
    
    // --cleanup indicates that we should clean out the directory in
    // preparation for an install.  This does the same thing as
    // --uninstall, except it checks first to see if it's possible to
    // clean everything out, and fails if not, while --uninstall tries
    // to do the best it can to delete everything that's safe to
    // delete.
    bool cleanup = (strcmp(argv[1], "--cleanup") == 0);
    bool uninstall = (strcmp(argv[1], "--uninstall") == 0);
    
    if (uninstall || cleanup) {
        // FIXME - temporary hack so I can continue working until I
        // figure out what to do with this log file.
        LogFile::InitLogFile(path(argv[2], native) / "temp" / "log");
    } else {
        LogFile::InitLogFile(path(argv[1], native) / "Updates" / "temp" / 
                             "log");
    }

    LogFile *logger = LogFile::GetLogFile();
        
    try {
        if (cleanup) {
            logger->Log("Checking if cleanup is possible.");
        } else if (uninstall) {
            logger->Log("Beginning uninstall");
        } else {
            logger->Log("Checking if update install is possible.");
        }
        InstallTool *tool;

        if (uninstall || cleanup) {
            path root(argv[2], native);
            tool = new Uninstaller(root, uninstall);
        } else {
            tool = new UpdateInstaller(path(argv[1], native),
                                       path(argv[2], native));
        }
        
        tool->Prepare();

        // If we're in --uninstall mode, we don't check if all of the
        // operations are possible.  We simply do our best effort to
        // delete everything we can.  We check if cleanup or install is
        // possible, though, so we don't break an existing installation
        // if it is not.
        if (!uninstall && !tool->IsPossible()) {
            if (cleanup) {
                logger->Log("Cleanup is impossible.");
            } else {
                // If we determine, safely, that updating is impossible, we should
                // just relaunch the program.
                // TODO - On Vista, this will show a dialog claiming the update
                // was successful.
                logger->Log("Update is impossible; relaunching.");
                LaunchProgram(false, argc, argv);
            }
            exit(1);
        }
        
        if (uninstall || cleanup) {
            logger->Log("Uninstall is possible; beginning uninstall.");
        } else {
            logger->Log("Update install is possible; beginning install.");
        }
        tool->Run();
    } catch (std::exception &e) {
        logger->Log(format("Error: %s") % e.what(), LogFile::FATAL);
    } catch (...) {
        logger->Log("Unknown error.", LogFile::FATAL);
    }

    if (cleanup) {
        logger->Log("Cleanup succeded.");
    } else if (uninstall) {
        logger->Log("Uninstall succeded.");
    } else {
        logger->Log("Update installed successfully. Relaunching.");
        LaunchProgram(true, argc, argv);
    }
}

