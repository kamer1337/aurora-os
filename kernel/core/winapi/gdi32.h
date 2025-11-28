/**
 * Aurora OS - GDI32.dll API Compatibility Layer
 * 
 * Stub implementations of Windows GDI32.dll functions
 */

#ifndef GDI32_H
#define GDI32_H

#include "winapi.h"
#include "user32.h"

/* Stock objects */
#define WHITE_BRUSH         0
#define LTGRAY_BRUSH        1
#define GRAY_BRUSH          2
#define DKGRAY_BRUSH        3
#define BLACK_BRUSH         4
#define NULL_BRUSH          5
#define WHITE_PEN           6
#define BLACK_PEN           7
#define NULL_PEN            8
#define OEM_FIXED_FONT      10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT       12
#define SYSTEM_FONT         13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16
#define DEFAULT_GUI_FONT    17

/* Pen styles */
#define PS_SOLID            0
#define PS_DASH             1
#define PS_DOT              2
#define PS_DASHDOT          3
#define PS_DASHDOTDOT       4
#define PS_NULL             5
#define PS_INSIDEFRAME      6

/* Brush styles */
#define BS_SOLID            0
#define BS_NULL             1
#define BS_HOLLOW           BS_NULL
#define BS_HATCHED          2
#define BS_PATTERN          3

/* Background modes */
#define TRANSPARENT         1
#define OPAQUE              2

/* Text alignment */
#define TA_LEFT             0
#define TA_RIGHT            2
#define TA_CENTER           6
#define TA_TOP              0
#define TA_BOTTOM           8
#define TA_BASELINE         24

/* Raster operations */
#define SRCCOPY             0x00CC0020
#define SRCPAINT            0x00EE0086
#define SRCAND              0x008800C6
#define SRCINVERT           0x00660046
#define SRCERASE            0x00440328
#define NOTSRCCOPY          0x00330008
#define NOTSRCERASE         0x001100A6
#define MERGECOPY           0x00C000CA
#define MERGEPAINT          0x00BB0226
#define PATCOPY             0x00F00021
#define PATPAINT            0x00FB0A09
#define PATINVERT           0x005A0049
#define DSTINVERT           0x00550009
#define BLACKNESS           0x00000042
#define WHITENESS           0x00FF0062

/* LOGFONT structure */
typedef struct tagLOGFONTA {
    LONG lfHeight;
    LONG lfWidth;
    LONG lfEscapement;
    LONG lfOrientation;
    LONG lfWeight;
    BYTE lfItalic;
    BYTE lfUnderline;
    BYTE lfStrikeOut;
    BYTE lfCharSet;
    BYTE lfOutPrecision;
    BYTE lfClipPrecision;
    BYTE lfQuality;
    BYTE lfPitchAndFamily;
    char lfFaceName[32];
} LOGFONTA, *PLOGFONTA, *LPLOGFONTA;

/* TEXTMETRIC structure */
typedef struct tagTEXTMETRICA {
    LONG tmHeight;
    LONG tmAscent;
    LONG tmDescent;
    LONG tmInternalLeading;
    LONG tmExternalLeading;
    LONG tmAveCharWidth;
    LONG tmMaxCharWidth;
    LONG tmWeight;
    LONG tmOverhang;
    LONG tmDigitizedAspectX;
    LONG tmDigitizedAspectY;
    BYTE tmFirstChar;
    BYTE tmLastChar;
    BYTE tmDefaultChar;
    BYTE tmBreakChar;
    BYTE tmItalic;
    BYTE tmUnderlined;
    BYTE tmStruckOut;
    BYTE tmPitchAndFamily;
    BYTE tmCharSet;
} TEXTMETRICA, *PTEXTMETRICA, *LPTEXTMETRICA;

/* SIZE structure */
typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE, *PSIZE, *LPSIZE;

#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb))>>8))
#define GetBValue(rgb) ((BYTE)((rgb)>>16))

/* GDI32 API functions */
void gdi32_init(void);

HGDIOBJ WINAPI GetStockObject(int fnObject);
HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ h);
BOOL WINAPI DeleteObject(HGDIOBJ ho);
HPEN WINAPI CreatePen(int fnPenStyle, int nWidth, COLORREF crColor);
HBRUSH WINAPI CreateSolidBrush(COLORREF crColor);
HFONT WINAPI CreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation,
                         int fnWeight, DWORD fdwItalic, DWORD fdwUnderline,
                         DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision,
                         DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily,
                         LPCSTR lpszFace);
HFONT WINAPI CreateFontIndirectA(const LOGFONTA* lplf);
COLORREF WINAPI SetTextColor(HDC hdc, COLORREF crColor);
COLORREF WINAPI SetBkColor(HDC hdc, COLORREF crColor);
int WINAPI SetBkMode(HDC hdc, int iBkMode);
UINT WINAPI SetTextAlign(HDC hdc, UINT fMode);
BOOL WINAPI TextOutA(HDC hdc, int nXStart, int nYStart, LPCSTR lpString, int cchString);
BOOL WINAPI ExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, const RECT* lprc,
                        LPCSTR lpString, UINT cbCount, const int* lpDx);
BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int c, LPSIZE lpSize);
BOOL WINAPI GetTextMetricsA(HDC hdc, LPTEXTMETRICA lptm);
BOOL WINAPI MoveToEx(HDC hdc, int X, int Y, LPPOINT lpPoint);
BOOL WINAPI LineTo(HDC hdc, int nXEnd, int nYEnd);
BOOL WINAPI Rectangle(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
BOOL WINAPI Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
COLORREF WINAPI SetPixel(HDC hdc, int X, int Y, COLORREF crColor);
COLORREF WINAPI GetPixel(HDC hdc, int nXPos, int nYPos);
BOOL WINAPI BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
                   HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop);
int WINAPI FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr);
int WINAPI FrameRect(HDC hDC, const RECT* lprc, HBRUSH hbr);
BOOL WINAPI PatBlt(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, DWORD dwRop);
int WINAPI GetDeviceCaps(HDC hdc, int nIndex);

#endif /* GDI32_H */
