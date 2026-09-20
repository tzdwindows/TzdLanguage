; ==============================================================================
; TzdTools Windows Installer (NSIS Script) - CPU Edition
; Native 64-bit Payload with Modern UI 2
; ==============================================================================
Unicode True

!include "MUI2.nsh"
!include "x64.nsh"

!define PRODUCT_NAME "TzdTools"
!define PRODUCT_VERSION "0.2.6"
!define PRODUCT_EDITION "CPU Edition"
!define PRODUCT_PUBLISHER "tzdwindows7"
!define PRODUCT_WEB_SITE "https://github.com/tzdwindows/TzdLanguage"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\TzdTools.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

Name "${PRODUCT_NAME} v${PRODUCT_VERSION} (${PRODUCT_EDITION})"
OutFile "..\dist\TzdTools_Setup_v0.2.6_CPU.exe"
InstallDir "$PROGRAMFILES64\TzdTools"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
RequestExecutionLevel admin

; Payload is already LZMA2-compressed, so do not recompress
SetCompress off

; ------------------------------------------------------------------------------
; Interface Settings
; ------------------------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install-blue.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall-blue.ico"

; ------------------------------------------------------------------------------
; Installer Pages
; ------------------------------------------------------------------------------
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_FINISHPAGE_RUN "$INSTDIR\tzd.cmd"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!insertmacro MUI_PAGE_FINISH

; ------------------------------------------------------------------------------
; Uninstaller Pages
; ------------------------------------------------------------------------------
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; ------------------------------------------------------------------------------
; Languages
; ------------------------------------------------------------------------------
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "English"

; ------------------------------------------------------------------------------
; Installation Section
; ------------------------------------------------------------------------------
Section "MainSection" SEC01
    SetOutPath "$INSTDIR"
    SetOverwrite on

    ; Extract 7-Zip decompression tools to temporary plugins directory
    InitPluginsDir
    SetOutPath "$PLUGINSDIR"
    File "C:\Program Files\7-Zip\7z.exe"
    File "C:\Program Files\7-Zip\7z.dll"
    File "payload_cpu.7z"

    ; Extract complete TzdTools runtime & CPU dependencies into $INSTDIR
    DetailPrint "Extracting TzdTools CPU runtime and LibTorch dependencies (~310 MB)..."
    nsExec::ExecToLog '"$PLUGINSDIR\7z.exe" x "$PLUGINSDIR\payload_cpu.7z" -o"$INSTDIR" -y'
    Pop $0
    ${If} $0 != 0
        MessageBox MB_ICONSTOP|MB_OK "Failed to extract runtime files. Error code: $0"
        Abort
    ${EndIf}

    ; Clean up temporary payload to free disk space
    Delete "$PLUGINSDIR\payload_cpu.7z"
    Delete "$PLUGINSDIR\7z.exe"
    Delete "$PLUGINSDIR\7z.dll"

    SetOutPath "$INSTDIR"

    ; Create Shortcuts
    SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\TzdTools"
    CreateShortCut "$SMPROGRAMS\TzdTools\TzdTools CLI.lnk" "$SYSDIR\cmd.exe" '/K "title TzdTools CLI && cd /D %USERPROFILE% && echo TzdLang environment ready. Type tzd --help for options."' "$INSTDIR\TzdTools.exe" 0
    CreateShortCut "$SMPROGRAMS\TzdTools\Uninstall TzdTools.lnk" "$INSTDIR\uninst.exe" "" "$INSTDIR\uninst.exe" 0
    CreateShortCut "$DESKTOP\TzdTools CLI.lnk" "$SYSDIR\cmd.exe" '/K "title TzdTools CLI && cd /D %USERPROFILE% && echo TzdLang environment ready. Type tzd --help for options."' "$INSTDIR\TzdTools.exe" 0

    ; Write Uninstaller
    WriteUninstaller "$INSTDIR\uninst.exe"

    ; Write Registry Keys for Add/Remove Programs
    WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\TzdTools.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME} (TzdLang Runtime & Compiler - CPU Edition)"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\TzdTools.exe"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
    WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegDWORD ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "EstimatedSize" 320000

    ; Add to System PATH
    DetailPrint "Configuring system PATH environment variable..."
    ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    Push "$0"
    Push "$INSTDIR"
    Call AddToPathStr
    Pop $1
    WriteRegExpandStr HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path" "$1"
    SendMessage 0xFFFF 0x001A 0 "STR:Environment" /TIMEOUT=5000
SectionEnd

; ------------------------------------------------------------------------------
; PATH String Helper
; ------------------------------------------------------------------------------
Function AddToPathStr
    Exch $1 ; Dir to add
    Exch
    Exch $0 ; Current PATH
    Push $2
    Push $3

    ; Check if already in PATH
    StrCpy $2 ";$0;"
    StrCpy $3 ";$1;"
    Push $2
    Push $3
    Call StrStr
    Pop $3
    StrCmp $3 "" not_found
    ; Already in path
    StrCpy $1 $0
    Goto done

not_found:
    StrCpy $1 "$0;$1"

done:
    Pop $3
    Pop $2
    Pop $0
    Exch $1
FunctionEnd

Function StrStr
    Exch $R1 ; needle
    Exch
    Exch $R2 ; haystack
    Push $R3
    Push $R4
    Push $R5
    Push $R6
    StrLen $R3 $R1
    StrLen $R4 $R2
    StrCpy $R5 0
loop:
    StrCpy $R6 $R2 $R3 $R5
    StrCmp $R6 $R1 found
    IntOp $R5 $R5 + 1
    IntCmp $R5 $R4 done done loop
found:
    StrCpy $R1 $R2 "" $R5
    Goto exit
done:
    StrCpy $R1 ""
exit:
    Pop $R6
    Pop $R5
    Pop $R4
    Pop $R3
    Pop $R2
    Exch $R1
FunctionEnd

; ------------------------------------------------------------------------------
; Uninstaller Section
; ------------------------------------------------------------------------------
Section Uninstall
    SetShellVarContext all

    ; Remove Shortcuts
    Delete "$DESKTOP\TzdTools CLI.lnk"
    Delete "$SMPROGRAMS\TzdTools\TzdTools CLI.lnk"
    Delete "$SMPROGRAMS\TzdTools\Uninstall TzdTools.lnk"
    RMDir "$SMPROGRAMS\TzdTools"

    ; Remove Files
    RMDir /r "$INSTDIR\stdlib"
    Delete "$INSTDIR\*.*"
    RMDir "$INSTDIR"

    ; Remove Registry Keys
    DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"

    ; Remove from System PATH
    ReadRegStr $0 HKLM "SYSTEM\CurrentControlSet\Control\Session Manager\Environment" "Path"
    SendMessage 0xFFFF 0x001A 0 "STR:Environment" /TIMEOUT=5000
    SetAutoClose true
SectionEnd
