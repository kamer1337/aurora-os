/**
 * Aurora OS - User32.dll API Compatibility Layer
 * 
 * Implementation of common User32.dll functions for GUI applications
 */

#ifndef USER32_H
#define USER32_H

#include "winapi.h"

/* Window styles */
#define WS_OVERLAPPED       0x00000000
#define WS_POPUP            0x80000000
#define WS_CHILD            0x40000000
#define WS_MINIMIZE         0x20000000
#define WS_VISIBLE          0x10000000
#define WS_DISABLED         0x08000000
#define WS_CLIPSIBLINGS     0x04000000
#define WS_CLIPCHILDREN     0x02000000
#define WS_MAXIMIZE         0x01000000
#define WS_CAPTION          0x00C00000
#define WS_BORDER           0x00800000
#define WS_DLGFRAME         0x00400000
#define WS_VSCROLL          0x00200000
#define WS_HSCROLL          0x00100000
#define WS_SYSMENU          0x00080000
#define WS_THICKFRAME       0x00040000
#define WS_MINIMIZEBOX      0x00020000
#define WS_MAXIMIZEBOX      0x00010000
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | \
                             WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)

/* Extended window styles */
#define WS_EX_DLGMODALFRAME 0x00000001
#define WS_EX_TOPMOST       0x00000008
#define WS_EX_TRANSPARENT   0x00000020
#define WS_EX_TOOLWINDOW    0x00000080
#define WS_EX_WINDOWEDGE    0x00000100
#define WS_EX_CLIENTEDGE    0x00000200
#define WS_EX_APPWINDOW     0x00040000

/* ShowWindow commands */
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9
#define SW_SHOWDEFAULT      10

/* Message box types */
#define MB_OK               0x00000000
#define MB_OKCANCEL         0x00000001
#define MB_ABORTRETRYIGNORE 0x00000002
#define MB_YESNOCANCEL      0x00000003
#define MB_YESNO            0x00000004
#define MB_RETRYCANCEL      0x00000005
#define MB_ICONERROR        0x00000010
#define MB_ICONQUESTION     0x00000020
#define MB_ICONWARNING      0x00000030
#define MB_ICONINFORMATION  0x00000040

/* Message box return values */
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7

/* Window messages */
#define WM_NULL             0x0000
#define WM_CREATE           0x0001
#define WM_DESTROY          0x0002
#define WM_MOVE             0x0003
#define WM_SIZE             0x0005
#define WM_ACTIVATE         0x0006
#define WM_SETFOCUS         0x0007
#define WM_KILLFOCUS        0x0008
#define WM_ENABLE           0x000A
#define WM_PAINT            0x000F
#define WM_CLOSE            0x0010
#define WM_QUIT             0x0012
#define WM_SHOWWINDOW       0x0018
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN      0x0207
#define WM_MBUTTONUP        0x0208
#define WM_MOUSEWHEEL       0x020A
#define WM_TIMER            0x0113
#define WM_USER             0x0400

/* Virtual key codes */
#define VK_BACK             0x08
#define VK_TAB              0x09
#define VK_RETURN           0x0D
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_PAUSE            0x13
#define VK_CAPITAL          0x14
#define VK_ESCAPE           0x1B
#define VK_SPACE            0x20
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28
#define VK_DELETE           0x2E
#define VK_F1               0x70
#define VK_F2               0x71
#define VK_F3               0x72
#define VK_F4               0x73
#define VK_F5               0x74
#define VK_F6               0x75
#define VK_F7               0x76
#define VK_F8               0x77
#define VK_F9               0x78
#define VK_F10              0x79
#define VK_F11              0x7A
#define VK_F12              0x7B

/* Color constants */
#define COLOR_WINDOW        5
#define COLOR_WINDOWTEXT    8
#define COLOR_BTNFACE       15

/* GetSystemMetrics indices */
#define SM_CXSCREEN         0
#define SM_CYSCREEN         1
#define SM_CXVSCROLL        2
#define SM_CYHSCROLL        3
#define SM_CYCAPTION        4
#define SM_CXBORDER         5
#define SM_CYBORDER         6
#define SM_CXDLGFRAME       7
#define SM_CYDLGFRAME       8

/* RECT structure */
typedef struct _RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *LPRECT;

/* POINT structure */
typedef struct _POINT {
    LONG x;
    LONG y;
} POINT, *LPPOINT;

/* MSG structure */
typedef struct _MSG {
    HWND hwnd;
    UINT message;
    DWORD wParam;
    LONG lParam;
    DWORD time;
    POINT pt;
} MSG, *LPMSG;

