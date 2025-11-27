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
void WINAPI ExitThread(DWORD exit_code);
BOOL WINAPI TerminateThread(HANDLE thread, DWORD exit_code);
HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
                           void* lpStartAddress, LPVOID lpParameter,
                           DWORD dwCreationFlags, LPDWORD lpThreadId);
DWORD WINAPI ResumeThread(HANDLE hThread);
DWORD WINAPI SuspendThread(HANDLE hThread);
BOOL WINAPI SetThreadPriority(HANDLE hThread, int nPriority);
int WINAPI GetThreadPriority(HANDLE hThread);
void WINAPI Sleep(DWORD dwMilliseconds);
DWORD WINAPI SleepEx(DWORD dwMilliseconds, BOOL bAlertable);
DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
DWORD WINAPI WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles,
                                     BOOL bWaitAll, DWORD dwMilliseconds);
BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode);
BOOL WINAPI GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);

/* Memory Management Functions */
LPVOID WINAPI VirtualAlloc(LPVOID address, SIZE_T size, DWORD alloc_type, DWORD protect);
BOOL WINAPI VirtualFree(LPVOID address, SIZE_T size, DWORD free_type);
BOOL WINAPI VirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, LPDWORD lpflOldProtect);
SIZE_T WINAPI VirtualQuery(LPCVOID lpAddress, void* lpBuffer, SIZE_T dwLength);
HGLOBAL WINAPI GlobalAlloc(DWORD flags, SIZE_T bytes);
HGLOBAL WINAPI GlobalFree(HGLOBAL mem);
LPVOID WINAPI GlobalLock(HGLOBAL hMem);
BOOL WINAPI GlobalUnlock(HGLOBAL hMem);
SIZE_T WINAPI GlobalSize(HGLOBAL hMem);
HLOCAL WINAPI LocalAlloc(DWORD flags, SIZE_T bytes);
HLOCAL WINAPI LocalFree(HLOCAL mem);
LPVOID WINAPI LocalLock(HLOCAL hMem);
BOOL WINAPI LocalUnlock(HLOCAL hMem);
SIZE_T WINAPI LocalSize(HLOCAL hMem);
HANDLE WINAPI HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
BOOL WINAPI HeapDestroy(HANDLE hHeap);
LPVOID WINAPI HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
BOOL WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
LPVOID WINAPI HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
SIZE_T WINAPI HeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem);
BOOL WINAPI HeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem);
HANDLE WINAPI GetProcessHeap(void);
void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer);

/* File Management Functions */
HANDLE WINAPI CreateFileA(LPCSTR filename, DWORD access, DWORD share_mode,
                          LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags, HANDLE template_file);
BOOL WINAPI ReadFile(HANDLE file, LPVOID buffer, DWORD bytes_to_read,
                     LPDWORD bytes_read, LPOVERLAPPED overlapped);
BOOL WINAPI WriteFile(HANDLE file, LPCVOID buffer, DWORD bytes_to_write,
                      LPDWORD bytes_written, LPOVERLAPPED overlapped);
BOOL WINAPI CloseHandle(HANDLE handle);
DWORD WINAPI GetFileSize(HANDLE file, LPDWORD high_size);
BOOL WINAPI GetFileSizeEx(HANDLE hFile, void* lpFileSize);
DWORD WINAPI SetFilePointer(HANDLE hFile, LONG lDistanceToMove,
                            LPLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
BOOL WINAPI SetEndOfFile(HANDLE hFile);
BOOL WINAPI FlushFileBuffers(HANDLE hFile);
BOOL WINAPI DeleteFileA(LPCSTR filename);
BOOL WINAPI CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists);
BOOL WINAPI MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName);
DWORD WINAPI GetFileAttributesA(LPCSTR lpFileName);
BOOL WINAPI SetFileAttributesA(LPCSTR lpFileName, DWORD dwFileAttributes);
DWORD WINAPI GetFileType(HANDLE hFile);
BOOL WINAPI GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime,
                        LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
BOOL WINAPI SetFileTime(HANDLE hFile, const FILETIME* lpCreationTime,
                        const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime);
BOOL WINAPI GetFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation);
BOOL WINAPI LockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                     DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh);
BOOL WINAPI UnlockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                       DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh);

