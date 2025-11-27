/**
 * Aurora OS - GDI32.dll API Compatibility Layer Implementation
 * 
 * Stub implementations of Windows GDI32.dll functions
 */

#include "gdi32.h"
#include "winapi.h"
#include "../../memory/memory.h"
#include "../../drivers/vga.h"

/* Initialize GDI32 compatibility layer */
void gdi32_init(void) {
    vga_write("GDI32: Initialized\n");
}

HGDIOBJ WINAPI GetStockObject(int fnObject) {
    /* Return a fake handle */
    return (HGDIOBJ)(uintptr_t)(fnObject + 0x50000);
}

HGDIOBJ WINAPI SelectObject(HDC hdc, HGDIOBJ h) {
    (void)hdc;
    return h;  /* Return previous object (same as new for stub) */
}

BOOL WINAPI DeleteObject(HGDIOBJ ho) {
    (void)ho;
    return TRUE;
}

HPEN WINAPI CreatePen(int fnPenStyle, int nWidth, COLORREF crColor) {
    (void)fnPenStyle; (void)nWidth; (void)crColor;
    static DWORD next_pen = 0x60000;
    return (HPEN)(uintptr_t)(next_pen++);
}

HBRUSH WINAPI CreateSolidBrush(COLORREF crColor) {
    (void)crColor;
    static DWORD next_brush = 0x70000;
    return (HBRUSH)(uintptr_t)(next_brush++);
}

HFONT WINAPI CreateFontA(int nHeight, int nWidth, int nEscapement, int nOrientation,
                         int fnWeight, DWORD fdwItalic, DWORD fdwUnderline,
                         DWORD fdwStrikeOut, DWORD fdwCharSet, DWORD fdwOutputPrecision,
                         DWORD fdwClipPrecision, DWORD fdwQuality, DWORD fdwPitchAndFamily,
                         LPCSTR lpszFace) {
    (void)nHeight; (void)nWidth; (void)nEscapement; (void)nOrientation;
    (void)fnWeight; (void)fdwItalic; (void)fdwUnderline; (void)fdwStrikeOut;
    (void)fdwCharSet; (void)fdwOutputPrecision; (void)fdwClipPrecision;
    (void)fdwQuality; (void)fdwPitchAndFamily; (void)lpszFace;
    
    static DWORD next_font = 0x80000;
    return (HFONT)(uintptr_t)(next_font++);
}

HFONT WINAPI CreateFontIndirectA(const LOGFONTA* lplf) {
    if (!lplf) return NULL;
    return CreateFontA(lplf->lfHeight, lplf->lfWidth, lplf->lfEscapement,
                       lplf->lfOrientation, lplf->lfWeight, lplf->lfItalic,
                       lplf->lfUnderline, lplf->lfStrikeOut, lplf->lfCharSet,
                       lplf->lfOutPrecision, lplf->lfClipPrecision, lplf->lfQuality,
                       lplf->lfPitchAndFamily, lplf->lfFaceName);
}

COLORREF WINAPI SetTextColor(HDC hdc, COLORREF crColor) {
    (void)hdc;
    static COLORREF prev_color = 0;
    COLORREF old = prev_color;
    prev_color = crColor;
    return old;
}

COLORREF WINAPI SetBkColor(HDC hdc, COLORREF crColor) {
    (void)hdc;
    static COLORREF prev_bk_color = 0xFFFFFF;
    COLORREF old = prev_bk_color;
    prev_bk_color = crColor;
    return old;
}

int WINAPI SetBkMode(HDC hdc, int iBkMode) {
    (void)hdc;
    static int prev_mode = OPAQUE;
    int old = prev_mode;
    prev_mode = iBkMode;
    return old;
}

UINT WINAPI SetTextAlign(HDC hdc, UINT fMode) {
    (void)hdc;
    static UINT prev_align = TA_LEFT | TA_TOP;
    UINT old = prev_align;
    prev_align = fMode;
    return old;
}

BOOL WINAPI TextOutA(HDC hdc, int nXStart, int nYStart, LPCSTR lpString, int cchString) {
    (void)hdc; (void)nXStart; (void)nYStart;
    
    if (lpString && cchString > 0) {
        /* For debugging, output text to VGA console */
        for (int i = 0; i < cchString && lpString[i]; i++) {
            char c[2] = {lpString[i], '\0'};
            vga_write(c);
        }
    }
    return TRUE;
}

