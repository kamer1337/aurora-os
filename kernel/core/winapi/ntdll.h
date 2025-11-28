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
#define STATUS_ABANDONED            ((NTSTATUS)0x00000080)
#define STATUS_NO_MORE_FILES        ((NTSTATUS)0x80000006)

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

/* ============================================ */
/* Additional Native API functions              */
/* ============================================ */

/* File operations */
NTSTATUS WINAPI NtOpenFile(PHANDLE FileHandle, DWORD DesiredAccess,
                           POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
                           DWORD ShareAccess, DWORD OpenOptions);
NTSTATUS WINAPI NtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
                                     LPVOID FileInformation, ULONG Length,
                                     DWORD FileInformationClass);
NTSTATUS WINAPI NtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
                                       LPVOID FileInformation, ULONG Length,
                                       DWORD FileInformationClass);
NTSTATUS WINAPI NtFlushBuffersFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock);
NTSTATUS WINAPI NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS WINAPI NtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, void* ApcRoutine,
                                     LPVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
                                     LPVOID FileInformation, ULONG Length,
                                     DWORD FileInformationClass, BOOLEAN ReturnSingleEntry,
                                     PUNICODE_STRING FileName, BOOLEAN RestartScan);

/* Memory operations */
NTSTATUS WINAPI NtProtectVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                       PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect);
NTSTATUS WINAPI NtLockVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                    PSIZE_T RegionSize, ULONG MapType);
NTSTATUS WINAPI NtUnlockVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                      PSIZE_T RegionSize, ULONG MapType);
NTSTATUS WINAPI NtFlushVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                     PSIZE_T RegionSize, PIO_STATUS_BLOCK IoStatusBlock);

/* Section (memory-mapped file) operations */
NTSTATUS WINAPI NtCreateSection(PHANDLE SectionHandle, DWORD DesiredAccess,
                                POBJECT_ATTRIBUTES ObjectAttributes, void* MaximumSize,
                                ULONG SectionPageProtection, ULONG AllocationAttributes,
                                HANDLE FileHandle);
NTSTATUS WINAPI NtOpenSection(PHANDLE SectionHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS WINAPI NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle,
                                   LPVOID* BaseAddress, ULONG ZeroBits, SIZE_T CommitSize,
                                   void* SectionOffset, PSIZE_T ViewSize,
                                   DWORD InheritDisposition, ULONG AllocationType, ULONG Win32Protect);
NTSTATUS WINAPI NtUnmapViewOfSection(HANDLE ProcessHandle, LPVOID BaseAddress);
NTSTATUS WINAPI NtExtendSection(HANDLE SectionHandle, void* NewSectionSize);

/* Process operations */
NTSTATUS WINAPI NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus);
NTSTATUS WINAPI NtOpenProcess(PHANDLE ProcessHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes, void* ClientId);
NTSTATUS WINAPI NtSuspendProcess(HANDLE ProcessHandle);
NTSTATUS WINAPI NtResumeProcess(HANDLE ProcessHandle);
NTSTATUS WINAPI NtSetInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
                                        LPVOID ProcessInformation, ULONG ProcessInformationLength);

/* Thread operations */
NTSTATUS WINAPI NtCreateThread(PHANDLE ThreadHandle, DWORD DesiredAccess,
                               POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ProcessHandle,
                               void* ClientId, void* ThreadContext, void* InitialTeb,
                               BOOLEAN CreateSuspended);
NTSTATUS WINAPI NtCreateThreadEx(PHANDLE ThreadHandle, DWORD DesiredAccess,
                                 POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ProcessHandle,
                                 void* StartRoutine, LPVOID Argument, ULONG CreateFlags,
                                 SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize,
                                 void* AttributeList);
NTSTATUS WINAPI NtOpenThread(PHANDLE ThreadHandle, DWORD DesiredAccess,
                             POBJECT_ATTRIBUTES ObjectAttributes, void* ClientId);
NTSTATUS WINAPI NtTerminateThread(HANDLE ThreadHandle, NTSTATUS ExitStatus);
NTSTATUS WINAPI NtSuspendThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount);
NTSTATUS WINAPI NtResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount);
NTSTATUS WINAPI NtSetInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass,
                                       LPVOID ThreadInformation, ULONG ThreadInformationLength);
NTSTATUS WINAPI NtAlertThread(HANDLE ThreadHandle);
NTSTATUS WINAPI NtAlertResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount);
NTSTATUS WINAPI NtGetContextThread(HANDLE ThreadHandle, void* ThreadContext);
NTSTATUS WINAPI NtSetContextThread(HANDLE ThreadHandle, void* ThreadContext);
NTSTATUS WINAPI NtYieldExecution(void);

