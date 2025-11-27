/**
 * Aurora OS - Windows API Compatibility Layer
 * 
 * Main header for Windows API emulation
 */

#ifndef WINAPI_H
#define WINAPI_H

#include <stdint.h>
#include <stddef.h>

/* Windows API calling convention */
#define WINAPI __attribute__((stdcall))

/* Common Windows types */
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint16_t USHORT;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef unsigned int UINT;
typedef int INT;
typedef int16_t SHORT;
typedef int BOOL;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef unsigned char BOOLEAN;
typedef uint16_t WCHAR;
typedef int WPARAM;
typedef long LPARAM;
typedef long LRESULT;
typedef uint16_t ATOM;
typedef size_t SIZE_T;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef uint16_t* LPWSTR;
typedef const uint16_t* LPCWSTR;
typedef LONG* LPLONG;
typedef DWORD* LPDWORD;
typedef WORD* LPWORD;
typedef int* LPBOOL;
typedef SIZE_T* PSIZE_T;
typedef ULONG* PULONG;
typedef DWORD COLORREF;

typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HINSTANCE;
typedef void* HWND;
typedef void* HDC;
typedef void* HKEY;
typedef void* HGLOBAL;
typedef void* HLOCAL;
typedef void* HGDIOBJ;
typedef void* HPEN;
typedef void* HBRUSH;
typedef void* HFONT;
typedef void* HBITMAP;
typedef void* HICON;
typedef void* HCURSOR;
typedef void* HPALETTE;
typedef void* HRGN;
typedef void* FARPROC;
typedef HANDLE* PHANDLE;

/* Boolean values */
#define TRUE  1
#define FALSE 0

/* NULL pointer */
#ifndef NULL
#define NULL ((void*)0)
#endif

/* Invalid handle value */
#define INVALID_HANDLE_VALUE ((HANDLE)-1)

/* Standard handles */
#define STD_INPUT_HANDLE  ((DWORD)-10)
#define STD_OUTPUT_HANDLE ((DWORD)-11)
#define STD_ERROR_HANDLE  ((DWORD)-12)

/* File access modes */
#define GENERIC_READ    0x80000000
#define GENERIC_WRITE   0x40000000
#define GENERIC_EXECUTE 0x20000000
#define GENERIC_ALL     0x10000000

/* File share modes */
#define FILE_SHARE_READ   0x00000001
#define FILE_SHARE_WRITE  0x00000002
#define FILE_SHARE_DELETE 0x00000004

/* File creation disposition */
#define CREATE_NEW        1
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define OPEN_ALWAYS       4
#define TRUNCATE_EXISTING 5

/* File attributes */
#define FILE_ATTRIBUTE_READONLY  0x00000001
#define FILE_ATTRIBUTE_HIDDEN    0x00000002
#define FILE_ATTRIBUTE_SYSTEM    0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE   0x00000020
#define FILE_ATTRIBUTE_NORMAL    0x00000080

/* Memory allocation flags */
#define MEM_COMMIT      0x00001000
#define MEM_RESERVE     0x00002000
#define MEM_RELEASE     0x00008000
#define PAGE_NOACCESS   0x00000001
#define PAGE_READONLY   0x00000002
#define PAGE_READWRITE  0x00000004
#define PAGE_EXECUTE    0x00000010

/* Error codes */
#define ERROR_SUCCESS           0
#define ERROR_INVALID_FUNCTION  1
#define ERROR_FILE_NOT_FOUND    2
#define ERROR_PATH_NOT_FOUND    3
#define ERROR_TOO_MANY_OPEN_FILES 4
#define ERROR_ACCESS_DENIED     5
#define ERROR_INVALID_HANDLE    6
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_INVALID_DRIVE     15
#define ERROR_NO_MORE_FILES     18
#define ERROR_WRITE_PROTECT     19
#define ERROR_NOT_READY         21
#define ERROR_SEEK              25
#define ERROR_WRITE_FAULT       29
#define ERROR_READ_FAULT        30
#define ERROR_GEN_FAILURE       31
#define ERROR_SHARING_VIOLATION 32
#define ERROR_LOCK_VIOLATION    33
#define ERROR_HANDLE_EOF        38
#define ERROR_FILE_EXISTS       80
#define ERROR_INVALID_PARAMETER 87
#define ERROR_BROKEN_PIPE       109
#define ERROR_DISK_FULL         112
#define ERROR_CALL_NOT_IMPLEMENTED 120
#define ERROR_INSUFFICIENT_BUFFER 122
#define ERROR_INVALID_NAME      123
#define ERROR_MOD_NOT_FOUND     126
#define ERROR_PROC_NOT_FOUND    127
#define ERROR_ALREADY_EXISTS    183
#define ERROR_ENVVAR_NOT_FOUND  203
#define ERROR_MORE_DATA         234
#define ERROR_NO_MORE_ITEMS     259
#define ERROR_DIRECTORY         267
#define ERROR_IO_PENDING        997
#define ERROR_NOACCESS          998
#define ERROR_INVALID_FLAGS     1004

/* Heap allocation flags */
#define HEAP_NO_SERIALIZE       0x00000001
#define HEAP_ZERO_MEMORY        0x00000008
#define HEAP_REALLOC_IN_PLACE_ONLY 0x00000010
#define HEAP_GENERATE_EXCEPTIONS 0x00000004

/* GlobalAlloc/LocalAlloc flags */
#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_ZEROINIT       0x0040
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)
#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define LMEM_FIXED          0x0000
#define LMEM_MOVEABLE       0x0002
#define LMEM_ZEROINIT       0x0040
#define LPTR                (LMEM_FIXED | LMEM_ZEROINIT)

