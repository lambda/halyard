;; We require InnoSetup 5.3.2-beta or later, as that's when PrepareToInstall
;; was introduced, which is critical for being able to install Halyard 0.6.x
;; prgrams over existing Halyard 0.4.x programs.
#if Ver < 0x05030200
  #error A more recent version of Inno Setup (>= 5.3.2) is required to compile this script
#endif

;; Set this to 1 to include various useful debugging stuff, and 0
;; to make a final build.
#define INCLUDE_DEBUGGING_SUPPORT 0

;; Set this to 1 to build a CD installer, and 0 otherwise.
#ifndef CD_INSTALLER
#define CD_INSTALLER 0
#endif

;; Set this to 0 to exclude engine PDBs for the release build
#ifndef INCLUDE_PDBS
#define INCLUDE_PDBS 1
#endif

[Files]
;; Halyard runtime files.  We use a pretty complicated exclusion pattern
;; and then add some stuff back in manually.
Source: engine\win32\*; DestDir: {app}\engine\win32; Excludes: *.pdb,.git*,.svn,*.bak,.#*,#*,*~,compiled,*.ilk,*.map,CommonTest*,qtcheck.dll,*_d.*,UpdateInstaller.exe; Flags: recursesubdirs; Components: base
#if INCLUDE_PDBS
Source: engine\win32\Halyard.pdb; DestDir: {app}\engine\win32; Components: base
#endif
Source: engine\win32\*.zo; DestDir: {app}\engine\win32; Flags: recursesubdirs touch skipifsourcedoesntexist; Components: base
Source: engine\win32\qtcheck.dll; Flags: dontcopy

;; WARNING: UpdateInstaller.exe must allways be installed in {app}, as
;; long as we want older updaters to work and to call the
;; newly-downloaded UpdateInstaller.exe.  Our updater looks for a file
;; called UpdateInstaller.exe at the top level when pulling an update
;; down, so if this is not present, we will always get the old
;; UpdateInstaller.exe that was installed originally.
Source: engine\win32\UpdateInstaller.exe; DestDir: {app}; Components: base

;; We include these files only if we're including debugging support.
#if INCLUDE_DEBUGGING_SUPPORT
Source: engine\win32\*_d.exe; DestDir: {app}\engine\win32; Components: debug
Source: engine\win32\*_d.dll; DestDir: {app}\engine\win32; Components: debug
Source: engine\win32\*_d.pdb; DestDir: {app}\engine\win32; Components: debug
Source: config\developer.prefs; DestDir: {app}\config; Components: debug
#endif

;; WARNING: If you don't include this, the crash reporter won't work.
Source: binaries\curl.exe; DestDir: {app}\binaries; Flags: skipifsourcedoesntexist; Components: base

;; WARNING: If you don't include these, the updater won't work.
;;
;; Note that gpgv and gnupg are affected by US export restrictions, as
;; described in bug 1046 and bug 1121.  Do not export these files or upload
;; them to a website without understanding the legal ramifications.
Source: binaries\gpgv.exe; DestDir: {app}\binaries; Flags: skipifsourcedoesntexist; Components: base
Source: binaries\gnupg-1.4.7.tar.bz2; DestDir: {app}\binaries; Flags: skipifsourcedoesntexist

;; This is a tricky file.  If present, it indicates that the user would
;; like to automatically update to new versions of the program.  But
;; nothing in Halyard itself actually checks this--it's up to the script.
;; Note that this file is not included in the update manifests, and can't
;; be moved to a new location without teaching UpdateInstaller.exe about
;; how to handle the move.
Source: AUTO-UPDATE; DestDir: {app}; Flags: skipifsourcedoesntexist; Tasks: autoupdate

;; TODO - Make sure halyard/test has LICENSE.txt.
Source: LICENSE.txt; DestDir: {app}; Flags: skipifsourcedoesntexist; Components: base

Source: application.halyard; DestDir: {app}; Components: base
Source: config\*; DestDir: {app}\config; Excludes: .git*,.svn,*.bak,.#*,#*,*~,developer.prefs; Flags: recursesubdirs; Components: base
Source: local\*; DestDir: {app}\local; Excludes: .git*,.svn,*.bak,.#*,#*,*~,*.psd; Flags: recursesubdirs nocompression; Components: base

Source: scripts\*; DestDir: {app}\scripts; Excludes: .git*,.svn,*.bak,.#*,#*,*~,compiled; Flags: recursesubdirs; Components: base
Source: scripts\*.zo; DestDir: {app}\scripts; Flags: recursesubdirs touch skipifsourcedoesntexist; Components: base
;; Some of our libraries have some test media that is large, and we would like
;; to avoid shipping with updates.  The excludes for _halyard\{local,streaming}\
;; media\tests prevent these from being included.
Source: collects\*; DestDir: {app}\collects; Excludes: .git*,.svn,*.bak,.#*,#*,*~,compiled,_halyard\local\media\tests,_halyard\streaming\media\tests; Flags: recursesubdirs; Components: base
Source: collects\*.zo; DestDir: {app}\collects; Flags: recursesubdirs touch skipifsourcedoesntexist; Components: base

