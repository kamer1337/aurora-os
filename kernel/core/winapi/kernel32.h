/**
 * Aurora OS - Kernel32.dll API Compatibility Layer
 * 
 * Implementation of common Kernel32.dll functions
 */

#ifndef KERNEL32_H
#define KERNEL32_H

#include "winapi.h"

/* Process and Thread Functions */
HANDLE WINAPI GetCurrentProcess(void);
DWORD WINAPI GetCurrentProcessId(void);
HANDLE WINAPI GetCurrentThread(void);
DWORD WINAPI GetCurrentThreadId(void);
void WINAPI ExitProcess(DWORD exit_code);
BOOL WINAPI TerminateProcess(HANDLE process, DWORD exit_code);

/* Memory Management Functions */
LPVOID WINAPI VirtualAlloc(LPVOID address, DWORD size, DWORD alloc_type, DWORD protect);
BOOL WINAPI VirtualFree(LPVOID address, DWORD size, DWORD free_type);
HGLOBAL WINAPI GlobalAlloc(DWORD flags, DWORD bytes);
HGLOBAL WINAPI GlobalFree(HGLOBAL mem);
HLOCAL WINAPI LocalAlloc(DWORD flags, DWORD bytes);
HLOCAL WINAPI LocalFree(HLOCAL mem);

/* File Management Functions */
HANDLE WINAPI CreateFileA(LPCSTR filename, DWORD access, DWORD share_mode,
                          void* security, DWORD creation, DWORD flags, HANDLE template_file);
BOOL WINAPI ReadFile(HANDLE file, LPVOID buffer, DWORD bytes_to_read,
                     DWORD* bytes_read, void* overlapped);
BOOL WINAPI WriteFile(HANDLE file, LPCVOID buffer, DWORD bytes_to_write,
                      DWORD* bytes_written, void* overlapped);
BOOL WINAPI CloseHandle(HANDLE handle);
DWORD WINAPI GetFileSize(HANDLE file, DWORD* high_size);
BOOL WINAPI DeleteFileA(LPCSTR filename);

/* Console Functions */
HANDLE WINAPI GetStdHandle(DWORD std_handle);
BOOL WINAPI WriteConsoleA(HANDLE console, LPCVOID buffer, DWORD length,
                          DWORD* written, void* reserved);

/* String Functions */
int WINAPI lstrlenA(LPCSTR string);
LPSTR WINAPI lstrcpyA(LPSTR dest, LPCSTR src);
LPSTR WINAPI lstrcatA(LPSTR dest, LPCSTR src);
int WINAPI lstrcmpA(LPCSTR str1, LPCSTR str2);

/* Module Loading Functions */
HMODULE WINAPI LoadLibraryA(LPCSTR lib_filename);
BOOL WINAPI FreeLibrary(HMODULE module);
FARPROC WINAPI GetProcAddress(HMODULE module, LPCSTR proc_name);
HMODULE WINAPI GetModuleHandleA(LPCSTR module_name);

/* Error Functions */
DWORD WINAPI GetLastError(void);
void WINAPI SetLastError(DWORD error_code);

/* System Information Functions */
void WINAPI GetSystemInfo(void* system_info);
DWORD WINAPI GetTickCount(void);

/* Initialize Kernel32.dll compatibility layer */
void kernel32_init(void);

#endif /* KERNEL32_H */