/* Synchronization objects */
NTSTATUS WINAPI NtCreateEvent(PHANDLE EventHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes, DWORD EventType,
                              BOOLEAN InitialState);
NTSTATUS WINAPI NtOpenEvent(PHANDLE EventHandle, DWORD DesiredAccess,
                            POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS WINAPI NtSetEvent(HANDLE EventHandle, PLONG PreviousState);
NTSTATUS WINAPI NtResetEvent(HANDLE EventHandle, PLONG PreviousState);
NTSTATUS WINAPI NtPulseEvent(HANDLE EventHandle, PLONG PreviousState);
NTSTATUS WINAPI NtClearEvent(HANDLE EventHandle);

NTSTATUS WINAPI NtCreateMutant(PHANDLE MutantHandle, DWORD DesiredAccess,
                               POBJECT_ATTRIBUTES ObjectAttributes, BOOLEAN InitialOwner);
NTSTATUS WINAPI NtOpenMutant(PHANDLE MutantHandle, DWORD DesiredAccess,
                             POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS WINAPI NtReleaseMutant(HANDLE MutantHandle, PLONG PreviousCount);

NTSTATUS WINAPI NtCreateSemaphore(PHANDLE SemaphoreHandle, DWORD DesiredAccess,
                                  POBJECT_ATTRIBUTES ObjectAttributes, LONG InitialCount, LONG MaximumCount);
NTSTATUS WINAPI NtOpenSemaphore(PHANDLE SemaphoreHandle, DWORD DesiredAccess,
                                POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS WINAPI NtReleaseSemaphore(HANDLE SemaphoreHandle, LONG ReleaseCount, PLONG PreviousCount);

/* Wait operations */
NTSTATUS WINAPI NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, void* Timeout);
NTSTATUS WINAPI NtWaitForMultipleObjects(ULONG Count, const HANDLE* Handles,
                                         DWORD WaitType, BOOLEAN Alertable, void* Timeout);
NTSTATUS WINAPI NtSignalAndWaitForSingleObject(HANDLE SignalHandle, HANDLE WaitHandle,
                                               BOOLEAN Alertable, void* Timeout);

/* Registry operations */
NTSTATUS WINAPI NtCreateKey(PHANDLE KeyHandle, DWORD DesiredAccess,
                            POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex,
                            PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition);
NTSTATUS WINAPI NtOpenKey(PHANDLE KeyHandle, DWORD DesiredAccess,
                          POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS WINAPI NtDeleteKey(HANDLE KeyHandle);
NTSTATUS WINAPI NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
                              ULONG TitleIndex, ULONG Type, LPVOID Data, ULONG DataSize);
NTSTATUS WINAPI NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
                                DWORD KeyValueInformationClass, LPVOID KeyValueInformation,
                                ULONG Length, PULONG ResultLength);
NTSTATUS WINAPI NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName);
NTSTATUS WINAPI NtEnumerateKey(HANDLE KeyHandle, ULONG Index,
                               DWORD KeyInformationClass, LPVOID KeyInformation,
                               ULONG Length, PULONG ResultLength);
NTSTATUS WINAPI NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index,
                                    DWORD KeyValueInformationClass, LPVOID KeyValueInformation,
                                    ULONG Length, PULONG ResultLength);
NTSTATUS WINAPI NtFlushKey(HANDLE KeyHandle);

/* RTL additional functions */
void WINAPI RtlInitAnsiString(void* DestinationString, LPCSTR SourceString);
NTSTATUS WINAPI RtlUnicodeStringToAnsiString(void* DestinationString,
                                              PUNICODE_STRING SourceString,
                                              BOOLEAN AllocateDestinationString);
void WINAPI RtlFreeAnsiString(void* AnsiString);
LONG WINAPI RtlCompareUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive);
BOOLEAN WINAPI RtlEqualUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive);
NTSTATUS WINAPI RtlAppendUnicodeStringToString(PUNICODE_STRING Destination, PUNICODE_STRING Source);
NTSTATUS WINAPI RtlAppendUnicodeToString(PUNICODE_STRING Destination, LPCWSTR Source);
NTSTATUS WINAPI RtlUpcaseUnicodeString(PUNICODE_STRING DestinationString,
                                        PUNICODE_STRING SourceString, BOOLEAN AllocateDestinationString);
NTSTATUS WINAPI RtlDowncaseUnicodeString(PUNICODE_STRING DestinationString,
                                          PUNICODE_STRING SourceString, BOOLEAN AllocateDestinationString);
