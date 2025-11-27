/**
 * Aurora OS - Windows API Compatibility Layer
 * 
 * Main header for Windows API emulation
 */

#ifndef WINAPI_H
#define WINAPI_H

#include <stdint.h>

/* Windows API calling convention */
#define WINAPI __attribute__((stdcall))

/* Common Windows types */
typedef void* HANDLE;
typedef void* HMODULE;
typedef void* HINSTANCE;
typedef void* HWND;
typedef void* HDC;
typedef void* HKEY;
typedef void* HGLOBAL;
typedef void* HLOCAL;
typedef void* FARPROC;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef int BOOL;
typedef char CHAR;
typedef unsigned char UCHAR;
typedef uint16_t WCHAR;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef uint16_t* LPWSTR;
typedef const uint16_t* LPCWSTR;

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
#define ERROR_FILE_NOT_FOUND    2
#define ERROR_ACCESS_DENIED     5
#define ERROR_INVALID_HANDLE    6
#define ERROR_NOT_ENOUGH_MEMORY 8
#define ERROR_READ_FAULT        30
#define ERROR_WRITE_FAULT       29
#define ERROR_INVALID_PARAMETER 87

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