;; All media except *.mp3 and *.mov files must be built into the
;; installer. This is because we can't stream anything but QuickTime media
;; formats.
#if CD_INSTALLER
Source: streaming\*.mov; DestDir: {app}\streaming; Flags: recursesubdirs skipifsourcedoesntexist nocompression; Components: media
Source: streaming\*.mp3; DestDir: {app}\streaming; Flags: recursesubdirs skipifsourcedoesntexist nocompression; Components: media
#else
;; The *.mp3 and *.mov files may be in a directory on the CD, or missing for a
;; web install.
Source: {src}\streaming\*; DestDir: {app}\streaming; Flags: recursesubdirs external; Components: media
#endif

;; The MANIFEST.* files are generated by our release-building
;; scripts, and should not be checked into Subversion.
Source: MANIFEST.base; DestDir: {app}; Flags: skipifsourcedoesntexist; Components: base
Source: MANIFEST.media; DestDir: {app}; Flags: skipifsourcedoesntexist; Components: media
Source: MANIFEST.debug; DestDir: {app}; Flags: skipifsourcedoesntexist; Components: debug
;; The release.spec file is also generated by the build script.
Source: release.spec; DestDir: {app}; Flags: skipifsourcedoesntexist; Components: base
Source: release.spec.sig; DestDir: {app}; Flags: skipifsourcedoesntexist; Components: base

;; QuickTime support.
#if CD_INSTALLER
Source: binaries\QuickTimeInstaller.exe; DestDir: {tmp}; Components: quicktime; Flags: deleteafterinstall
#else
Source: {src}\QuickTimeInstaller.exe; DestDir: {tmp}; Components: quicktime; Flags: deleteafterinstall external
#endif
Source: binaries\On2_VP3.qtx; DestDir: {sys}\QuickTime; Components: vp3; Flags: uninsneveruninstall skipifsourcedoesntexist
Source: binaries\vp3-3.2.2.1.tar.bz2; DestDir: {app}\binaries; Components: vp3; Flags: skipifsourcedoesntexist

