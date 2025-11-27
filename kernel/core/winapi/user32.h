/**
 * Aurora OS - User32.dll API Compatibility Layer
 * 
 * Stub implementations of Windows User32.dll functions
 */

#ifndef USER32_H
#define USER32_H

#include "winapi.h"

/* Message box flags */
#define MB_OK               0x00000000
#define MB_OKCANCEL         0x00000001
#define MB_ABORTRETRYIGNORE 0x00000002
#define MB_YESNOCANCEL      0x00000003
#define MB_YESNO            0x00000004
#define MB_RETRYCANCEL      0x00000005
#define MB_ICONHAND         0x00000010
#define MB_ICONQUESTION     0x00000020
#define MB_ICONEXCLAMATION  0x00000030
#define MB_ICONASTERISK     0x00000040
#define MB_ICONERROR        MB_ICONHAND
#define MB_ICONWARNING      MB_ICONEXCLAMATION
#define MB_ICONINFORMATION  MB_ICONASTERISK

/* Message box return values */
#define IDOK        1
#define IDCANCEL    2
#define IDABORT     3
#define IDRETRY     4
#define IDIGNORE    5
#define IDYES       6
#define IDNO        7

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
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_COMMAND          0x0111
#define WM_TIMER            0x0113
#define WM_MOUSEMOVE        0x0200
#define WM_LBUTTONDOWN      0x0201
#define WM_LBUTTONUP        0x0202
#define WM_RBUTTONDOWN      0x0204
#define WM_RBUTTONUP        0x0205
#define WM_USER             0x0400

/* Window class styles */
#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_DBLCLKS          0x0008
#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080

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
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX)

/* ShowWindow commands */
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_RESTORE          9

/* Virtual key codes */
#define VK_BACK             0x08
#define VK_TAB              0x09
#define VK_RETURN           0x0D
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_ESCAPE           0x1B
#define VK_SPACE            0x20
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28

/* POINT structure */
typedef struct tagPOINT {
    LONG x;
    LONG y;
} POINT, *PPOINT, *LPPOINT;

/* RECT structure */
typedef struct tagRECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
} RECT, *PRECT, *LPRECT;

/* MSG structure */
typedef struct tagMSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    POINT pt;
} MSG, *PMSG, *LPMSG;

/* WNDCLASSA structure */
typedef struct tagWNDCLASSA {
    UINT style;
    void* lpfnWndProc;
    int cbClsExtra;
    int cbWndExtra;
    HANDLE hInstance;
    HANDLE hIcon;
    HANDLE hCursor;
    HANDLE hbrBackground;
    LPCSTR lpszMenuName;
    LPCSTR lpszClassName;
} WNDCLASSA, *PWNDCLASSA, *LPWNDCLASSA;

/* User32 API functions */
void user32_init(void);

int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
ATOM WINAPI RegisterClassA(const WNDCLASSA* lpWndClass);
HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName,
                            DWORD dwStyle, int x, int y, int nWidth, int nHeight,
                            HWND hWndParent, HANDLE hMenu, HANDLE hInstance, LPVOID lpParam);
BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow);
BOOL WINAPI UpdateWindow(HWND hWnd);
BOOL WINAPI DestroyWindow(HWND hWnd);
void WINAPI PostQuitMessage(int nExitCode);
BOOL WINAPI GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
BOOL WINAPI TranslateMessage(const MSG* lpMsg);
LRESULT WINAPI DispatchMessageA(const MSG* lpMsg);
LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect);
HDC WINAPI GetDC(HWND hWnd);
int WINAPI ReleaseDC(HWND hWnd, HDC hDC);
BOOL WINAPI InvalidateRect(HWND hWnd, const RECT* lpRect, BOOL bErase);
HANDLE WINAPI LoadIconA(HANDLE hInstance, LPCSTR lpIconName);
HANDLE WINAPI LoadCursorA(HANDLE hInstance, LPCSTR lpCursorName);
int WINAPI GetSystemMetrics(int nIndex);
SHORT WINAPI GetAsyncKeyState(int vKey);
SHORT WINAPI GetKeyState(int nVirtKey);
UINT WINAPI SetTimer(HWND hWnd, UINT nIDEvent, UINT uElapse, void* lpTimerFunc);
BOOL WINAPI KillTimer(HWND hWnd, UINT uIDEvent);

#endif /* USER32_H */