/* WNDCLASSA structure */
typedef struct _WNDCLASSA {
    UINT style;
    void* lpfnWndProc;  /* WNDPROC */
    int cbClsExtra;
    int cbWndExtra;
    HINSTANCE hInstance;
    HANDLE hIcon;
    HANDLE hCursor;
    HANDLE hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASSA, *LPWNDCLASSA;

/* PAINTSTRUCT structure */
typedef struct _PAINTSTRUCT {
    HDC hdc;
    BOOL fErase;
    RECT rcPaint;
    BOOL fRestore;
    BOOL fIncUpdate;
    BYTE rgbReserved[32];
} PAINTSTRUCT, *LPPAINTSTRUCT;

/* Window procedure type */
typedef LONG (*WNDPROC)(HWND, UINT, DWORD, LONG);

/* Window management functions */
ATOM WINAPI RegisterClassA(const WNDCLASSA* lpWndClass);
HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                            DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                            HWND hWndParent, HANDLE hMenu, HINSTANCE hInstance, LPVOID lpParam);
BOOL WINAPI DestroyWindow(HWND hWnd);
BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow);
BOOL WINAPI UpdateWindow(HWND hWnd);
BOOL WINAPI MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect);
BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect);
HWND WINAPI GetDesktopWindow(void);
HWND WINAPI GetForegroundWindow(void);
BOOL WINAPI SetForegroundWindow(HWND hWnd);
HWND WINAPI GetFocus(void);
HWND WINAPI SetFocus(HWND hWnd);
BOOL WINAPI IsWindow(HWND hWnd);
BOOL WINAPI IsWindowVisible(HWND hWnd);
BOOL WINAPI IsIconic(HWND hWnd);
BOOL WINAPI IsZoomed(HWND hWnd);

/* Message functions */
BOOL WINAPI GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
LONG WINAPI DispatchMessageA(const MSG* lpMsg);
BOOL WINAPI TranslateMessage(const MSG* lpMsg);
BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, DWORD wParam, LONG lParam);
BOOL WINAPI SendMessageA(HWND hWnd, UINT Msg, DWORD wParam, LONG lParam);
void WINAPI PostQuitMessage(int nExitCode);
LONG WINAPI DefWindowProcA(HWND hWnd, UINT Msg, DWORD wParam, LONG lParam);

/* Painting functions */
HDC WINAPI BeginPaint(HWND hWnd, LPPAINTSTRUCT lpPaint);
BOOL WINAPI EndPaint(HWND hWnd, const PAINTSTRUCT* lpPaint);
HDC WINAPI GetDC(HWND hWnd);
int WINAPI ReleaseDC(HWND hWnd, HDC hDC);
BOOL WINAPI InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase);
BOOL WINAPI ValidateRect(HWND hWnd, const RECT* lpRect);

/* Drawing functions */
BOOL WINAPI FillRect(HDC hDC, const RECT* lprc, HANDLE hbr);
BOOL WINAPI DrawTextA(HDC hDC, LPCSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);
int WINAPI SetBkMode(HDC hdc, int mode);
DWORD WINAPI SetBkColor(HDC hdc, DWORD crColor);
DWORD WINAPI SetTextColor(HDC hdc, DWORD crColor);

/* Dialog functions */
int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);

/* System metrics and information */
int WINAPI GetSystemMetrics(int nIndex);
BOOL WINAPI SystemParametersInfoA(UINT uiAction, UINT uiParam, LPVOID pvParam, UINT fWinIni);

/* Cursor functions */
HANDLE WINAPI LoadCursorA(HINSTANCE hInstance, LPCSTR lpCursorName);
HANDLE WINAPI SetCursor(HANDLE hCursor);
BOOL WINAPI GetCursorPos(LPPOINT lpPoint);
BOOL WINAPI SetCursorPos(int X, int Y);
int WINAPI ShowCursor(BOOL bShow);

/* Timer functions */
UINT WINAPI SetTimer(HWND hWnd, UINT nIDEvent, UINT uElapse, void* lpTimerFunc);
BOOL WINAPI KillTimer(HWND hWnd, UINT uIDEvent);

/* Keyboard functions */
SHORT WINAPI GetKeyState(int nVirtKey);
SHORT WINAPI GetAsyncKeyState(int vKey);

/* Clipboard functions */
BOOL WINAPI OpenClipboard(HWND hWndNewOwner);
BOOL WINAPI CloseClipboard(void);
BOOL WINAPI EmptyClipboard(void);
HANDLE WINAPI GetClipboardData(UINT uFormat);
HANDLE WINAPI SetClipboardData(UINT uFormat, HANDLE hMem);

/* Color functions */
DWORD WINAPI GetSysColor(int nIndex);

/* Initialize User32.dll compatibility layer */
void user32_init(void);

#endif /* USER32_H */
