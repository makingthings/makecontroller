; Installer for MAKE Controller Helper & source

[Setup]
AppName=mchelper
AppVerName=mchelper v2.5.1
OutputBaseFilename=mchelper-v2.5.1-setup
AppPublisher=MakingThings
AppPublisherURL=http://www.makingthings.com
AppSupportURL=http://www.makingthings.com
AppUpdatesURL=http://www.makingthings.com
DefaultGroupName=MakingThings
DefaultDirName={pf}\mchelper
AllowNoIcons=yes
InfoBeforeFile=Info.rtf
OutputDir=.
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "..\..\release\mchelper.exe"; DestDir: "{app}"; Flags: replacesameversion
Source: "ReadMe.rtf"; DestDir: "{app}"; Flags: isreadme
Source: "atm6124.sys"; DestDir: "{sys}\drivers"; Flags: onlyifdoesntexist
Source: "atm6124.Inf"; DestDir: "{win}\inf"; Flags: replacesameversion
Source: "mingwm10.dll"; DestDir: "{app}"; Flags: replacesameversion
Source: "QtCore4.dll"; DestDir: "{app}"; Flags: replacesameversion
Source: "QtGui4.dll"; DestDir: "{app}"; Flags: replacesameversion
Source: "QtNetwork4.dll"; DestDir: "{app}"; Flags: replacesameversion
Source: "QtXml4.dll"; DestDir: "{app}"; Flags: replacesameversion
Source: "sam7.exe"; DestDir: "{app}"; Flags: replacesameversion
Source: "readline5.dll"; DestDir: "{app}"; Flags: replacesameversion
Source: "make_controller_kit.inf"; DestDir: "{win}\inf"; Flags: replacesameversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\mchelper"; Filename: "{app}\mchelper.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall mchelper"; Filename: "{uninstallexe}"

[Run]
Filename: "{app}\mchelper.exe"; Description: "{cm:LaunchProgram,MAKE Controller Helper}"; Flags: nowait postinstall skipifsilent