[Types]
Name: regular; Description: Regular Install
[Components]
Name: base; Description: {cm:AppShortName}; Flags: fixed; Types: regular
#if CD_INSTALLER
Name: media; Description: Video Files; Flags: fixed; Types: regular
#else
Name: media; Description: Video Files; Types: regular; Check: DirCheck(ExpandConstant('{src}\streaming'))
#endif
Name: debug; Description: Debugging Support; Types: regular
Name: quicktime; Description: QuickTime; Flags: fixed disablenouninstallwarning; Types: regular; Check: NeedQuickTime; ExtraDiskSpaceRequired: 4194304
Name: vp3; Description: On2 VP3 QuickTime Codec; Flags: fixed disablenouninstallwarning; Types: regular; Check: NeedVP3
[Setup]
SourceDir=..
DefaultDirName={pf}\{cm:AppShortName}
DefaultGroupName={cm:AppShortName}
MinVersion=0,5.01.2600sp2
;;LicenseFile=LICENSE.txt
PrivilegesRequired=admin
AlwaysShowComponentsList=false
ShowLanguageDialog=yes
#if CD_INSTALLER
OutputDir=cd_installer
#else
OutputDir=.
#endif
#if CD_INSTALLER
DiskSpanning=true
ReserveBytes=31457280
DiskSliceSize=732954624
#endif
UninstallDisplayIcon={app}\local\branding\application.ico
[Run]
;; We're going to re-enable our internal QuickTime support, since Joe
;; Henderson has asked use to use the /passive flag.  See bug #3581 for the
;; details of the decision and our licensing discussion with Apple.
Filename: {tmp}\QuickTimeInstaller.exe; Parameters: /passive; Components: quicktime; StatusMsg: Installing QuickTime...
Filename: {app}\engine\win32\Halyard.exe; Parameters: """{app}"""; WorkingDir: {app}; Description: Launch {cm:AppShortName}; Flags: postinstall nowait
[Icons]
Name: {group}\{cm:AppShortName}; Filename: {app}\engine\win32\Halyard.exe; IconIndex: 0; Flags: createonlyiffileexists; Parameters: """."""; WorkingDir: {app}; IconFilename: {app}\local\branding\application.ico
Name: {group}\Halyard License; Filename: {app}\engine\win32\LICENSE.txt; Flags: createonlyiffileexists
Name: {group}\Update {cm:AppShortName}; Filename: {app}\engine\win32\Halyard.exe; Parameters: "-e ""(set! *updater-only* #t)"" ""."""; WorkingDir: {app}; Flags: createonlyiffileexists; Components: base; Tasks: not autoupdate; IconFilename: {app}\local\branding\update.ico; IconIndex: 0
Name: {commondesktop}\{cm:AppShortName}; Filename: {app}\engine\win32\Halyard.exe; IconIndex: 0; Flags: createonlyiffileexists; Parameters: """."""; WorkingDir: {app}; IconFilename: {app}\local\branding\application.ico; Tasks: desktopicon
[Tasks]
Name: desktopicon; Description: Create a shortcut to {cm:AppShortName} on the desktop; Components: base
Name: autoupdate; Description: Periodically check for updates; Components: base
[Registry]
Root: HKCU; Subkey: Software\Halyard; Flags: uninsdeletekey dontcreatekey
[UninstallRun]
Filename: {app}\UpdateInstaller.exe; Parameters: --uninstall .; WorkingDir: {app}; Flags: runminimized
[Code]
const
    // Display the currently installed versions of various components.
    // (For debugging purposes only.)
    DebugShowVersions = false;
    // Minimum QuickTime version.  See below for a URL explaining what
    // this means.
    MinimumQuickTimeVersion = $07318000;
    MinimumQuickTimeVersionString = '7.3.1';
    QuickTimeInstaller = '{src}\QuickTimeInstaller.exe';
    QuickTimeProWarning =
        'If you have installed QuickTime Pro, upgrading to QuickTime 7 ' +
        'will disable QuickTime Pro functionality. To re-enable ' +
        'QuickTime Pro functionality, you will need to purchase a ' +
        'QuickTime 7 Pro key from from Apple Computer, Inc.';
    // Minimum VP3 version.  This is a nasty, in-house hacked version
    // with minimum support for QuickTime 6; there should be a better
    // one available in the QuickTime update service.
    MinimumVP3Version = 131075;
    UninstallError =
        'Installation is impossible. You appear to be installing over an ' +
        'existing installation of {cm:AppShortName}, ' +
        'which contains files that the installer cannot ' +
        'delete. Please choose another directory to install ' +
        'into, or delete this directory and try again.';

var
    // Cached checks for QuickTime components.
    QuickTimeVersionOK, UpgradingQuickTime, VP3VersionOK: Boolean;


//-------------------------------------------------------------------------
//  Basic Checks
//-------------------------------------------------------------------------

// Never use the specified component.
function NeverUse: Boolean;
begin
    Result := false;
end;

// Check to see whether a directory exists.
function DirCheck(DirName: String): Boolean;
begin
    Result := DirExists(DirName);
end;

// Check to see whether a file exists.
function FileCheck(FileName: String): Boolean;
begin
    Result := FileExists(FileName);
end;

// Check to see whether we can create the specified ActiveX control.
function HaveActiveXControl(const ControlName: String): Boolean;
var
    Player: Variant;
begin
    Result := true;
    try
        Player := CreateOleObject(ControlName);
    except
        Result := false;
    end;
end;


//-------------------------------------------------------------------------
//  QuickTime Support
//-------------------------------------------------------------------------
//  This is more annoying than it should be, because we're not allowed to
//  include the QuickTime installer with web downloads, and we need an
//  external DLL to figure out what is already installed.

// Check to see whether we have a QuickTime installer.
function HaveQuickTimeInstaller(): Boolean;
begin
#if CD_INSTALLER
    Result := true
#else
    Result := FileCheck(ExpandConstant(QuickTimeInstaller));
#endif
end;

// Import QuickTime-related functions from qtcheck.dll.  The *Version
// functions return weirdly encoded version numbers; see
// http://developer.apple.com/technotes/tn/tn1197.html for some discussion
// of how these numbers are interpreted.
procedure QuickTimeCheckSetup();
external 'QuickTimeCheckSetup@files:qtcheck.dll stdcall';
function QuickTimeVersion(): Integer;
external 'QuickTimeVersion@files:qtcheck.dll stdcall';
function QuickTimeComponentVersion(ctype, csubtype: String): Integer;
external 'QuickTimeComponentVersion@files:qtcheck.dll stdcall';
procedure QuickTimeCheckCleanup();
external 'QuickTimeCheckCleanup@files:qtcheck.dll stdcall';

// Figure out a bunch of facts about QuickTime and cache them for later.
procedure CacheQuickTimeChecks();
var
    QTVer, VP3Ver: Integer;
