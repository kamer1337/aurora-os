/**
 * Aurora OS - User32.dll API Compatibility Layer Implementation
 * 
 * Implementation of common User32.dll functions for GUI applications
 */

#include "user32.h"
#include "winapi.h"
#include "../../memory/memory.h"
#include "../../drivers/vga.h"
#include "../../gui/gui.h"
#include <stddef.h>

/* ============================================================================
 * Internal Data Structures
 * ============================================================================ */

/* Window class registry */
#define MAX_WINDOW_CLASSES 32
typedef struct {
    char class_name[64];
    WNDCLASSA wnd_class;
    int in_use;
} window_class_entry_t;

static window_class_entry_t g_window_classes[MAX_WINDOW_CLASSES];

/* Window handles */
#define MAX_WINDOWS 64
typedef struct {
    HWND hwnd;
    char class_name[64];
    char window_name[128];
    DWORD style;
    DWORD ex_style;
    int x, y, width, height;
    HWND parent;
    HINSTANCE instance;
    WNDPROC wnd_proc;
    int visible;
    int minimized;
    int maximized;
    int in_use;
} window_entry_t;

static window_entry_t g_windows[MAX_WINDOWS];
static int g_user32_initialized = 0;
static HWND g_focus_window = NULL;
static HWND g_foreground_window = NULL;
static int g_cursor_visible = 1;

/* Message queue */
#define MSG_QUEUE_SIZE 256
static MSG g_msg_queue[MSG_QUEUE_SIZE];
static int g_msg_queue_head = 0;
static int g_msg_queue_tail = 0;

/* Clipboard */
static void* g_clipboard_data = NULL;
static UINT g_clipboard_format = 0;
static int g_clipboard_open = 0;

/* String helpers */
static int u32_strlen(const char* s) {
    int len = 0;
    if (s) while (*s++) len++;
    return len;
}

static void u32_strcpy(char* dest, const char* src) {
    if (dest && src) {
        while (*src) *dest++ = *src++;
        *dest = '\0';
    }
}

static void u32_strncpy(char* dest, const char* src, int max) {
    if (!dest || !src || max <= 0) return;
    int i;
    for (i = 0; i < max - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int u32_strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return s1 ? 1 : (s2 ? -1 : 0);
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static void u32_memset(void* dest, int c, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    while (n--) *d++ = (uint8_t)c;
}

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

static void init_user32(void) {
    if (g_user32_initialized) return;
    
    u32_memset(g_window_classes, 0, sizeof(g_window_classes));
    u32_memset(g_windows, 0, sizeof(g_windows));
    u32_memset(g_msg_queue, 0, sizeof(g_msg_queue));
    
    g_user32_initialized = 1;
}

static window_class_entry_t* find_window_class(const char* name) {
    if (!name) return NULL;
    for (int i = 0; i < MAX_WINDOW_CLASSES; i++) {
        if (g_window_classes[i].in_use && 
            u32_strcmp(g_window_classes[i].class_name, name) == 0) {
            return &g_window_classes[i];
        }
    }
    return NULL;
}

static window_entry_t* find_window(HWND hwnd) {
    if (!hwnd) return NULL;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].in_use && g_windows[i].hwnd == hwnd) {
            return &g_windows[i];
        }
    }
    return NULL;
}

static window_entry_t* alloc_window(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!g_windows[i].in_use) {
            g_windows[i].in_use = 1;
            g_windows[i].hwnd = (HWND)(uintptr_t)(0x1000 + i);
            return &g_windows[i];
        }
    }
    return NULL;
}

/* ============================================================================
 * Window Management Functions
 * ============================================================================ */

void user32_init(void) {
    init_user32();
    vga_write("User32: Compatibility layer initialized\n");
}