WCHAR WINAPI RtlUpcaseUnicodeChar(WCHAR SourceCharacter);
WCHAR WINAPI RtlDowncaseUnicodeChar(WCHAR SourceCharacter);
NTSTATUS WINAPI RtlIntegerToUnicodeString(ULONG Value, ULONG Base, PUNICODE_STRING String);
NTSTATUS WINAPI RtlUnicodeStringToInteger(PUNICODE_STRING String, ULONG Base, PULONG Value);

/* Heap management */
HANDLE WINAPI RtlCreateHeap(ULONG Flags, LPVOID HeapBase, SIZE_T ReserveSize,
                            SIZE_T CommitSize, LPVOID Lock, void* Parameters);
HANDLE WINAPI RtlDestroyHeap(HANDLE HeapHandle);
LPVOID WINAPI RtlAllocateHeap(HANDLE HeapHandle, ULONG Flags, SIZE_T Size);
BOOLEAN WINAPI RtlFreeHeap(HANDLE HeapHandle, ULONG Flags, LPVOID HeapBase);
LPVOID WINAPI RtlReAllocateHeap(HANDLE HeapHandle, ULONG Flags, LPVOID BaseAddress, SIZE_T Size);
SIZE_T WINAPI RtlSizeHeap(HANDLE HeapHandle, ULONG Flags, LPVOID BaseAddress);
BOOLEAN WINAPI RtlValidateHeap(HANDLE HeapHandle, ULONG Flags, LPVOID BaseAddress);
ULONG WINAPI RtlGetProcessHeaps(ULONG NumberOfHeaps, HANDLE* ProcessHeaps);

/* Exception handling */
BOOLEAN WINAPI RtlDispatchException(void* ExceptionRecord, void* Context);
void WINAPI RtlRaiseException(void* ExceptionRecord);
void WINAPI RtlUnwind(LPVOID TargetFrame, LPVOID TargetIp, void* ExceptionRecord, LPVOID ReturnValue);

/* Critical sections */
NTSTATUS WINAPI RtlInitializeCriticalSection(void* CriticalSection);
NTSTATUS WINAPI RtlInitializeCriticalSectionAndSpinCount(void* CriticalSection, ULONG SpinCount);
NTSTATUS WINAPI RtlDeleteCriticalSection(void* CriticalSection);
NTSTATUS WINAPI RtlEnterCriticalSection(void* CriticalSection);
NTSTATUS WINAPI RtlLeaveCriticalSection(void* CriticalSection);
BOOLEAN WINAPI RtlTryEnterCriticalSection(void* CriticalSection);

/* Environment functions */
NTSTATUS WINAPI RtlCreateEnvironment(BOOLEAN CloneCurrentEnvironment, LPVOID* Environment);
NTSTATUS WINAPI RtlDestroyEnvironment(LPVOID Environment);
NTSTATUS WINAPI RtlSetCurrentEnvironment(LPVOID Environment, LPVOID* PreviousEnvironment);
NTSTATUS WINAPI RtlQueryEnvironmentVariable_U(LPVOID Environment, PUNICODE_STRING Name, PUNICODE_STRING Value);
NTSTATUS WINAPI RtlSetEnvironmentVariable(LPVOID* Environment, PUNICODE_STRING Name, PUNICODE_STRING Value);

/* Path functions */
ULONG WINAPI RtlGetCurrentDirectory_U(ULONG BufferLength, LPWSTR Buffer);
NTSTATUS WINAPI RtlSetCurrentDirectory_U(PUNICODE_STRING PathName);
ULONG WINAPI RtlGetFullPathName_U(LPCWSTR FileName, ULONG BufferLength, LPWSTR Buffer, LPWSTR* FilePart);
BOOLEAN WINAPI RtlDosPathNameToNtPathName_U(LPCWSTR DosFileName, PUNICODE_STRING NtFileName,
                                            LPWSTR* FilePart, void* RelativeName);
void WINAPI RtlFreeHeapString(HANDLE HeapHandle, PUNICODE_STRING String);

/* System time functions */
void WINAPI RtlTimeToTimeFields(void* Time, void* TimeFields);
BOOLEAN WINAPI RtlTimeFieldsToTime(void* TimeFields, void* Time);
void WINAPI RtlSecondsSince1970ToTime(ULONG ElapsedSeconds, void* Time);
void WINAPI RtlSecondsSince1980ToTime(ULONG ElapsedSeconds, void* Time);
BOOLEAN WINAPI RtlTimeToSecondsSince1970(void* Time, PULONG ElapsedSeconds);
BOOLEAN WINAPI RtlTimeToSecondsSince1980(void* Time, PULONG ElapsedSeconds);

/* System information */
NTSTATUS WINAPI NtQuerySystemTime(void* SystemTime);
NTSTATUS WINAPI NtSetSystemTime(void* SystemTime, void* PreviousTime);
NTSTATUS WINAPI NtQueryPerformanceCounter(void* PerformanceCounter, void* PerformanceFrequency);