begin
    // Initialize our DLL.
    QuickTimeCheckSetup();

    // Look up the versions we care about.
    QTVer := QuickTimeVersion();
    VP3Ver := QuickTimeComponentVersion('imco', 'VP31');

    // Clean up our DLL.
    QuickTimeCheckCleanup();

    // Display what we've learned.  To make sense of these numbers, you'll
    // need to convert them into hexadecimal.
    if DebugShowVersions then begin
      MsgBox('QuickTime: '+IntToStr(QTVer)+' VP3: '+IntToStr(VP3Ver),
        mbInformation, MB_OK);
    end;

    // Default our cached values to false;
    QuickTimeVersionOK := false;
    UpgradingQuickTime := false;
    VP3VersionOK := false;
    if QTVer >= MinimumQuickTimeVersion then begin
        QuickTimeVersionOK := true;
    end else if QTVer > 0 then begin
		// The version isn't new enough to be OK, but it's
		// greater than zero, which means we'll be performing
		// an upgrade.
		UpgradingQuickTime := true;
    end;
    if VP3Ver >= MinimumVP3Version then begin
        VP3VersionOK := true;
    end;
end;

// Do we need to install or update QuickTime?
function NeedQuickTime(): Boolean;
begin
    Result := not QuickTimeVersionOK;
end;

// Do we need to install or update our VP3 codec?
function NeedVP3(): Boolean;
begin
    Result := not VP3VersionOK;
end;

// Should we warn the user that they might lose a QuickTime Pro
// license?
function ShouldWarnQuickTimeProUsers(): Boolean;
begin
	Result := UpgradingQuickTime;
end;


//-------------------------------------------------------------------------
//  Installer Logic
//-------------------------------------------------------------------------

// This is called immediately after the installer starts.
function InitializeSetup(): Boolean;
begin
    // Assume, by default, that we want to run the installer.
    Result := true;

    // Load our DLL and figure out a bunch of useful information
    // about QuickTime *once*.  We'll use this information throughout
    // the install process.
    CacheQuickTimeChecks();

    // Make sure we'll be able to do something intelligent about
    // about QuickTime.
    if NeedQuickTime() and not HaveQuickTimeInstaller() then begin
        MsgBox(ExpandConstant('Please install QuickTime ' +
                              MinimumQuickTimeVersionString + ' ' +
                              'or later before installing this application.'),
               mbError, MB_OK);
        Result := false;
    end;
end;

// This procedure gets called after InitializeSetup, and gives us a chance
// to update the wizard screens displayed by the installer.
procedure InitializeWizard();
begin
    if ShouldWarnQuickTimeProUsers() then begin
        CreateOutputMsgPage(wpLicense,
                            'Important notice for QuickTime Pro users',
                            'Please read if you have purchased QuickTime Pro.',
                            QuickTimeProWarning);
    end;
end;

// This function is called to prepare for installation.  We do this by
// checking to see if the chosen destination directory already has an
// installed program.  If it does, we use the UpdateInstaller.exe to
// clean it out before installing, so that old files and directories
// won't cause problems for the newly installed program.  If
// UpdateInstaller.exe fails, we return an error message, which will
// cause the installation to fail.
function PrepareToInstall(): String;
var
    ResultCode: Integer;
begin
    Result := '';

    if FileCheck(ExpandConstant('{app}\release.spec')) then begin
        ExtractTemporaryFile('UpdateInstaller.exe');
        if Exec(ExpandConstant('{tmp}\UpdateInstaller.exe'),
                               '--cleanup "' + ExpandConstant('{app}') + '"',
                               '', SW_SHOW, ewWaitUntilTerminated,
                               ResultCode) then begin
            // The exec call succeeded.  Check the return code to see whether
            // the uninstall succeeded.
            if ResultCode <> 0 then begin
                Result := ExpandConstant(UninstallError);
            end;
        end
        else begin
            // The exec call failed.
            // TODO - Should we give a different error message indicating that
            // something went wrong with the installer? I'm not really sure
            // what failure here means; it may mean that the installer couldn't
            // extract a temporary UpdateInstaller.exe, or couldn't launch
            // it for some reason.
            Result := ExpandConstant(UninstallError);
        end;
    end;
end;

// This routine used to fail under QuickTime 6 (because the old QuickTime
// installer would leave half-installed messes behind if the user cancelled),
// but QuickTime 7 appears to much better behaved in our testing.  Still, this
// is of no use to use because we can't detect when and if the user *cancels*
// a QuickTime installation.
//procedure CheckQuickTimeInstall();
//begin
//	CacheQuickTimeChecks(); // Re-run our QuickTime checks.
//	if NeedQuickTime() then begin
//		MsgBox('Your copy of QuickTime may not be fully installed. ' +
//		       'Before running the application, please ' +
//		       'make sure QuickTime is installed.',
//		       mbError, MB_OK);
//	end;
//end;







