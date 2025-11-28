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

/* Additional DC functions */
HDC WINAPI CreateCompatibleDC(HDC hdc);
BOOL WINAPI DeleteDC(HDC hdc);
HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight);
HBITMAP WINAPI CreateBitmap(int nWidth, int nHeight, UINT cPlanes, UINT cBitsPerPel, const void* lpvBits);
HBITMAP WINAPI CreateDIBSection(HDC hdc, const void* lpbmi, UINT iUsage, void** ppvBits, HANDLE hSection, DWORD dwOffset);
int WINAPI GetObjectA(HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject);
int WINAPI GetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, void* lpbi, UINT uUsage);
int WINAPI SetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, const void* lpvBits, const void* lpbmi, UINT fuColorUse);

/* Extended Blt functions */
BOOL WINAPI StretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                       HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop);
int WINAPI SetStretchBltMode(HDC hdc, int iStretchMode);

/* Text drawing - Note: DrawTextA is declared in user32.h */
BOOL WINAPI TextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cchString);
BOOL WINAPI GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int c, LPSIZE lpSize);

/* Region functions */
HRGN WINAPI CreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
HRGN WINAPI CreateEllipticRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect);
int WINAPI CombineRgn(HRGN hrgnDest, HRGN hrgnSrc1, HRGN hrgnSrc2, int fnCombineMode);
BOOL WINAPI PtInRegion(HRGN hrgn, int X, int Y);
int WINAPI SelectClipRgn(HDC hdc, HRGN hrgn);
int WINAPI GetClipRgn(HDC hdc, HRGN hrgn);

/* Brush functions */
HBRUSH WINAPI CreatePatternBrush(HBITMAP hbmp);
HBRUSH WINAPI CreateHatchBrush(int fnStyle, COLORREF clrref);

/* Path functions */
BOOL WINAPI BeginPath(HDC hdc);
BOOL WINAPI EndPath(HDC hdc);
BOOL WINAPI StrokePath(HDC hdc);
BOOL WINAPI FillPath(HDC hdc);
BOOL WINAPI StrokeAndFillPath(HDC hdc);

/* Arc and curve functions */
BOOL WINAPI Arc(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
                int nXStartArc, int nYStartArc, int nXEndArc, int nYEndArc);
BOOL WINAPI Pie(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
                int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2);
BOOL WINAPI Chord(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
                  int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2);
BOOL WINAPI PolyBezier(HDC hdc, const POINT* lppt, DWORD cPoints);
BOOL WINAPI Polygon(HDC hdc, const POINT* lpPoints, int nCount);
BOOL WINAPI Polyline(HDC hdc, const POINT* lppt, int cPoints);

/* Color and palette functions */
HPALETTE WINAPI CreatePalette(const void* lplgpl);
HPALETTE WINAPI SelectPalette(HDC hdc, HPALETTE hpal, BOOL bForceBackground);
UINT WINAPI RealizePalette(HDC hdc);
COLORREF WINAPI GetNearestColor(HDC hdc, COLORREF crColor);

/* Save/restore DC state */
int WINAPI SaveDC(HDC hdc);
BOOL WINAPI RestoreDC(HDC hdc, int nSavedDC);

/* Mapping mode functions */
int WINAPI SetMapMode(HDC hdc, int fnMapMode);
int WINAPI GetMapMode(HDC hdc);
BOOL WINAPI SetWindowOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint);
BOOL WINAPI SetViewportOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint);

/* Pen functions */
HPEN WINAPI CreatePenIndirect(const void* lplgpn);
HPEN WINAPI ExtCreatePen(DWORD dwPenStyle, DWORD dwWidth, const void* lplb, DWORD dwStyleCount, const DWORD* lpStyle);

/* GDI object functions */
DWORD WINAPI GetObjectType(HGDIOBJ h);
HGDIOBJ WINAPI GetCurrentObject(HDC hdc, UINT uObjectType);

/* Region combination modes */
#define RGN_AND  1
#define RGN_OR   2
#define RGN_XOR  3
#define RGN_DIFF 4
#define RGN_COPY 5

/* StretchBlt modes */
#define BLACKONWHITE 1
#define WHITEONBLACK 2
#define COLORONCOLOR 3
#define HALFTONE     4
#define STRETCH_ANDSCANS    1
#define STRETCH_ORSCANS     2
#define STRETCH_DELETESCANS 3
#define STRETCH_HALFTONE    4

/* GDI object types */
#define OBJ_PEN         1
#define OBJ_BRUSH       2
#define OBJ_DC          3
#define OBJ_METADC      4
#define OBJ_PAL         5
#define OBJ_FONT        6
#define OBJ_BITMAP      7
#define OBJ_REGION      8
#define OBJ_METAFILE    9
#define OBJ_MEMDC       10
#define OBJ_EXTPEN      11
#define OBJ_ENHMETADC   12
#define OBJ_ENHMETAFILE 13

/* DIB color usage */
#define DIB_RGB_COLORS  0
#define DIB_PAL_COLORS  1

/* Draw text format flags */
#define DT_LEFT         0x00000000
#define DT_TOP          0x00000000
#define DT_CENTER       0x00000001
#define DT_RIGHT        0x00000002
#define DT_VCENTER      0x00000004
#define DT_BOTTOM       0x00000008
#define DT_WORDBREAK    0x00000010
#define DT_SINGLELINE   0x00000020
#define DT_NOCLIP       0x00000100
#define DT_CALCRECT     0x00000400
#define DT_NOPREFIX     0x00000800

/* Hatch styles */
#define HS_HORIZONTAL   0
#define HS_VERTICAL     1
#define HS_FDIAGONAL    2
#define HS_BDIAGONAL    3
#define HS_CROSS        4
#define HS_DIAGCROSS    5

/* Mapping modes */
#define MM_TEXT         1
#define MM_LOMETRIC     2
#define MM_HIMETRIC     3
#define MM_LOENGLISH    4
#define MM_HIENGLISH    5
#define MM_TWIPS        6
#define MM_ISOTROPIC    7
#define MM_ANISOTROPIC  8

/* Pen styles - extended */
#define PS_ENDCAP_ROUND   0x00000000
#define PS_ENDCAP_SQUARE  0x00000100
#define PS_ENDCAP_FLAT    0x00000200
#define PS_JOIN_ROUND     0x00000000
#define PS_JOIN_BEVEL     0x00001000
#define PS_JOIN_MITER     0x00002000
#define PS_GEOMETRIC      0x00010000
#define PS_COSMETIC       0x00000000

/* BITMAPINFOHEADER structure */
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG  biWidth;
    LONG  biHeight;
    WORD  biPlanes;
    WORD  biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG  biXPelsPerMeter;
    LONG  biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

/* RGBQUAD structure */
typedef struct tagRGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
} RGBQUAD;

/* BITMAPINFO structure */
typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO, *LPBITMAPINFO;

/* BITMAP structure */
typedef struct tagBITMAP {
    LONG bmType;
    LONG bmWidth;
    LONG bmHeight;
    LONG bmWidthBytes;
    WORD bmPlanes;
    WORD bmBitsPixel;
    LPVOID bmBits;
} BITMAP, *LPBITMAP;

/* LOGPEN structure */
typedef struct tagLOGPEN {
    UINT lopnStyle;
    POINT lopnWidth;
    COLORREF lopnColor;
} LOGPEN, *LPLOGPEN;

/* LOGBRUSH structure */
typedef struct tagLOGBRUSH {
    UINT lbStyle;
    COLORREF lbColor;
    ULONG lbHatch;
} LOGBRUSH, *LPLOGBRUSH;

#endif /* GDI32_H */
