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

/* ============================================ */
/* Additional DC and Bitmap functions           */
/* ============================================ */

/* DC state storage for SaveDC/RestoreDC */
typedef struct {
    COLORREF text_color;
    COLORREF bk_color;
    int bk_mode;
    UINT text_align;
    int map_mode;
    POINT window_org;
    POINT viewport_org;
    HGDIOBJ selected_pen;
    HGDIOBJ selected_brush;
    HGDIOBJ selected_font;
    HRGN clip_region;
    int stretch_blt_mode;
} DC_STATE;

#define MAX_DC_STATES 16
static DC_STATE g_dc_states[MAX_DC_STATES];
static int g_dc_state_top = 0;

/* Current DC state (simplified - single DC support) */
static DC_STATE g_current_dc_state = {
    .text_color = 0,
    .bk_color = 0xFFFFFF,
    .bk_mode = OPAQUE,
    .text_align = TA_LEFT | TA_TOP,
    .map_mode = MM_TEXT,
    .window_org = {0, 0},
    .viewport_org = {0, 0},
    .selected_pen = NULL,
    .selected_brush = NULL,
    .selected_font = NULL,
    .clip_region = NULL,
    .stretch_blt_mode = COLORONCOLOR
};

HDC WINAPI CreateCompatibleDC(HDC hdc) {
    (void)hdc;
    /* Return a fake compatible DC handle */
    static DWORD next_dc = 0xA0000;
    return (HDC)(uintptr_t)(next_dc++);
}

BOOL WINAPI DeleteDC(HDC hdc) {
    (void)hdc;
    /* Mark the DC as invalid (would free resources in real implementation) */
    return TRUE;
}

HBITMAP WINAPI CreateCompatibleBitmap(HDC hdc, int nWidth, int nHeight) {
    (void)hdc;
    if (nWidth <= 0 || nHeight <= 0) return NULL;
    /* Return a fake bitmap handle */
    static DWORD next_bitmap = 0xB0000;
    return (HBITMAP)(uintptr_t)(next_bitmap++);
}

HBITMAP WINAPI CreateBitmap(int nWidth, int nHeight, UINT cPlanes, UINT cBitsPerPel, const void* lpvBits) {
    (void)lpvBits;
    if (nWidth <= 0 || nHeight <= 0) return NULL;
    if (cPlanes == 0 || cBitsPerPel == 0) return NULL;
    /* Return a fake bitmap handle */
    static DWORD next_bitmap = 0xB1000;
    return (HBITMAP)(uintptr_t)(next_bitmap++);
}

HBITMAP WINAPI CreateDIBSection(HDC hdc, const void* lpbmi, UINT iUsage, void** ppvBits, HANDLE hSection, DWORD dwOffset) {
    (void)hdc; (void)lpbmi; (void)iUsage; (void)hSection; (void)dwOffset;
    
    if (ppvBits) {
        /* In a real implementation, this would allocate DIB memory */
        *ppvBits = NULL;  /* Would point to actual bitmap bits */
    }
    
    /* Return a fake DIB section handle */
    static DWORD next_dib = 0xB2000;
    return (HBITMAP)(uintptr_t)(next_dib++);
}

int WINAPI GetObjectA(HGDIOBJ hgdiobj, int cbBuffer, LPVOID lpvObject) {
    if (!hgdiobj) return 0;
    
    /* Determine object type and return appropriate structure */
    uintptr_t handle_val = (uintptr_t)hgdiobj;
    
    if (handle_val >= 0xB0000 && handle_val < 0xC0000) {
        /* Bitmap handle range */
        if (lpvObject && cbBuffer >= (int)sizeof(BITMAP)) {
            BITMAP* bmp = (BITMAP*)lpvObject;
            bmp->bmType = 0;
            bmp->bmWidth = 100;   /* Default size */
            bmp->bmHeight = 100;
            bmp->bmWidthBytes = 400;  /* 100 * 4 bytes per pixel */
            bmp->bmPlanes = 1;
            bmp->bmBitsPixel = 32;
            bmp->bmBits = NULL;
            return sizeof(BITMAP);
        }
        return sizeof(BITMAP);
    }
    else if (handle_val >= 0x60000 && handle_val < 0x70000) {
        /* Pen handle range */
        if (lpvObject && cbBuffer >= (int)sizeof(LOGPEN)) {
            LOGPEN* pen = (LOGPEN*)lpvObject;
            pen->lopnStyle = PS_SOLID;
            pen->lopnWidth.x = 1;
            pen->lopnWidth.y = 0;
            pen->lopnColor = 0;
            return sizeof(LOGPEN);
        }
        return sizeof(LOGPEN);
    }
    else if (handle_val >= 0x70000 && handle_val < 0x80000) {
        /* Brush handle range */
        if (lpvObject && cbBuffer >= (int)sizeof(LOGBRUSH)) {
            LOGBRUSH* brush = (LOGBRUSH*)lpvObject;
            brush->lbStyle = BS_SOLID;
            brush->lbColor = 0;
            brush->lbHatch = 0;
            return sizeof(LOGBRUSH);
        }
        return sizeof(LOGBRUSH);
    }
    else if (handle_val >= 0x80000 && handle_val < 0x90000) {
        /* Font handle range */
        if (lpvObject && cbBuffer >= (int)sizeof(LOGFONTA)) {
            LOGFONTA* font = (LOGFONTA*)lpvObject;
            font->lfHeight = 16;
            font->lfWidth = 8;
            font->lfEscapement = 0;
            font->lfOrientation = 0;
            font->lfWeight = 400;
            font->lfItalic = 0;
            font->lfUnderline = 0;
            font->lfStrikeOut = 0;
            font->lfCharSet = 0;
            font->lfOutPrecision = 0;
            font->lfClipPrecision = 0;
            font->lfQuality = 0;
            font->lfPitchAndFamily = 0;
            font->lfFaceName[0] = '\0';
            return sizeof(LOGFONTA);
        }
        return sizeof(LOGFONTA);
    }
    
    return 0;
}

