; Umbra — Inno Setup installer script.
;
; Builds a per-user installer for umbra.exe + its settings-ui/ bundle,
; detecting and auto-installing the WebView2 runtime if it's missing
; (per PRD.md's stated mitigation for that risk), with the same violet
; visual identity as the app itself (see ARCHITECTURE.md's "Visual
; design direction" and installer/wizard-image.bmp / wizard-small.bmp).
;
; Build with (from this directory, on Windows, with Inno Setup 6 installed):
;   iscc umbra.iss /DMyAppVersion=0.1.0
;
; Expects umbra.exe already built (cmake --build build --config Release)
; and settings-ui/'s bundle already built (npm run build inside
; settings-ui/) — this script only packages them, it doesn't build them.
; Override MyAppExeDir/MySettingsUiDistDir below if your build output
; lives somewhere other than the defaults.

#define MyAppName "Umbra"
#define MyAppPublisher "David Ambrozio"
#define MyAppURL "https://github.com/davidambz/umbra"
#define MyAppExeName "umbra.exe"

#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif
#ifndef MyAppExeDir
  #define MyAppExeDir "..\build\Release"
#endif
#ifndef MySettingsUiDistDir
  #define MySettingsUiDistDir "..\settings-ui\dist"
#endif

[Setup]
; Generated once for this app — do not reuse for another project, and
; do not regenerate for future Umbra releases (it's what lets an update
; install over a previous version instead of side-by-side).
AppId={{6E6D2C6C-6B0C-4B0A-9C3E-2B8B9C9E9C5C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
; Per #78's silent self-update — the mechanism behind Updater::applyUpdate()'s
; "/VERYSILENT ... then the running app restarts itself" with no code in
; umbra.exe actually closing or relaunching it. Deliberately *not* paired
; with an AppMutex directive: AppMutex is a separate, older check ("if
; this mutex exists, halt and show a 'please close it manually' dialog")
; that runs independently of CloseApplications/Restart Manager and would
; show that manual dialog on every install regardless of these settings —
; it doesn't need to be told what to look for, since Restart Manager
; itself finds whatever process is holding a lock on umbra.exe (the file
; actually being replaced) without a mutex name. CloseApplications=force
; silently closes it via Restart Manager (no prompt, since /VERYSILENT
; already means no UI)...
CloseApplications=force
; ...and relaunches whatever Restart Manager closed, once installation
; finishes — this is what makes the update feel like "the app restarted
; itself" rather than silently disappearing.
RestartApplications=yes
; A background wallpaper app installed just for the current user, with
; no system-wide effect, doesn't need — and shouldn't ask for — admin
; rights.
PrivilegesRequired=lowest
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
LicenseFile=..\LICENSE
OutputDir=output
OutputBaseFilename=UmbraSetup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
WizardImageFile=wizard-image.bmp
WizardSmallImageFile=wizard-small.bmp
SetupIconFile=..\resources\app.ico
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Files]
Source: "{#MyAppExeDir}\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
; settings-ui/'s built bundle — loaded by src/ui/settings_window.cpp via
; a folder named "settings-ui" next to the executable.
Source: "{#MySettingsUiDistDir}\*"; DestDir: "{app}\settings-ui"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
const
  // The WebView2 runtime's own update-client GUID — the standard way
  // Microsoft's docs recommend detecting whether it's installed (there's
  // no dedicated "is WebView2 present" API; every install channel,
  // Evergreen or otherwise, registers under this key).
  WebView2ClientGuid = '{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}';
  WebView2BootstrapperUrl = 'https://go.microsoft.com/fwlink/p/?LinkId=2124703';
  WebView2ManualInstallUrl = 'https://developer.microsoft.com/microsoft-edge/webview2/';

// Inno Setup 6 is Unicode-only (no ANSI compiler exists), so its `string`
// type marshals as UTF-16 — must bind to the wide URLDownloadToFileW
// export, not the ANSI URLDownloadToFileA, or the URL/path pointers get
// misinterpreted and the download silently fails.
function URLDownloadToFile(pCaller: Integer; lpszURL: string; lpszFileName: string;
  dwReserved: Integer; lpfnCB: Integer): Integer;
  external 'URLDownloadToFileW@urlmon.dll stdcall';

function IsWebView2RuntimeInstalled(): Boolean;
var
  Version: String;
begin
  Result :=
    (RegQueryStringValue(HKLM64,
       'SOFTWARE\Microsoft\EdgeUpdate\Clients\' + WebView2ClientGuid, 'pv', Version)
     and (Version <> '')) or
    (RegQueryStringValue(HKLM32,
       'SOFTWARE\WOW6432Node\Microsoft\EdgeUpdate\Clients\' + WebView2ClientGuid, 'pv', Version)
     and (Version <> '')) or
    (RegQueryStringValue(HKCU,
       'SOFTWARE\Microsoft\EdgeUpdate\Clients\' + WebView2ClientGuid, 'pv', Version)
     and (Version <> ''));
end;

procedure InstallWebView2Runtime();
var
  BootstrapperPath: String;
  ResultCode: Integer;
begin
  BootstrapperPath := ExpandConstant('{tmp}\MicrosoftEdgeWebView2Setup.exe');

  WizardForm.StatusLabel.Caption :=
    'Downloading the WebView2 runtime (needed for Umbra''s settings window and web wallpapers)...';
  WizardForm.ProgressGauge.Style := npbstMarquee;

  if URLDownloadToFile(0, WebView2BootstrapperUrl, BootstrapperPath, 0, 0) <> 0 then
  begin
    if not WizardSilent then
      MsgBox('Could not download the WebView2 runtime automatically. Umbra needs it to run — ' +
             'please install it manually from ' + WebView2ManualInstallUrl +
             ' after setup finishes.', mbInformation, MB_OK);
    Exit;
  end;

  WizardForm.StatusLabel.Caption := 'Installing the WebView2 runtime...';
  // /silent suppresses the bootstrapper's own UI; it still shows nothing
  // for the (already-installed-elsewhere) case since it exits immediately.
  if not Exec(BootstrapperPath, '/silent /install', '', SW_HIDE, ewWaitUntilTerminated, ResultCode)
     or (ResultCode <> 0) then
  begin
    if not WizardSilent then
      MsgBox('The WebView2 runtime installer did not complete successfully. Umbra needs it to ' +
             'run — please install it manually from ' + WebView2ManualInstallUrl +
             ' after setup finishes.', mbInformation, MB_OK);
  end;

  WizardForm.ProgressGauge.Style := npbstNormal;
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and (not IsWebView2RuntimeInstalled()) then
    InstallWebView2Runtime();
end;