/* Directory Functions */
BOOL WINAPI CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
BOOL WINAPI RemoveDirectoryA(LPCSTR lpPathName);
DWORD WINAPI GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer);
BOOL WINAPI SetCurrentDirectoryA(LPCSTR lpPathName);
HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData);
BOOL WINAPI FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData);
BOOL WINAPI FindClose(HANDLE hFindFile);
DWORD WINAPI GetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength,
                              LPSTR lpBuffer, LPSTR* lpFilePart);
DWORD WINAPI GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer);
DWORD WINAPI GetTempFileNameA(LPCSTR lpPathName, LPCSTR lpPrefixString,
                              DWORD uUnique, LPSTR lpTempFileName);

/* Console Functions */
HANDLE WINAPI GetStdHandle(DWORD std_handle);
BOOL WINAPI SetStdHandle(DWORD nStdHandle, HANDLE hHandle);
BOOL WINAPI WriteConsoleA(HANDLE console, LPCVOID buffer, DWORD length,
                          LPDWORD written, LPVOID reserved);
BOOL WINAPI ReadConsoleA(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead,
                         LPDWORD lpNumberOfCharsRead, LPVOID pInputControl);
BOOL WINAPI AllocConsole(void);
BOOL WINAPI FreeConsole(void);
BOOL WINAPI SetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode);
BOOL WINAPI GetConsoleMode(HANDLE hConsoleHandle, LPDWORD lpMode);
BOOL WINAPI SetConsoleTitleA(LPCSTR lpConsoleTitle);
DWORD WINAPI GetConsoleTitleA(LPSTR lpConsoleTitle, DWORD nSize);

/* String Functions */
int WINAPI lstrlenA(LPCSTR string);
LPSTR WINAPI lstrcpyA(LPSTR dest, LPCSTR src);
LPSTR WINAPI lstrcpynA(LPSTR lpString1, LPCSTR lpString2, int iMaxLength);
LPSTR WINAPI lstrcatA(LPSTR dest, LPCSTR src);
int WINAPI lstrcmpA(LPCSTR str1, LPCSTR str2);
int WINAPI lstrcmpiA(LPCSTR str1, LPCSTR str2);
int WINAPI MultiByteToWideChar(DWORD CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                               int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
int WINAPI WideCharToMultiByte(DWORD CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
                               int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte,
                               LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);

/* Module Loading Functions */
HMODULE WINAPI LoadLibraryA(LPCSTR lib_filename);
HMODULE WINAPI LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
BOOL WINAPI FreeLibrary(HMODULE module);
FARPROC WINAPI GetProcAddress(HMODULE module, LPCSTR proc_name);
HMODULE WINAPI GetModuleHandleA(LPCSTR module_name);
DWORD WINAPI GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize);

/* Error Functions */
DWORD WINAPI GetLastError(void);
void WINAPI SetLastError(DWORD error_code);
DWORD WINAPI FormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
                            DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, void* Arguments);

/* Environment Functions */
DWORD WINAPI GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
BOOL WINAPI SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue);
LPSTR WINAPI GetEnvironmentStringsA(void);
BOOL WINAPI FreeEnvironmentStringsA(LPSTR lpszEnvironmentBlock);
LPSTR WINAPI GetCommandLineA(void);

/* System Information Functions */
void WINAPI GetSystemInfo(LPSYSTEM_INFO system_info);
void WINAPI GetNativeSystemInfo(LPSYSTEM_INFO lpSystemInfo);
DWORD WINAPI GetTickCount(void);
BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA lpVersionInfo);
DWORD WINAPI GetVersion(void);
UINT WINAPI GetSystemDirectoryA(LPSTR lpBuffer, UINT uSize);
UINT WINAPI GetWindowsDirectoryA(LPSTR lpBuffer, UINT uSize);
void WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime);
void WINAPI GetLocalTime(LPSYSTEMTIME lpSystemTime);
BOOL WINAPI SetSystemTime(const SYSTEMTIME* lpSystemTime);
BOOL WINAPI SetLocalTime(const SYSTEMTIME* lpSystemTime);
BOOL WINAPI SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime, LPFILETIME lpFileTime);
BOOL WINAPI FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime);
DWORD WINAPI GetTimeZoneInformation(void* lpTimeZoneInformation);
BOOL WINAPI QueryPerformanceCounter(void* lpPerformanceCount);
BOOL WINAPI QueryPerformanceFrequency(void* lpFrequency);
DWORD WINAPI GetLogicalDrives(void);
DWORD WINAPI GetLogicalDriveStringsA(DWORD nBufferLength, LPSTR lpBuffer);
UINT WINAPI GetDriveTypeA(LPCSTR lpRootPathName);
BOOL WINAPI GetDiskFreeSpaceA(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster,
                              LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters,
                              LPDWORD lpTotalNumberOfClusters);