int WINAPI GetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, LPVOID lpvBits, void* lpbi, UINT uUsage) {
    (void)hdc; (void)hbmp; (void)uStartScan; (void)lpvBits; (void)lpbi; (void)uUsage;
    
    /* In a real implementation, this would retrieve bitmap bits */
    return (int)cScanLines;  /* Return number of scan lines copied */
}

int WINAPI SetDIBits(HDC hdc, HBITMAP hbmp, UINT uStartScan, UINT cScanLines, const void* lpvBits, const void* lpbmi, UINT fuColorUse) {
    (void)hdc; (void)hbmp; (void)uStartScan; (void)lpvBits; (void)lpbmi; (void)fuColorUse;
    
    /* In a real implementation, this would set bitmap bits */
    return (int)cScanLines;  /* Return number of scan lines set */
}

/* ============================================ */
/* Extended Blt functions                       */
/* ============================================ */

BOOL WINAPI StretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                       HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop) {
    (void)hdcDest; (void)nXOriginDest; (void)nYOriginDest; (void)nWidthDest; (void)nHeightDest;
    (void)hdcSrc; (void)nXOriginSrc; (void)nYOriginSrc; (void)nWidthSrc; (void)nHeightSrc; (void)dwRop;
    
    /* In a real implementation, this would stretch-blit between DCs */
    return TRUE;
}

int WINAPI SetStretchBltMode(HDC hdc, int iStretchMode) {
    (void)hdc;
    int old_mode = g_current_dc_state.stretch_blt_mode;
    g_current_dc_state.stretch_blt_mode = iStretchMode;
    return old_mode;
}

/* ============================================ */
/* Text drawing functions                       */
/* ============================================ */

/* Note: DrawTextA is implemented in user32.c */

BOOL WINAPI TextOutW(HDC hdc, int nXStart, int nYStart, LPCWSTR lpString, int cchString) {
    (void)hdc; (void)nXStart; (void)nYStart;
    
    if (!lpString || cchString <= 0) return TRUE;
    
    /* Convert wide string to ANSI and output (simplified) */
    for (int i = 0; i < cchString && lpString[i]; i++) {
        char c[2] = {(char)lpString[i], '\0'};
        vga_write(c);
    }
    return TRUE;
}

BOOL WINAPI GetTextExtentPoint32W(HDC hdc, LPCWSTR lpString, int c, LPSIZE lpSize) {
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

/* ============================================ */
/* Region functions                             */
/* ============================================ */

HRGN WINAPI CreateRectRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect) {
    (void)nLeftRect; (void)nTopRect; (void)nRightRect; (void)nBottomRect;
    static DWORD next_rgn = 0xC0000;
    return (HRGN)(uintptr_t)(next_rgn++);
}

HRGN WINAPI CreateEllipticRgn(int nLeftRect, int nTopRect, int nRightRect, int nBottomRect) {
    (void)nLeftRect; (void)nTopRect; (void)nRightRect; (void)nBottomRect;
    static DWORD next_rgn = 0xC1000;
    return (HRGN)(uintptr_t)(next_rgn++);
}

int WINAPI CombineRgn(HRGN hrgnDest, HRGN hrgnSrc1, HRGN hrgnSrc2, int fnCombineMode) {
    (void)hrgnDest; (void)hrgnSrc1; (void)hrgnSrc2; (void)fnCombineMode;
    /* Returns region complexity type - SIMPLEREGION for simplicity */
    return 2;  /* SIMPLEREGION */
}

