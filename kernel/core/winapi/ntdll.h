/**
 * Aurora OS - NTDLL.dll API Compatibility Layer
 * 
 * Stub implementations of Windows NTDLL.dll (Native API) functions
 */

#ifndef NTDLL_H
#define NTDLL_H

#include "winapi.h"

/* NTSTATUS codes */
typedef LONG NTSTATUS;

#define STATUS_SUCCESS              ((NTSTATUS)0x00000000)
#define STATUS_UNSUCCESSFUL         ((NTSTATUS)0xC0000001)
#define STATUS_NOT_IMPLEMENTED      ((NTSTATUS)0xC0000002)
#define STATUS_INVALID_INFO_CLASS   ((NTSTATUS)0xC0000003)
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004)
#define STATUS_ACCESS_VIOLATION     ((NTSTATUS)0xC0000005)
#define STATUS_INVALID_HANDLE       ((NTSTATUS)0xC0000008)
#define STATUS_INVALID_PARAMETER    ((NTSTATUS)0xC000000D)
#define STATUS_NO_MEMORY            ((NTSTATUS)0xC0000017)
#define STATUS_OBJECT_NAME_NOT_FOUND ((NTSTATUS)0xC0000034)
#define STATUS_BUFFER_TOO_SMALL     ((NTSTATUS)0xC0000023)
#define STATUS_PENDING              ((NTSTATUS)0x00000103)

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

/* UNICODE_STRING structure */
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    LPWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

/* OBJECT_ATTRIBUTES structure */
typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    LPVOID SecurityDescriptor;
    LPVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

/* IO_STATUS_BLOCK structure */
typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        LPVOID Pointer;
    };
    ULONG Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

/* CLIENT_ID structure */
typedef struct _CLIENT_ID {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

/* Thread information classes */
typedef enum _THREADINFOCLASS {
    ThreadBasicInformation = 0,
    ThreadTimes = 1,
    ThreadPriority = 2,
    ThreadQuerySetWin32StartAddress = 9
} THREADINFOCLASS;

/* Process information classes */
typedef enum _PROCESSINFOCLASS {
    ProcessBasicInformation = 0,
    ProcessWow64Information = 26
} PROCESSINFOCLASS;

/* System information classes */
typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation = 0,
    SystemProcessorInformation = 1,
    SystemPerformanceInformation = 2,
    SystemTimeOfDayInformation = 3,
    SystemProcessInformation = 5
} SYSTEM_INFORMATION_CLASS;

/* RTL_USER_PROCESS_PARAMETERS */
typedef struct _RTL_USER_PROCESS_PARAMETERS {
    DWORD MaximumLength;
    DWORD Length;
    DWORD Flags;
    DWORD DebugFlags;
    LPVOID ConsoleHandle;
    DWORD ConsoleFlags;
    HANDLE StdInputHandle;
    HANDLE StdOutputHandle;
    HANDLE StdErrorHandle;
    UNICODE_STRING CurrentDirectoryPath;
    HANDLE CurrentDirectoryHandle;
    UNICODE_STRING DllPath;
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
    LPVOID Environment;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

/* PEB (Process Environment Block) */
typedef struct _PEB {
    BOOLEAN InheritedAddressSpace;
    BOOLEAN ReadImageFileExecOptions;
    BOOLEAN BeingDebugged;
    BOOLEAN SpareBool;
    HANDLE Mutant;
    LPVOID ImageBaseAddress;
    LPVOID Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    /* More fields exist but are not needed for basic compatibility */
} PEB, *PPEB;

/* TEB (Thread Environment Block) */
typedef struct _TEB {
    /* NT_TIB structure at the start */
    LPVOID ExceptionList;
    LPVOID StackBase;
    LPVOID StackLimit;
    LPVOID SubSystemTib;
    union {
        LPVOID FiberData;
        DWORD Version;
    };
    LPVOID ArbitraryUserPointer;
    struct _TEB* Self;
    /* Additional fields */
    LPVOID EnvironmentPointer;
    CLIENT_ID ClientId;
    LPVOID ActiveRpcHandle;
    LPVOID ThreadLocalStoragePointer;
    PPEB ProcessEnvironmentBlock;
    DWORD LastErrorValue;
} TEB, *PTEB;

/* NTDLL API functions */
void ntdll_init(void);

/* RTL functions */
void WINAPI RtlInitUnicodeString(PUNICODE_STRING DestinationString, LPCWSTR SourceString);
NTSTATUS WINAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING DestinationString,
                                              void* SourceString, BOOLEAN AllocateDestinationString);