/* Debugging */
NTSTATUS WINAPI NtSetDebugFilterState(ULONG ComponentId, ULONG Level, BOOLEAN State);
NTSTATUS WINAPI NtQueryDebugFilterState(ULONG ComponentId, ULONG Level);
void WINAPI DbgPrint(LPCSTR Format, ...);
void WINAPI DbgBreakPoint(void);

/* Additional status codes */
#define STATUS_OBJECT_TYPE_MISMATCH    ((NTSTATUS)0xC0000024)
#define STATUS_PORT_DISCONNECTED       ((NTSTATUS)0xC0000037)
#define STATUS_OBJECT_NAME_INVALID     ((NTSTATUS)0xC0000033)
#define STATUS_OBJECT_NAME_COLLISION   ((NTSTATUS)0xC0000035)
#define STATUS_OBJECT_PATH_NOT_FOUND   ((NTSTATUS)0xC000003A)
#define STATUS_OBJECT_PATH_SYNTAX_BAD  ((NTSTATUS)0xC000003B)
#define STATUS_INSUFFICIENT_RESOURCES  ((NTSTATUS)0xC000009A)
#define STATUS_SECTION_NOT_IMAGE       ((NTSTATUS)0xC0000049)
#define STATUS_ILLEGAL_FUNCTION        ((NTSTATUS)0xC00000AF)
#define STATUS_END_OF_FILE             ((NTSTATUS)0xC0000011)
#define STATUS_NO_MORE_ENTRIES         ((NTSTATUS)0x8000001A)
#define STATUS_MORE_ENTRIES            ((NTSTATUS)0x00000105)
#define STATUS_TIMEOUT                 ((NTSTATUS)0x00000102)
#define STATUS_CANCELLED               ((NTSTATUS)0xC0000120)
#define STATUS_ALERTED                 ((NTSTATUS)0x00000101)
#define STATUS_USER_APC                ((NTSTATUS)0x000000C0)

/* Wait types */
#define WaitAll 0
#define WaitAny 1

/* Event types */
#define NotificationEvent 0
#define SynchronizationEvent 1

/* ANSI_STRING structure */
typedef struct _ANSI_STRING {
    USHORT Length;
    USHORT MaximumLength;
    LPSTR Buffer;
} ANSI_STRING, *PANSI_STRING;

/* File information classes */
typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation = 1,
    FileFullDirectoryInformation = 2,
    FileBothDirectoryInformation = 3,
    FileBasicInformation = 4,
    FileStandardInformation = 5,
    FileInternalInformation = 6,
    FileEaInformation = 7,
    FileAccessInformation = 8,
    FileNameInformation = 9,
    FileRenameInformation = 10,
    FileLinkInformation = 11,
    FileNamesInformation = 12,
    FileDispositionInformation = 13,
    FilePositionInformation = 14,
    FileFullEaInformation = 15,
    FileModeInformation = 16,
    FileAlignmentInformation = 17,
    FileAllInformation = 18,
    FileAllocationInformation = 19,
    FileEndOfFileInformation = 20,
    FileAlternateNameInformation = 21,
    FileStreamInformation = 22,
    FilePipeInformation = 23,
    FilePipeLocalInformation = 24,
    FilePipeRemoteInformation = 25,
    FileMailslotQueryInformation = 26,
    FileMailslotSetInformation = 27,
    FileCompressionInformation = 28,
    FileObjectIdInformation = 29,
    FileCompletionInformation = 30,
    FileMoveClusterInformation = 31,
    FileQuotaInformation = 32,
    FileReparsePointInformation = 33,
    FileNetworkOpenInformation = 34,
    FileAttributeTagInformation = 35,
    FileTrackingInformation = 36,
    FileIdBothDirectoryInformation = 37,
    FileIdFullDirectoryInformation = 38,
    FileValidDataLengthInformation = 39,
    FileShortNameInformation = 40
} FILE_INFORMATION_CLASS;

/* Section inherit disposition */
typedef enum _SECTION_INHERIT {
    ViewShare = 1,
    ViewUnmap = 2
} SECTION_INHERIT;

/* Section attributes */
#define SEC_BASED       0x00200000
#define SEC_NO_CHANGE   0x00400000
#define SEC_FILE        0x00800000
#define SEC_IMAGE       0x01000000
#define SEC_RESERVE     0x04000000
#define SEC_COMMIT      0x08000000
#define SEC_NOCACHE     0x10000000

/* Memory map type */
#define MAP_PROCESS 1
#define MAP_SYSTEM  2

#endif /* NTDLL_H */