BOOL WINAPI PtInRegion(HRGN hrgn, int X, int Y) {
    (void)hrgn; (void)X; (void)Y;
    return FALSE;  /* Point not in region by default */
}

int WINAPI SelectClipRgn(HDC hdc, HRGN hrgn) {
    (void)hdc;
    g_current_dc_state.clip_region = hrgn;
    return 2;  /* SIMPLEREGION */
}

int WINAPI GetClipRgn(HDC hdc, HRGN hrgn) {
    (void)hdc; (void)hrgn;
    if (g_current_dc_state.clip_region) {
        return 1;  /* Region copied */
    }
    return 0;  /* No clipping region */
}

/* ============================================ */
/* Brush functions                              */
/* ============================================ */

HBRUSH WINAPI CreatePatternBrush(HBITMAP hbmp) {
    (void)hbmp;
    static DWORD next_brush = 0x71000;
    return (HBRUSH)(uintptr_t)(next_brush++);
}

HBRUSH WINAPI CreateHatchBrush(int fnStyle, COLORREF clrref) {
    (void)fnStyle; (void)clrref;
    static DWORD next_brush = 0x72000;
    return (HBRUSH)(uintptr_t)(next_brush++);
}

/* ============================================ */
/* Path functions                               */
/* ============================================ */

static int g_path_active = 0;

BOOL WINAPI BeginPath(HDC hdc) {
    (void)hdc;
    g_path_active = 1;
    return TRUE;
}

BOOL WINAPI EndPath(HDC hdc) {
    (void)hdc;
    /* Path is now defined and ready for use */
    return TRUE;
}

BOOL WINAPI StrokePath(HDC hdc) {
    (void)hdc;
    g_path_active = 0;
    return TRUE;
}

BOOL WINAPI FillPath(HDC hdc) {
    (void)hdc;
    g_path_active = 0;
    return TRUE;
}

BOOL WINAPI StrokeAndFillPath(HDC hdc) {
    (void)hdc;
    g_path_active = 0;
    return TRUE;
}

/* ============================================ */
/* Arc and curve functions                      */
/* ============================================ */

BOOL WINAPI Arc(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
                int nXStartArc, int nYStartArc, int nXEndArc, int nYEndArc) {
    (void)hdc; (void)nLeftRect; (void)nTopRect; (void)nRightRect; (void)nBottomRect;
    (void)nXStartArc; (void)nYStartArc; (void)nXEndArc; (void)nYEndArc;
    return TRUE;
}

BOOL WINAPI Pie(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
                int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2) {
    (void)hdc; (void)nLeftRect; (void)nTopRect; (void)nRightRect; (void)nBottomRect;
    (void)nXRadial1; (void)nYRadial1; (void)nXRadial2; (void)nYRadial2;
    return TRUE;
}

BOOL WINAPI Chord(HDC hdc, int nLeftRect, int nTopRect, int nRightRect, int nBottomRect,
                  int nXRadial1, int nYRadial1, int nXRadial2, int nYRadial2) {
    (void)hdc; (void)nLeftRect; (void)nTopRect; (void)nRightRect; (void)nBottomRect;
    (void)nXRadial1; (void)nYRadial1; (void)nXRadial2; (void)nYRadial2;
    return TRUE;
}

BOOL WINAPI PolyBezier(HDC hdc, const POINT* lppt, DWORD cPoints) {
    (void)hdc; (void)lppt; (void)cPoints;
    return TRUE;
}

BOOL WINAPI Polygon(HDC hdc, const POINT* lpPoints, int nCount) {
    (void)hdc; (void)lpPoints; (void)nCount;
    return TRUE;
}

BOOL WINAPI Polyline(HDC hdc, const POINT* lppt, int cPoints) {
    (void)hdc; (void)lppt; (void)cPoints;
    return TRUE;
}

/* ============================================ */
/* Color and palette functions                  */
/* ============================================ */

HPALETTE WINAPI CreatePalette(const void* lplgpl) {
    (void)lplgpl;
    static DWORD next_palette = 0xD0000;
    return (HPALETTE)(uintptr_t)(next_palette++);
}

HPALETTE WINAPI SelectPalette(HDC hdc, HPALETTE hpal, BOOL bForceBackground) {
    (void)hdc; (void)bForceBackground;
    static HPALETTE prev_palette = NULL;
    HPALETTE old = prev_palette;
    prev_palette = hpal;
    return old;
}

UINT WINAPI RealizePalette(HDC hdc) {
    (void)hdc;
    return 0;  /* Number of palette entries realized */
}

