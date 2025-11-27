/**
 * Aurora OS - User32.dll API Compatibility Layer Implementation
 * 
 * Stub implementations of Windows User32.dll functions
 */

#include "user32.h"
#include "winapi.h"
#include "../../memory/memory.h"
#include "../../drivers/vga.h"

/* Initialize User32 compatibility layer */
void user32_init(void) {
    vga_write("User32: Initialized\n");
}

int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
    (void)hWnd;
    (void)uType;
    
    vga_write("\n=== Message Box ===\n");
    if (lpCaption) {
        vga_write("Title: ");
        vga_write(lpCaption);
        vga_write("\n");
    }
    if (lpText) {
        vga_write("Text: ");
        vga_write(lpText);
        vga_write("\n");
    }
    vga_write("==================\n");
    
    winapi_set_last_error(ERROR_SUCCESS);
    return IDOK;
}

ATOM WINAPI RegisterClassA(const WNDCLASSA* lpWndClass) {
    static ATOM next_atom = 1;
    
    if (!lpWndClass || !lpWndClass->lpszClassName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    vga_write("User32: RegisterClassA - ");
    vga_write(lpWndClass->lpszClassName);
    vga_write("\n");
    
    winapi_set_last_error(ERROR_SUCCESS);
    return next_atom++;
}

HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                            DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                            HWND hWndParent, HANDLE hMenu, HANDLE hInstance, LPVOID lpParam) {
    (void)dwExStyle; (void)dwStyle;
    (void)x; (void)y; (void)nWidth; (void)nHeight;
    (void)hWndParent; (void)hMenu; (void)hInstance; (void)lpParam;
    
    vga_write("User32: CreateWindowExA - ");
    if (lpClassName) vga_write(lpClassName);
    vga_write(": ");
    if (lpWindowName) vga_write(lpWindowName);
    vga_write("\n");
    
    /* Return a fake HWND */
    winapi_set_last_error(ERROR_SUCCESS);
    return (HWND)0x10000;
}

BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow) {
    (void)hWnd; (void)nCmdShow;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI UpdateWindow(HWND hWnd) {
    (void)hWnd;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI DestroyWindow(HWND hWnd) {
    (void)hWnd;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

void WINAPI PostQuitMessage(int nExitCode) {
    vga_write("User32: PostQuitMessage(");
    vga_write_dec(nExitCode);
    vga_write(")\n");
}

BOOL WINAPI GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
    (void)hWnd; (void)wMsgFilterMin; (void)wMsgFilterMax;
    
    if (!lpMsg) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Return WM_QUIT to end message loop */
    lpMsg->message = WM_QUIT;
    lpMsg->wParam = 0;
    lpMsg->lParam = 0;
    lpMsg->hwnd = NULL;
    lpMsg->time = 0;
    lpMsg->pt.x = 0;
    lpMsg->pt.y = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return FALSE;  /* Return FALSE for WM_QUIT */
}

BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
    (void)hWnd; (void)wMsgFilterMin; (void)wMsgFilterMax; (void)wRemoveMsg;
    
    if (lpMsg) {
        lpMsg->message = 0;
        lpMsg->wParam = 0;
        lpMsg->lParam = 0;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return FALSE;  /* No message available */
}

BOOL WINAPI TranslateMessage(const MSG* lpMsg) {
    (void)lpMsg;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

LRESULT WINAPI DispatchMessageA(const MSG* lpMsg) {
    (void)lpMsg;
    winapi_set_last_error(ERROR_SUCCESS);
    return 0;
}

LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    (void)hWnd; (void)Msg; (void)wParam; (void)lParam;
    return 0;
}

BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect) {
    (void)hWnd;
    
    if (!lpRect) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    lpRect->left = 0;
    lpRect->top = 0;
    lpRect->right = 800;
    lpRect->bottom = 600;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HDC WINAPI GetDC(HWND hWnd) {
    (void)hWnd;
    winapi_set_last_error(ERROR_SUCCESS);
    return (HDC)0x20000;
}

int WINAPI ReleaseDC(HWND hWnd, HDC hDC) {
    (void)hWnd; (void)hDC;
    winapi_set_last_error(ERROR_SUCCESS);
    return 1;
}

BOOL WINAPI InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase) {
    (void)hWnd; (void)lpRect; (void)bErase;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HANDLE WINAPI LoadIconA(HANDLE hInstance, LPCSTR lpIconName) {
    (void)hInstance; (void)lpIconName;
    winapi_set_last_error(ERROR_SUCCESS);
    return (HANDLE)0x30000;
}

HANDLE WINAPI LoadCursorA(HANDLE hInstance, LPCSTR lpCursorName) {
    (void)hInstance; (void)lpCursorName;
    winapi_set_last_error(ERROR_SUCCESS);
    return (HANDLE)0x40000;
}

int WINAPI GetSystemMetrics(int nIndex) {
    switch (nIndex) {
        case 0: return 800;   /* SM_CXSCREEN */
        case 1: return 600;   /* SM_CYSCREEN */
        default: return 0;
    }
}

SHORT WINAPI GetAsyncKeyState(int vKey) {
    (void)vKey;
    return 0;
}

SHORT WINAPI GetKeyState(int nVirtKey) {
    (void)nVirtKey;
    return 0;
}

UINT WINAPI SetTimer(HWND hWnd, UINT nIDEvent, UINT uElapse, void* lpTimerFunc) {
    (void)hWnd; (void)uElapse; (void)lpTimerFunc;
    winapi_set_last_error(ERROR_SUCCESS);
    return nIDEvent ? nIDEvent : 1;
}

BOOL WINAPI KillTimer(HWND hWnd, UINT uIDEvent) {
    (void)hWnd; (void)uIDEvent;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}