void WINAPI RtlFreeUnicodeString(PUNICODE_STRING UnicodeString);
void WINAPI RtlZeroMemory(LPVOID Destination, SIZE_T Length);
void WINAPI RtlCopyMemory(LPVOID Destination, const LPVOID Source, SIZE_T Length);
void WINAPI RtlMoveMemory(LPVOID Destination, const LPVOID Source, SIZE_T Length);
void WINAPI RtlFillMemory(LPVOID Destination, SIZE_T Length, BYTE Fill);
SIZE_T WINAPI RtlCompareMemory(const LPVOID Source1, const LPVOID Source2, SIZE_T Length);
NTSTATUS WINAPI RtlGetVersion(void* lpVersionInformation);

/* Nt/Zw functions */
NTSTATUS WINAPI NtClose(HANDLE Handle);
NTSTATUS WINAPI NtCreateFile(PHANDLE FileHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes,
                              PIO_STATUS_BLOCK IoStatusBlock,
                              void* AllocationSize, DWORD FileAttributes,
                              DWORD ShareAccess, DWORD CreateDisposition,
                              DWORD CreateOptions, LPVOID EaBuffer, DWORD EaLength);
NTSTATUS WINAPI NtReadFile(HANDLE FileHandle, HANDLE Event, void* ApcRoutine,
                            LPVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
                            LPVOID Buffer, DWORD Length, void* ByteOffset, void* Key);
NTSTATUS WINAPI NtWriteFile(HANDLE FileHandle, HANDLE Event, void* ApcRoutine,
                             LPVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
                             LPCVOID Buffer, DWORD Length, void* ByteOffset, void* Key);
NTSTATUS WINAPI NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
                                           LPVOID ProcessInformation, ULONG ProcessInformationLength,
                                           PULONG ReturnLength);
NTSTATUS WINAPI NtQueryInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass,
                                          LPVOID ThreadInformation, ULONG ThreadInformationLength,
                                          PULONG ReturnLength);
NTSTATUS WINAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
                                          LPVOID SystemInformation, ULONG SystemInformationLength,
                                          PULONG ReturnLength);
NTSTATUS WINAPI NtDelayExecution(BOOLEAN Alertable, void* DelayInterval);
NTSTATUS WINAPI NtAllocateVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                         ULONG ZeroBits, PSIZE_T RegionSize,
                                         ULONG AllocationType, ULONG Protect);
NTSTATUS WINAPI NtFreeVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                     PSIZE_T RegionSize, ULONG FreeType);
NTSTATUS WINAPI NtQueryVirtualMemory(HANDLE ProcessHandle, LPVOID BaseAddress,
                                      int MemoryInformationClass, LPVOID MemoryInformation,
                                      SIZE_T MemoryInformationLength, PSIZE_T ReturnLength);

/* LdrGetDllHandle / LdrLoadDll / LdrGetProcedureAddress */
NTSTATUS WINAPI LdrLoadDll(LPCWSTR PathToFile, PULONG Flags, PUNICODE_STRING ModuleFileName,
                            PHANDLE ModuleHandle);
NTSTATUS WINAPI LdrUnloadDll(HANDLE ModuleHandle);
NTSTATUS WINAPI LdrGetProcedureAddress(HANDLE ModuleHandle, void* ProcedureName,
                                        DWORD ProcedureNumber, FARPROC* ProcedureAddress);
NTSTATUS WINAPI LdrGetDllHandle(LPCWSTR PathToFile, PULONG Flags, PUNICODE_STRING ModuleFileName,
                                 PHANDLE ModuleHandle);

/* NtCurrentTeb / NtCurrentPeb */
PTEB WINAPI NtCurrentTeb(void);
PPEB WINAPI NtCurrentPeb(void);

#endif /* NTDLL_H */