COLORREF WINAPI GetNearestColor(HDC hdc, COLORREF crColor) {
    (void)hdc;
    return crColor;  /* Return the same color (no palette mapping) */
}

/* ============================================ */
/* Save/restore DC state                        */
/* ============================================ */

int WINAPI SaveDC(HDC hdc) {
    (void)hdc;
    
    if (g_dc_state_top >= MAX_DC_STATES) {
        return 0;  /* Stack full */
    }
    
    g_dc_states[g_dc_state_top] = g_current_dc_state;
    g_dc_state_top++;
    
    return g_dc_state_top;  /* Return saved state index */
}

BOOL WINAPI RestoreDC(HDC hdc, int nSavedDC) {
    (void)hdc;
    
    if (nSavedDC == -1) {
        /* Restore the most recently saved state */
        if (g_dc_state_top <= 0) return FALSE;
        g_dc_state_top--;
        g_current_dc_state = g_dc_states[g_dc_state_top];
        return TRUE;
    }
    
    if (nSavedDC <= 0 || nSavedDC > g_dc_state_top) {
        return FALSE;
    }
    
    /* Restore to specific state and pop all states above it */
    g_current_dc_state = g_dc_states[nSavedDC - 1];
    g_dc_state_top = nSavedDC - 1;
    return TRUE;
}

/* ============================================ */
/* Mapping mode functions                       */
/* ============================================ */

int WINAPI SetMapMode(HDC hdc, int fnMapMode) {
    (void)hdc;
    int old_mode = g_current_dc_state.map_mode;
    g_current_dc_state.map_mode = fnMapMode;
    return old_mode;
}

int WINAPI GetMapMode(HDC hdc) {
    (void)hdc;
    return g_current_dc_state.map_mode;
}

BOOL WINAPI SetWindowOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint) {
    (void)hdc;
    
    if (lpPoint) {
        lpPoint->x = g_current_dc_state.window_org.x;
        lpPoint->y = g_current_dc_state.window_org.y;
    }
    
    g_current_dc_state.window_org.x = X;
    g_current_dc_state.window_org.y = Y;
    return TRUE;
}

BOOL WINAPI SetViewportOrgEx(HDC hdc, int X, int Y, LPPOINT lpPoint) {
    (void)hdc;
    
    if (lpPoint) {
        lpPoint->x = g_current_dc_state.viewport_org.x;
        lpPoint->y = g_current_dc_state.viewport_org.y;
    }
    
    g_current_dc_state.viewport_org.x = X;
    g_current_dc_state.viewport_org.y = Y;
    return TRUE;
}

/* ============================================ */
/* Pen functions                                */
/* ============================================ */

HPEN WINAPI CreatePenIndirect(const void* lplgpn) {
    (void)lplgpn;
    static DWORD next_pen = 0x61000;
    return (HPEN)(uintptr_t)(next_pen++);
}

HPEN WINAPI ExtCreatePen(DWORD dwPenStyle, DWORD dwWidth, const void* lplb, DWORD dwStyleCount, const DWORD* lpStyle) {
    (void)dwPenStyle; (void)dwWidth; (void)lplb; (void)dwStyleCount; (void)lpStyle;
    static DWORD next_pen = 0x62000;
    return (HPEN)(uintptr_t)(next_pen++);
}

/* ============================================ */
/* GDI object functions                         */
/* ============================================ */

DWORD WINAPI GetObjectType(HGDIOBJ h) {
    if (!h) return 0;
    
    uintptr_t handle_val = (uintptr_t)h;
    
    /* Determine object type based on handle range */
    if (handle_val >= 0x60000 && handle_val < 0x63000) return OBJ_PEN;
    if (handle_val >= 0x70000 && handle_val < 0x73000) return OBJ_BRUSH;
    if (handle_val >= 0x80000 && handle_val < 0x90000) return OBJ_FONT;
    if (handle_val >= 0xA0000 && handle_val < 0xA1000) return OBJ_DC;
    if (handle_val >= 0xB0000 && handle_val < 0xC0000) return OBJ_BITMAP;
    if (handle_val >= 0xC0000 && handle_val < 0xD0000) return OBJ_REGION;
    if (handle_val >= 0xD0000 && handle_val < 0xE0000) return OBJ_PAL;
    
    return 0;  /* Unknown type */
}

HGDIOBJ WINAPI GetCurrentObject(HDC hdc, UINT uObjectType) {
    (void)hdc;
    
    switch (uObjectType) {
        case OBJ_PEN:    return g_current_dc_state.selected_pen;
        case OBJ_BRUSH:  return g_current_dc_state.selected_brush;
        case OBJ_FONT:   return g_current_dc_state.selected_font;
        default:         return NULL;
    }
}