BOOL WINAPI ExtTextOutA(HDC hdc, int X, int Y, UINT fuOptions, const RECT* lprc,
                        LPCSTR lpString, UINT cbCount, const int* lpDx) {
    (void)fuOptions; (void)lprc; (void)lpDx;
    return TextOutA(hdc, X, Y, lpString, (int)cbCount);
}

BOOL WINAPI GetTextExtentPoint32A(HDC hdc, LPCSTR lpString, int c, LPSIZE lpSize) {
    (void)hdc;
    
    if (!lpSize) return FALSE;
    
    /* Estimate text size (8x16 font assumed) */
    int len = 0;
    if (lpString) {
        for (int i = 0; i < c && lpString[i]; i++) len++;
    }
    
    lpSize->cx = len * 8;
    lpSize->cy = 16;
    return TRUE;
}

BOOL WINAPI GetTextMetricsA(HDC hdc, LPTEXTMETRICA lptm) {
    (void)hdc;
    
    if (!lptm) return FALSE;
    
    /* Fill with default font metrics */
    lptm->tmHeight = 16;
    lptm->tmAscent = 13;
    lptm->tmDescent = 3;
    lptm->tmInternalLeading = 2;
    lptm->tmExternalLeading = 0;
    lptm->tmAveCharWidth = 8;
    lptm->tmMaxCharWidth = 8;
    lptm->tmWeight = 400;
    lptm->tmOverhang = 0;
    lptm->tmDigitizedAspectX = 96;
    lptm->tmDigitizedAspectY = 96;
    lptm->tmFirstChar = 32;
    lptm->tmLastChar = 255;
    lptm->tmDefaultChar = '?';
    lptm->tmBreakChar = ' ';
    lptm->tmItalic = 0;
    lptm->tmUnderlined = 0;
    lptm->tmStruckOut = 0;
    lptm->tmPitchAndFamily = 0;
    lptm->tmCharSet = 0;
    
    return TRUE;
}

BOOL WINAPI MoveToEx(HDC hdc, int X, int Y, LPPOINT lpPoint) {
    (void)hdc; (void)X; (void)Y;
    if (lpPoint) {
        lpPoint->x = 0;
        lpPoint->y = 0;
    }
    return TRUE;
}

BOOL WINAPI LineTo(HDC hdc, int nXEnd, int nYEnd) {
    (void)hdc; (void)nXEnd; (void)nYEnd;
    return TRUE;
}

BOOL WINAPI Rectangle(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect) {
    (void)hdc; (void)nLeftRect; (void)nTopRect; (void)nRightRect; (void)nBottomRect;
    return TRUE;
}

BOOL WINAPI Ellipse(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect) {
    (void)hdc; (void)nLeftRect; (void)nTopRect; (void)nRightRect; (void)nBottomRect;
    return TRUE;
}

COLORREF WINAPI SetPixel(HDC hdc, int X, int Y, COLORREF crColor) {
    (void)hdc; (void)X; (void)Y;
    return crColor;
}

COLORREF WINAPI GetPixel(HDC hdc, int nXPos, int nYPos) {
    (void)hdc; (void)nXPos; (void)nYPos;
    return 0;
}

BOOL WINAPI BitBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight,
                   HDC hdcSrc, int nXSrc, int nYSrc, DWORD dwRop) {
    (void)hdcDest; (void)nXDest; (void)nYDest; (void)nWidth; (void)nHeight;
    (void)hdcSrc; (void)nXSrc; (void)nYSrc; (void)dwRop;
    return TRUE;
}

int WINAPI FillRect(HDC hDC, const RECT* lprc, HBRUSH hbr) {
    (void)hDC; (void)lprc; (void)hbr;
    return 1;
}

int WINAPI FrameRect(HDC hDC, const RECT* lprc, HBRUSH hbr) {
    (void)hDC; (void)lprc; (void)hbr;
    return 1;
}

BOOL WINAPI PatBlt(HDC hdc, int nXLeft, int nYLeft, int nWidth, int nHeight, DWORD dwRop) {
    (void)hdc; (void)nXLeft; (void)nYLeft; (void)nWidth; (void)nHeight; (void)dwRop;
    return TRUE;
}

int WINAPI GetDeviceCaps(HDC hdc, int nIndex) {
    (void)hdc;
    
    switch (nIndex) {
        case 8:  return 800;   /* HORZRES */
        case 10: return 600;   /* VERTRES */
        case 12: return 32;    /* BITSPIXEL */
        case 88: return 96;    /* LOGPIXELSX */
        case 90: return 96;    /* LOGPIXELSY */
        default: return 0;
    }
}
