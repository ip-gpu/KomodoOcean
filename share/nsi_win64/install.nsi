Target x86-unicode
; SetCompressor lzma
; SetCompressor /SOLID zlib

RequestExecutionLevel highest
SetCompressor /SOLID lzma

; Settings 
Name "KomodoOcean (Komodo-Qt)"
OutFile "komodo-qt-install.exe"
; RequestExecutionLevel admin
InstallDir "$PROGRAMFILES64\KomodoOcean"
Icon "komodo.ico"
CRCCheck on

; Includes

; Pages
Page components 
Page directory
Page instfiles

;--------------------------------

Section "Install Komodo-Qt (GUI)" Section1
  SetOutPath $INSTDIR
  File "content\komodo-qt-win.exe"
  CreateShortCut "$DESKTOP\Komodo-Qt.lnk" "$INSTDIR\komodo-qt-win.exe"
SectionEnd

Section "Download ZCash Params" Section3
  
SetOverwrite on
CreateDirectory "$APPDATA\ZcashParams"  
DetailPrint "Checking: sprout-proving.key"
IfFileExists "$APPDATA\ZcashParams\sprout-proving.key" next_1
DetailPrint "Downloading: sprout-proving.key"
inetc::get /CAPTION "ZCash Params" /POPUP "sprout-proving.key" \
    "https://z.cash/downloads/sprout-proving.key.deprecated-sworn-elves" "$APPDATA\ZcashParams\sprout-proving.key" \
    /END
Pop $0
StrCmp $0 "OK" next_1
MessageBox MB_OK|MB_ICONEXCLAMATION "Failed to fetch ZCash params, click OK to abort installation" /SD IDOK
Abort
next_1:

DetailPrint "Checking: sprout-verifying.key"
IfFileExists "$APPDATA\ZcashParams\sprout-verifying.key" next_2
DetailPrint "Downloading: sprout-verifying.key"
inetc::get /CAPTION "ZCash Params" /POPUP "sprout-verifying.key" \
    "https://z.cash/downloads/sprout-verifying.key" "$APPDATA\ZcashParams\sprout-verifying.key" \
    /END
Pop $0
StrCmp $0 "OK" next_2
MessageBox MB_OK|MB_ICONEXCLAMATION "Failed to fetch ZCash params, click OK to abort installation" /SD IDOK
Abort
next_2:

DetailPrint "Checking: sapling-spend.params"
IfFileExists "$APPDATA\ZcashParams\sapling-spend.params" next_3
DetailPrint "Downloading: sapling-spend.params"
inetc::get /CAPTION "ZCash Params" /POPUP "sapling-spend.params" \
    "https://z.cash/downloads/sapling-spend.params" "$APPDATA\ZcashParams\sapling-spend.params" \
    /END
Pop $0
StrCmp $0 "OK" next_3
MessageBox MB_OK|MB_ICONEXCLAMATION "Failed to fetch ZCash params, click OK to abort installation" /SD IDOK
Abort
next_3:

DetailPrint "Checking: sapling-output.params"
IfFileExists "$APPDATA\ZcashParams\sapling-output.params" next_4
DetailPrint "Downloading: sapling-output.params"
inetc::get /CAPTION "ZCash Params" /POPUP "sapling-output.params" \
    "https://z.cash/downloads/sapling-output.params" "$APPDATA\ZcashParams\sapling-output.params" \
    /END
Pop $0
StrCmp $0 "OK" next_4
MessageBox MB_OK|MB_ICONEXCLAMATION "Failed to fetch ZCash params, click OK to abort installation" /SD IDOK
Abort
next_4:

DetailPrint "Checking: sprout-groth16.params"
IfFileExists "$APPDATA\ZcashParams\sprout-groth16.params" next_5
DetailPrint "Downloading: sprout-groth16.params"
inetc::get /CAPTION "ZCash Params" /POPUP "sprout-groth16.params" \
    "https://z.cash/downloads/sprout-groth16.params" "$APPDATA\ZcashParams\sprout-groth16.params" \
    /END
Pop $0
StrCmp $0 "OK" next_5
MessageBox MB_OK|MB_ICONEXCLAMATION "Failed to fetch ZCash params, click OK to abort installation" /SD IDOK
Abort
next_5:
SectionEnd

Section "Create komodo.conf" Section2
  SetOverwrite on
  CreateDirectory "$APPDATA\Komodo"  
  SetOutPath $APPDATA\Komodo
  File "content\komodo.conf"
SectionEnd