BOOL WINAPI GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize);
BOOL WINAPI GetUserNameA(LPSTR lpBuffer, LPDWORD pcbBuffer);

/* Synchronization Functions */
HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset,
                           BOOL bInitialState, LPCSTR lpName);
BOOL WINAPI SetEvent(HANDLE hEvent);
BOOL WINAPI ResetEvent(HANDLE hEvent);
BOOL WINAPI PulseEvent(HANDLE hEvent);
HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
BOOL WINAPI ReleaseMutex(HANDLE hMutex);
HANDLE WINAPI CreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount,
                               LONG lMaximumCount, LPCSTR lpName);
BOOL WINAPI ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount);
void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
BOOL WINAPI TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
LONG WINAPI InterlockedIncrement(LONG volatile* lpAddend);
LONG WINAPI InterlockedDecrement(LONG volatile* lpAddend);
LONG WINAPI InterlockedExchange(LONG volatile* Target, LONG Value);
LONG WINAPI InterlockedCompareExchange(LONG volatile* Destination, LONG Exchange, LONG Comparand);

/* Interlocked Operations */
LPVOID WINAPI InterlockedExchangePointer(LPVOID volatile* Target, LPVOID Value);
LPVOID WINAPI InterlockedCompareExchangePointer(LPVOID volatile* Destination,
                                                 LPVOID Exchange, LPVOID Comparand);

/* TLS Functions */
DWORD WINAPI TlsAlloc(void);
BOOL WINAPI TlsFree(DWORD dwTlsIndex);
LPVOID WINAPI TlsGetValue(DWORD dwTlsIndex);
BOOL WINAPI TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue);

/* Process Creation */
BOOL WINAPI CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                           LPSECURITY_ATTRIBUTES lpProcessAttributes,
                           LPSECURITY_ATTRIBUTES lpThreadAttributes,
                           BOOL bInheritHandles, DWORD dwCreationFlags,
                           LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,
                           LPSTARTUPINFOA lpStartupInfo,
                           LPPROCESS_INFORMATION lpProcessInformation);
HANDLE WINAPI OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId);

/* Debugging */
void WINAPI OutputDebugStringA(LPCSTR lpOutputString);
BOOL WINAPI IsDebuggerPresent(void);
void WINAPI DebugBreak(void);

/* Exception Handling */
LPVOID WINAPI AddVectoredExceptionHandler(DWORD First, void* Handler);
LPVOID WINAPI AddVectoredContinueHandler(DWORD First, void* Handler);
DWORD WINAPI RemoveVectoredExceptionHandler(LPVOID Handle);
DWORD WINAPI RemoveVectoredContinueHandler(LPVOID Handle);
void WINAPI RaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags,
                           DWORD nNumberOfArguments, const DWORD* lpArguments);
LPVOID WINAPI SetUnhandledExceptionFilter(void* lpTopLevelExceptionFilter);

/* Miscellaneous */
void WINAPI GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo);
DWORD WINAPI GetCurrentDirectory_A(DWORD nBufferLength, LPSTR lpBuffer);
BOOL WINAPI Beep(DWORD dwFreq, DWORD dwDuration);
UINT WINAPI SetErrorMode(UINT uMode);
DWORD WINAPI GetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault,
                                      LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
BOOL WINAPI WritePrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName,
                                       LPCSTR lpString, LPCSTR lpFileName);

/* Initialize Kernel32.dll compatibility layer */
void kernel32_init(void);

#endif /* KERNEL32_H */
