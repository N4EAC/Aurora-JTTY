Unicode True
!include "MUI2.nsh"
Name "Aurora JTTY 1.1"
OutFile "..\build\windows-output\Aurora-JTTY-1.1-windows-x64-setup.exe"
InstallDir "$LOCALAPPDATA\Programs\Aurora JTTY"
RequestExecutionLevel user
Icon "..\native\Assets\Aurora-JTTY.ico"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\COPYING"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "English"
Section
  SetOutPath "$INSTDIR"
  File /r "..\build\windows-package\*"
  CreateDirectory "$SMPROGRAMS\Aurora JTTY"
  CreateShortcut "$SMPROGRAMS\Aurora JTTY\Aurora JTTY.lnk" "$INSTDIR\bin\Aurora-JTTY.exe" "" "$INSTDIR\Aurora-JTTY.ico"
  CreateShortcut "$DESKTOP\Aurora JTTY.lnk" "$INSTDIR\bin\Aurora-JTTY.exe" "" "$INSTDIR\Aurora-JTTY.ico"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd
Section "Uninstall"
  Delete "$DESKTOP\Aurora JTTY.lnk"
  RMDir /r "$SMPROGRAMS\Aurora JTTY"
  RMDir /r "$INSTDIR"
SectionEnd