ATOM WINAPI RegisterClassA(const WNDCLASSA* lpWndClass) {
    init_user32();
    
    if (!lpWndClass || !lpWndClass->lpszClassName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    /* Check if class already exists */
    if (find_window_class(lpWndClass->lpszClassName)) {
        winapi_set_last_error(ERROR_CLASS_ALREADY_EXISTS);
        return 0;
    }
    
    /* Find free slot */
    for (int i = 0; i < MAX_WINDOW_CLASSES; i++) {
        if (!g_window_classes[i].in_use) {
            g_window_classes[i].in_use = 1;
            u32_strncpy(g_window_classes[i].class_name, lpWndClass->lpszClassName, 64);
            g_window_classes[i].wnd_class = *lpWndClass;
            
            winapi_set_last_error(ERROR_SUCCESS);
            return (ATOM)(i + 1);
        }
    }
    
    winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
    return 0;
}

HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                            DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                            HWND hWndParent, HANDLE hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    init_user32();
    (void)hMenu;
    (void)lpParam;
    
    if (!lpClassName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    window_class_entry_t* wc = find_window_class(lpClassName);
    if (!wc) {
        winapi_set_last_error(ERROR_CANNOT_FIND_WND_CLASS);
        return NULL;
    }
    
    window_entry_t* win = alloc_window();
    if (!win) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    u32_strncpy(win->class_name, lpClassName, 64);
    if (lpWindowName) {
        u32_strncpy(win->window_name, lpWindowName, 128);
    }
    win->style = dwStyle;
    win->ex_style = dwExStyle;
    win->x = x == 0x80000000 ? 100 : x;  /* CW_USEDEFAULT */
    win->y = y == 0x80000000 ? 100 : y;
    win->width = nWidth == 0x80000000 ? 640 : nWidth;
    win->height = nHeight == 0x80000000 ? 480 : nHeight;
    win->parent = hWndParent;
    win->instance = hInstance;
    win->wnd_proc = (WNDPROC)wc->wnd_class.lpfnWndProc;
    win->visible = (dwStyle & WS_VISIBLE) ? 1 : 0;
    win->minimized = 0;
    win->maximized = 0;
    
    /* Send WM_CREATE */
    if (win->wnd_proc) {
        win->wnd_proc(win->hwnd, WM_CREATE, 0, 0);
    }
    
    vga_write("User32: Created window '");
    vga_write(lpWindowName ? lpWindowName : "(null)");
    vga_write("'\n");
    
    winapi_set_last_error(ERROR_SUCCESS);
    return win->hwnd;
}

BOOL WINAPI DestroyWindow(HWND hWnd) {
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    
    /* Send WM_DESTROY */
    if (win->wnd_proc) {
        win->wnd_proc(win->hwnd, WM_DESTROY, 0, 0);
    }
    
    /* Clear focus if this was the focus window */
    if (g_focus_window == hWnd) {
        g_focus_window = NULL;
    }
    if (g_foreground_window == hWnd) {
        g_foreground_window = NULL;
    }
    
    u32_memset(win, 0, sizeof(window_entry_t));
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow) {
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    
    int was_visible = win->visible;
    
    switch (nCmdShow) {
        case SW_HIDE:
            win->visible = 0;
            break;
        case SW_SHOWNORMAL:
        case SW_SHOW:
        case SW_RESTORE:
            win->visible = 1;
            win->minimized = 0;
            win->maximized = 0;
            break;
        case SW_SHOWMINIMIZED:
        case SW_MINIMIZE:
            win->visible = 1;
            win->minimized = 1;
            break;
        case SW_SHOWMAXIMIZED:  /* SW_MAXIMIZE is same value */
            win->visible = 1;
            win->maximized = 1;
            break;
        default:
            win->visible = 1;
            break;
    }
    
    /* Send WM_SHOWWINDOW */
    if (win->wnd_proc) {
        win->wnd_proc(win->hwnd, WM_SHOWWINDOW, win->visible, 0);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return was_visible;
}

BOOL WINAPI UpdateWindow(HWND hWnd) {
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    
    /* Send WM_PAINT */
    if (win->wnd_proc) {
        win->wnd_proc(win->hwnd, WM_PAINT, 0, 0);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint) {
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    
    win->x = X;
    win->y = Y;
    win->width = nWidth;
    win->height = nHeight;
    
    /* Send WM_MOVE and WM_SIZE */
    if (win->wnd_proc) {
        win->wnd_proc(win->hwnd, WM_MOVE, 0, (Y << 16) | (X & 0xFFFF));
        win->wnd_proc(win->hwnd, WM_SIZE, 0, (nHeight << 16) | (nWidth & 0xFFFF));
    }
    
    if (bRepaint) {
        UpdateWindow(hWnd);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) {
    (void)hWndInsertAfter;
    (void)uFlags;
    return MoveWindow(hWnd, X, Y, cx, cy, TRUE);
}

BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect) {
    if (!lpRect) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    
    lpRect->left = win->x;
    lpRect->top = win->y;
    lpRect->right = win->x + win->width;
    lpRect->bottom = win->y + win->height;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect) {
    if (!lpRect) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    
    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = win->width;
    lpRect->bottom = win->height;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HWND WINAPI GetDesktopWindow(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return (HWND)(uintptr_t)0xFFFF;  /* Pseudo-handle for desktop */
}

HWND WINAPI GetForegroundWindow(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return g_foreground_window;
}

BOOL WINAPI SetForegroundWindow(HWND hWnd) {
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return FALSE;
    }
    
    g_foreground_window = hWnd;
    
    /* Also set focus */
    SetFocus(hWnd);
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HWND WINAPI GetFocus(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return g_focus_window;
}

HWND WINAPI SetFocus(HWND hWnd) {
    HWND old_focus = g_focus_window;
    
    if (hWnd) {
        window_entry_t* win = find_window(hWnd);
        if (!win) {
            winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
            return NULL;
        }
    }
    
    /* Send WM_KILLFOCUS to old window */
    if (old_focus) {
        window_entry_t* old_win = find_window(old_focus);
        if (old_win && old_win->wnd_proc) {
            old_win->wnd_proc(old_focus, WM_KILLFOCUS, (DWORD)(uintptr_t)hWnd, 0);
        }
    }
    
    g_focus_window = hWnd;
    
    /* Send WM_SETFOCUS to new window */
    if (hWnd) {
        window_entry_t* win = find_window(hWnd);
        if (win && win->wnd_proc) {
            win->wnd_proc(hWnd, WM_SETFOCUS, (DWORD)(uintptr_t)old_focus, 0);
        }
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return old_focus;
}

BOOL WINAPI IsWindow(HWND hWnd) {
    return find_window(hWnd) != NULL;
}

BOOL WINAPI IsWindowVisible(HWND hWnd) {
    window_entry_t* win = find_window(hWnd);
    return win ? win->visible : FALSE;
}

BOOL WINAPI IsIconic(HWND hWnd) {
    window_entry_t* win = find_window(hWnd);
    return win ? win->minimized : FALSE;
}

BOOL WINAPI IsZoomed(HWND hWnd) {
    window_entry_t* win = find_window(hWnd);
    return win ? win->maximized : FALSE;
}

/* ============================================================================
 * Message Functions
 * ============================================================================ */

BOOL WINAPI GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    (void)hWnd;
    (void)wMsgFilterMin;
    (void)wMsgFilterMax;
    init_user32();
    
    if (!lpMsg) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Wait for message (simplified - just return WM_NULL for now) */
    while (g_msg_queue_head == g_msg_queue_tail) {
        /* Queue empty - would normally wait here */
        u32_memset(lpMsg, 0, sizeof(MSG));
        lpMsg->message = WM_NULL;
        return TRUE;
    }
    
    *lpMsg = g_msg_queue[g_msg_queue_head];
    g_msg_queue_head = (g_msg_queue_head + 1) % MSG_QUEUE_SIZE;
    
    /* Return FALSE for WM_QUIT */
    if (lpMsg->message == WM_QUIT) {
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    (void)hWnd;
    (void)wMsgFilterMin;
    (void)wMsgFilterMax;
    init_user32();
    
    if (!lpMsg) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (g_msg_queue_head == g_msg_queue_tail) {
        return FALSE;  /* No messages */
    }
    
    *lpMsg = g_msg_queue[g_msg_queue_head];
    
    if (wRemoveMsg & 0x0001) {  /* PM_REMOVE */
        g_msg_queue_head = (g_msg_queue_head + 1) % MSG_QUEUE_SIZE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

LONG WINAPI DispatchMessageA(const MSG* lpMsg) {
    if (!lpMsg) {
        return 0;
    }
    
    window_entry_t* win = find_window(lpMsg->hwnd);
    if (win && win->wnd_proc) {
        return win->wnd_proc(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
    }
    
    return 0;
}

BOOL WINAPI TranslateMessage(const MSG* lpMsg) {
    (void)lpMsg;
    /* Would translate virtual key messages to character messages */
    return TRUE;
}

BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, DWORD wParam, LONG lParam) {
    init_user32();
    
    int next_tail = (g_msg_queue_tail + 1) % MSG_QUEUE_SIZE;
    if (next_tail == g_msg_queue_head) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;  /* Queue full */
    }
    
    g_msg_queue[g_msg_queue_tail].hwnd = hWnd;
    g_msg_queue[g_msg_queue_tail].message = Msg;
    g_msg_queue[g_msg_queue_tail].wParam = wParam;
    g_msg_queue[g_msg_queue_tail].lParam = lParam;
    g_msg_queue[g_msg_queue_tail].time = 0;  /* Would use GetTickCount */
    g_msg_queue[g_msg_queue_tail].pt.x = 0;
    g_msg_queue[g_msg_queue_tail].pt.y = 0;
    
    g_msg_queue_tail = next_tail;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI SendMessageA(HWND hWnd, UINT Msg, DWORD wParam, LONG lParam) {
    window_entry_t* win = find_window(hWnd);
    if (win && win->wnd_proc) {
        return (BOOL)win->wnd_proc(hWnd, Msg, wParam, lParam);
    }
    return FALSE;
}

void WINAPI PostQuitMessage(int nExitCode) {
    PostMessageA(NULL, WM_QUIT, (DWORD)nExitCode, 0);
}

LONG WINAPI DefWindowProcA(HWND hWnd, UINT Msg, DWORD wParam, LONG lParam) {
    (void)hWnd;
    (void)wParam;
    (void)lParam;
    
    switch (Msg) {
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            return 0;
        default:
            return 0;
    }
}

/* ============================================================================
 * Painting Functions
 * ============================================================================ */

HDC WINAPI BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint) {
    if (!lpPaint) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    window_entry_t* win = find_window(hWnd);
    if (!win) {
        winapi_set_last_error(ERROR_INVALID_WINDOW_HANDLE);
        return NULL;
    }
    
    u32_memset(lpPaint, 0, sizeof(PAINTSTRUCT));
    lpPaint->hdc = (HDC)(uintptr_t)0x1;  /* Pseudo DC handle */
    lpPaint->fErase = TRUE;
    lpPaint->rcPaint.left = 0;
    lpPaint->rcPaint.top = 0;
    lpPaint->rcPaint.right = win->width;
    lpPaint->rcPaint.bottom = win->height;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return lpPaint->hdc;
}

BOOL WINAPI EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint) {
    (void)hWnd;
    (void)lpPaint;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HDC WINAPI GetDC(HWND hWnd) {
    (void)hWnd;
    winapi_set_last_error(ERROR_SUCCESS);
    return (HDC)(uintptr_t)0x1;  /* Pseudo DC handle */
}

int WINAPI ReleaseDC(HWND hWnd, HDC hDC) {
    (void)hWnd;
    (void)hDC;
    winapi_set_last_error(ERROR_SUCCESS);
    return 1;
}

BOOL WINAPI InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase) {
    (void)lpRect;
    (void)bErase;
    
    /* Post WM_PAINT to window */
    if (hWnd) {
        PostMessageA(hWnd, WM_PAINT, 0, 0);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI ValidateRect(HWND hWnd, const RECT* lpRect) {
    (void)hWnd;
    (void)lpRect;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ============================================================================
 * Drawing Functions
 * ============================================================================ */

BOOL WINAPI FillRect(HDC hDC, const RECT* lprc, HANDLE hbr) {
    (void)hDC;
    (void)lprc;
    (void)hbr;
    /* Would fill rectangle with brush color */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI DrawTextA(HDC hDC, LPCSTR lpString, int nCount, LPRECT lpRect, UINT uFormat) {
    (void)hDC;
    (void)lpString;
    (void)nCount;
    (void)lpRect;
    (void)uFormat;
    /* Would draw text to DC */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

int WINAPI SetBkMode(HDC hdc, int mode) {
    (void)hdc;
    (void)mode;
    return 1;  /* Previous mode (OPAQUE) */
}

DWORD WINAPI SetBkColor(HDC hdc, DWORD crColor) {
    (void)hdc;
    (void)crColor;
    return 0;  /* Previous color */
}

DWORD WINAPI SetTextColor(HDC hdc, DWORD crColor) {
    (void)hdc;
    (void)crColor;
    return 0;  /* Previous color */
}

/* ============================================================================
 * Dialog Functions
 * ============================================================================ */

int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    (void)hWnd;
    (void)uType;
    
    vga_write("MessageBox: ");
    vga_write(lpCaption ? lpCaption : "Message");
    vga_write(" - ");
    vga_write(lpText ? lpText : "");
    vga_write("\n");
    
    /* Return OK for now */
    winapi_set_last_error(ERROR_SUCCESS);
    return IDOK;
}

/* ============================================================================
 * System Metrics and Information
 * ============================================================================ */

int WINAPI GetSystemMetrics(int nIndex) {
    switch (nIndex) {
        case SM_CXSCREEN:
            return 1024;  /* Screen width */
        case SM_CYSCREEN:
            return 768;   /* Screen height */
        case SM_CXVSCROLL:
        case SM_CYHSCROLL:
            return 16;
        case SM_CYCAPTION:
            return 24;
        case SM_CXBORDER:
        case SM_CYBORDER:
            return 1;
        case SM_CXDLGFRAME:
        case SM_CYDLGFRAME:
            return 3;
        default:
            return 0;
    }
}

BOOL WINAPI SystemParametersInfoA(UINT uiAction, UINT uiParam, LPVOID pvParam, UINT fWinIni) {
    (void)uiAction;
    (void)uiParam;
    (void)pvParam;
    (void)fWinIni;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ============================================================================
 * Cursor Functions
 * ============================================================================ */

HANDLE WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName) {
    (void)hInstance;
    (void)lpCursorName;
    winapi_set_last_error(ERROR_SUCCESS);
    return (HANDLE)(uintptr_t)0x1;  /* Pseudo cursor handle */
}

HANDLE WINAPI SetCursor(HANDLE hCursor) {
    (void)hCursor;
    winapi_set_last_error(ERROR_SUCCESS);
    return (HANDLE)(uintptr_t)0x1;
}

BOOL WINAPI GetCursorPos(LPPOINT lpPoint) {
    if (!lpPoint) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    lpPoint->x = 0;
    lpPoint->y = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI SetCursorPos(int X, int Y) {
    (void)X;
    (void)Y;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

int WINAPI ShowCursor(BOOL bShow) {
    if (bShow) {
        g_cursor_visible++;
    } else {
        g_cursor_visible--;
    }
    return g_cursor_visible;
}

/* ============================================================================
 * Timer Functions
 * ============================================================================ */

static UINT g_next_timer_id = 1;

UINT WINAPI SetTimer(HWND hWnd, UINT nIDEvent, UINT uElapse, void* lpTimerFunc) {
    (void)hWnd;
    (void)nIDEvent;
    (void)uElapse;
    (void)lpTimerFunc;
    
    /* Would register timer with system */
    winapi_set_last_error(ERROR_SUCCESS);
    return g_next_timer_id++;
}

BOOL WINAPI KillTimer(HWND hWnd, UINT uIDEvent) {
    (void)hWnd;
    (void)uIDEvent;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ============================================================================
 * Keyboard Functions
 * ============================================================================ */

SHORT WINAPI GetKeyState(int nVirtKey) {
    (void)nVirtKey;
    return 0;  /* Key not pressed */
}

SHORT WINAPI GetAsyncKeyState(int vKey) {
    (void)vKey;
    return 0;  /* Key not pressed */
}

/* ============================================================================
 * Clipboard Functions
 * ============================================================================ */

BOOL WINAPI OpenClipboard(HWND hWndNewOwner) {
    (void)hWndNewOwner;
    
    if (g_clipboard_open) {
        winapi_set_last_error(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    
    g_clipboard_open = 1;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI CloseClipboard(void) {
    g_clipboard_open = 0;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI EmptyClipboard(void) {
    if (!g_clipboard_open) {
        winapi_set_last_error(ERROR_CLIPBOARD_NOT_OPEN);
        return FALSE;
    }
    
    if (g_clipboard_data) {
        kfree(g_clipboard_data);
        g_clipboard_data = NULL;
    }
    g_clipboard_format = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HANDLE WINAPI GetClipboardData(UINT uFormat) {
    if (!g_clipboard_open) {
        winapi_set_last_error(ERROR_CLIPBOARD_NOT_OPEN);
        return NULL;
    }
    
    if (uFormat != g_clipboard_format) {
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return g_clipboard_data;
}

HANDLE WINAPI SetClipboardData(UINT uFormat, HANDLE hMem) {
    if (!g_clipboard_open) {
        winapi_set_last_error(ERROR_CLIPBOARD_NOT_OPEN);
        return NULL;
    }
    
    g_clipboard_format = uFormat;
    g_clipboard_data = hMem;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return hMem;
}

/* ============================================================================
 * Color Functions
 * ============================================================================ */

DWORD WINAPI GetSysColor(int nIndex) {
    switch (nIndex) {
        case COLOR_WINDOW:
            return 0x00FFFFFF;  /* White */
        case COLOR_WINDOWTEXT:
            return 0x00000000;  /* Black */
        case COLOR_BTNFACE:
            return 0x00C0C0C0;  /* Light gray */
        default:
            return 0x00000000;
    }
}
