; mcbuilder_install.nsi

!ifndef VERSION
  !define VERSION '0.1.0'
!endif

!ifndef APPNAME
  !define APPNAME "mcbuilder"
!endif

SetCompressor /SOLID lzma
!include "MUI2.nsh"

!define MUI_ABORTWARNING

;--------------------------------

; The name of the installer
Name ${APPNAME}
Caption "${APPNAME} ${VERSION} Setup"

; The file to write
OutFile ${APPNAME}-${VERSION}-setup.exe

; The default installation directory
InstallDir C:\${APPNAME}

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------
; Modern UI pages
  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE "license.rtf"
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
	
	!define MUI_FINISHPAGE_NOAUTOCLOSE
    !define MUI_FINISHPAGE_RUN
    !define MUI_FINISHPAGE_RUN_NOTCHECKED
    !define MUI_FINISHPAGE_RUN_TEXT "Run mcbuilder now"
    !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"
    !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
    !define MUI_FINISHPAGE_SHOWREADME $INSTDIR\ReadMe.rtf
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_LANGUAGE "English"
  
Function LaunchLink
  SetOutPath $INSTDIR ; Set the appropriate working directory for the shortcut
  ExecShell "" "$INSTDIR\${APPNAME}.exe"
FunctionEnd

;--------------------------------

; The stuff to install
Section "" ;No components page, name is not important

  ; Create a shortcut in the start menu
  SetOutPath $INSTDIR ; Set the appropriate working directory for the shortcut
  CreateDirectory "$SMPROGRAMS\MakingThings"
  CreateShortCut "$SMPROGRAMS\MakingThings\${APPNAME}.lnk" "$INSTDIR\${APPNAME}.exe" \
    "" "$INSTDIR\${APPNAME}.exe" 2
  
  ; Create registry keys so the app shows up in the Add/Remove Program list
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\mcbuilder" \
    "DisplayName" "mcbuilder ${VERSION} by MakingThings"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\mcbuilder" \
    "UninstallString" "$INSTDIR\uninstall.exe"

  SetOutPath $WINDIR\inf ;inf files(both sam-ba and Make Controller) in the right spot
  File atm6124.Inf
  File make_controller_kit.inf
  
  SetOutPath $SYSDIR\drivers ; Atmel supplied Windows driver
  File atm6124.sys
  
  SetOutPath $INSTDIR
  File "..\..\..\bin\${APPNAME}.exe"
  File mingwm10.dll
  File QtCore4.dll
  File QtGui4.dll
  File QtNetwork4.dll
  File QtXml4.dll
  File "..\ReadMe.rtf"
  
  SetOutPath $INSTDIR\resources\templates
  File /nonfatal /r /x .svn "..\..\templates\*"
  
  SetOutPath $INSTDIR\resources\examples
  File /nonfatal /r /x .svn "..\..\examples\*"
  
  SetOutPath $INSTDIR\resources\cores\makecontroller
  File /nonfatal /r /x .svn "..\..\cores\makecontroller\*"
  
  SetOutPath $INSTDIR\resources\board_profiles
  File /nonfatal /r /x .svn "..\..\board_profiles\*"
  
  SetOutPath $INSTDIR\resources\tools
  File /nonfatal /r /x .svn /x *.zip "..\..\tools\*"
  
  SetOutPath $INSTDIR\resources\reference
  File /nonfatal /r /x .svn /x diagrams "..\..\reference\*"
  
  SetOutPath $INSTDIR\libraries
  File /nonfatal /r /x .svn "..\..\..\libraries\*"
  
  WriteUninstaller "$INSTDIR\uninstall.exe"
  
SectionEnd
  
Section "Uninstall"
  Delete "$INSTDIR\uninstall.exe"
  Delete $INSTDIR\${APPNAME}.exe
  Delete "$SMPROGRAMS\MakingThings\${APPNAME}.lnk" ; Delete the shortcut we made
  RMDir /r /REBOOTOK $INSTDIR
  RMDir $PROGRAMFILES\MakingThings ;should only delete if it's empty since /r is not specified
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\mcbuilder"
SectionEnd

  