/* File pointer move methods */
#define FILE_BEGIN          0
#define FILE_CURRENT        1
#define FILE_END            2

/* Process creation flags */
#define CREATE_SUSPENDED        0x00000004
#define CREATE_NEW_CONSOLE      0x00000010
#define NORMAL_PRIORITY_CLASS   0x00000020
#define IDLE_PRIORITY_CLASS     0x00000040
#define HIGH_PRIORITY_CLASS     0x00000080
#define REALTIME_PRIORITY_CLASS 0x00000100

/* Thread priority levels */
#define THREAD_PRIORITY_LOWEST        (-2)
#define THREAD_PRIORITY_BELOW_NORMAL  (-1)
#define THREAD_PRIORITY_NORMAL        0
#define THREAD_PRIORITY_ABOVE_NORMAL  1
#define THREAD_PRIORITY_HIGHEST       2

/* Wait return values */
#define WAIT_OBJECT_0       0x00000000
#define WAIT_ABANDONED      0x00000080
#define WAIT_TIMEOUT        0x00000102
#define WAIT_FAILED         0xFFFFFFFF
#define INFINITE            0xFFFFFFFF

/* GetFileType return values */
#define FILE_TYPE_UNKNOWN   0x0000
#define FILE_TYPE_DISK      0x0001
#define FILE_TYPE_CHAR      0x0002
#define FILE_TYPE_PIPE      0x0003
#define FILE_TYPE_REMOTE    0x8000

/* Console mode flags */
#define ENABLE_PROCESSED_INPUT  0x0001
#define ENABLE_LINE_INPUT       0x0002
#define ENABLE_ECHO_INPUT       0x0004
#define ENABLE_PROCESSED_OUTPUT 0x0001
#define ENABLE_WRAP_AT_EOL_OUTPUT 0x0002

/* Event flags */
#define EVENT_MODIFY_STATE  0x0002
#define EVENT_ALL_ACCESS    0x001F0003

/* Maximum path length */
#define MAX_PATH            260

/* System info processor architecture values */
#define PROCESSOR_ARCHITECTURE_INTEL   0
#define PROCESSOR_ARCHITECTURE_AMD64   9
#define PROCESSOR_ARCHITECTURE_ARM     5
#define PROCESSOR_ARCHITECTURE_ARM64   12
#define PROCESSOR_ARCHITECTURE_UNKNOWN 0xFFFF

/* Version info platform IDs */
#define VER_PLATFORM_WIN32_NT 2

/* Additional pointer types */
typedef BYTE* LPBYTE;
typedef int* LPINT;

/* Handle type for heap */
typedef HANDLE HHEAP;

/* SECURITY_ATTRIBUTES structure */
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

/* OVERLAPPED structure for async I/O */
typedef struct _OVERLAPPED {
    DWORD Internal;
    DWORD InternalHigh;
    DWORD Offset;
    DWORD OffsetHigh;
    HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

/* SYSTEM_INFO structure */
typedef struct _SYSTEM_INFO {
    WORD wProcessorArchitecture;
    WORD wReserved;
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

/* OSVERSIONINFO structure */
typedef struct _OSVERSIONINFOA {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    CHAR szCSDVersion[128];
} OSVERSIONINFOA, *LPOSVERSIONINFOA;

/* MEMORYSTATUS structure */
typedef struct _MEMORYSTATUS {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORD dwTotalPhys;
    DWORD dwAvailPhys;
    DWORD dwTotalPageFile;
    DWORD dwAvailPageFile;
    DWORD dwTotalVirtual;
    DWORD dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

/* FILETIME structure */
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *LPFILETIME;

/* SYSTEMTIME structure */
typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

/* BY_HANDLE_FILE_INFORMATION structure */
typedef struct _BY_HANDLE_FILE_INFORMATION {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD dwVolumeSerialNumber;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD nNumberOfLinks;
    DWORD nFileIndexHigh;
    DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

/* WIN32_FIND_DATAA structure */
typedef struct _WIN32_FIND_DATAA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    CHAR cFileName[MAX_PATH];
    CHAR cAlternateFileName[14];
} WIN32_FIND_DATAA, *LPWIN32_FIND_DATAA;

/* STARTUPINFOA structure */
typedef struct _STARTUPINFOA {
    DWORD cb;
    LPSTR lpReserved;
    LPSTR lpDesktop;
    LPSTR lpTitle;
    DWORD dwX;
    DWORD dwY;
    DWORD dwXSize;
    DWORD dwYSize;
    DWORD dwXCountChars;
    DWORD dwYCountChars;
    DWORD dwFillAttribute;
    DWORD dwFlags;
    WORD wShowWindow;
    WORD cbReserved2;
    LPBYTE lpReserved2;
    HANDLE hStdInput;
    HANDLE hStdOutput;
    HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

/* PROCESS_INFORMATION structure */
typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;

/* CRITICAL_SECTION structure (simplified) */
typedef struct _CRITICAL_SECTION {
    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;
    HANDLE LockSemaphore;
    DWORD SpinCount;
} CRITICAL_SECTION, *LPCRITICAL_SECTION;

/**
 * Initialize Windows API compatibility layer
 */
void winapi_init(void);

/**
 * Get last error code
 */
DWORD winapi_get_last_error(void);

/**
 * Set last error code
 */
void winapi_set_last_error(DWORD error_code);

#endif /* WINAPI_H */
