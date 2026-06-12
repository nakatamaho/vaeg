// Minimal resource definitions for building without MFC/SDK resource headers.
#ifndef NP2RC_H
#define NP2RC_H

#ifndef VS_VERSION_INFO
#define VS_VERSION_INFO 1
#endif

#ifndef IDOK
#define IDOK 1
#endif

#ifndef IDCANCEL
#define IDCANCEL 2
#endif

#ifndef IDC_STATIC
#define IDC_STATIC (-1)
#endif

#ifndef LANG_JAPANESE
#define LANG_JAPANESE 0x11
#endif

#ifndef SUBLANG_DEFAULT
#define SUBLANG_DEFAULT 0x01
#endif

#ifndef WS_POPUP
#define WS_POPUP 0x80000000L
#endif

#ifndef WS_CAPTION
#define WS_CAPTION 0x00C00000L
#endif

#ifndef WS_SYSMENU
#define WS_SYSMENU 0x00080000L
#endif

#ifndef WS_VSCROLL
#define WS_VSCROLL 0x00200000L
#endif

#ifndef WS_GROUP
#define WS_GROUP 0x00020000L
#endif

#ifndef WS_TABSTOP
#define WS_TABSTOP 0x00010000L
#endif

#ifndef WS_EX_RIGHT
#define WS_EX_RIGHT 0x00001000L
#endif

#ifndef DS_MODALFRAME
#define DS_MODALFRAME 0x00000080L
#endif

#ifndef DS_CENTER
#define DS_CENTER 0x00000800L
#endif

#ifndef BS_AUTOCHECKBOX
#define BS_AUTOCHECKBOX 0x00000003L
#endif

#ifndef BS_AUTORADIOBUTTON
#define BS_AUTORADIOBUTTON 0x00000009L
#endif

#ifndef CBS_DROPDOWN
#define CBS_DROPDOWN 0x0002L
#endif

#ifndef CBS_DROPDOWNLIST
#define CBS_DROPDOWNLIST 0x0003L
#endif

#ifndef CBS_SORT
#define CBS_SORT 0x0100L
#endif

#ifndef ES_MULTILINE
#define ES_MULTILINE 0x0004L
#endif

#ifndef ES_AUTOHSCROLL
#define ES_AUTOHSCROLL 0x0080L
#endif

#ifndef ES_READONLY
#define ES_READONLY 0x0800L
#endif

#ifndef ES_WANTRETURN
#define ES_WANTRETURN 0x1000L
#endif

#ifndef SS_BITMAP
#define SS_BITMAP 0x0000000EL
#endif

#ifndef SS_ENHMETAFILE
#define SS_ENHMETAFILE 0x0000000FL
#endif

#ifndef SS_NOTIFY
#define SS_NOTIFY 0x00000100L
#endif

#ifndef SS_CENTERIMAGE
#define SS_CENTERIMAGE 0x00000200L
#endif

#ifndef SS_SUNKEN
#define SS_SUNKEN 0x00001000L
#endif

#ifndef TBS_BOTH
#define TBS_BOTH 0x0008L
#endif

#ifndef TBS_NOTICKS
#define TBS_NOTICKS 0x0010L
#endif

#endif
