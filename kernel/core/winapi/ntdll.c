/**
 * Aurora OS - NTDLL.dll API Compatibility Layer Implementation
 * 
 * Stub implementations of Windows NTDLL.dll (Native API) functions
 * 
 * Note: Static handle counters and state variables in this file are not 
 * thread-safe by design. In a production environment, thread safety should 
 * be handled at the kernel level using appropriate synchronization primitives 
 * (spinlocks, mutexes). These stub implementations prioritize simplicity 
 * for the compatibility layer.
 */

#include "ntdll.h"
#include "winapi.h"
#include "kernel32.h"
#include "../../memory/memory.h"
#include "../../drivers/vga.h"
#include "../../drivers/timer.h"
#include "../dll_loader.h"

/* Global TEB and PEB for the simulated process */
static TEB g_teb;
static PEB g_peb;
static RTL_USER_PROCESS_PARAMETERS g_process_params;
static int g_ntdll_initialized = 0;

/* Helper functions */
static void ntdll_memset(void* dest, int c, SIZE_T n) {
    uint8_t* d = (uint8_t*)dest;
    while (n--) *d++ = (uint8_t)c;
}

static void ntdll_memcpy(void* dest, const void* src, SIZE_T n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
}

static SIZE_T ntdll_wcslen(const WCHAR* str) {
    SIZE_T len = 0;
    if (str) while (*str++) len++;
    return len;
}

/* Initialize NTDLL compatibility layer */
void ntdll_init(void) {
    if (g_ntdll_initialized) return;
    
    ntdll_memset(&g_teb, 0, sizeof(TEB));
    ntdll_memset(&g_peb, 0, sizeof(PEB));
    ntdll_memset(&g_process_params, 0, sizeof(RTL_USER_PROCESS_PARAMETERS));
    
    /* Set up TEB */
    g_teb.Self = &g_teb;
    g_teb.ProcessEnvironmentBlock = &g_peb;
    g_teb.ClientId.UniqueProcess = (HANDLE)(uintptr_t)1;
    g_teb.ClientId.UniqueThread = (HANDLE)(uintptr_t)1;
    
    /* Set up PEB */
    g_peb.ProcessParameters = &g_process_params;
    g_peb.ImageBaseAddress = (LPVOID)0x00400000;
    g_peb.BeingDebugged = FALSE;
    
    /* Set up process parameters */
    g_process_params.Length = sizeof(RTL_USER_PROCESS_PARAMETERS);
    
    g_ntdll_initialized = 1;
    vga_write("NTDLL: Initialized\n");
}

/* RTL functions */

void WINAPI RtlInitUnicodeString(PUNICODE_STRING DestinationString, LPCWSTR SourceString) {
    if (!DestinationString) return;
    
    DestinationString->Buffer = (LPWSTR)SourceString;
    if (SourceString) {
        SIZE_T len = ntdll_wcslen(SourceString);
        DestinationString->Length = (USHORT)(len * sizeof(WCHAR));
        DestinationString->MaximumLength = (USHORT)((len + 1) * sizeof(WCHAR));
    } else {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
    }
}

NTSTATUS WINAPI RtlAnsiStringToUnicodeString(PUNICODE_STRING DestinationString,
                                              void* SourceString, BOOLEAN AllocateDestinationString) {
    (void)DestinationString; (void)SourceString; (void)AllocateDestinationString;
    return STATUS_NOT_IMPLEMENTED;
}

void WINAPI RtlFreeUnicodeString(PUNICODE_STRING UnicodeString) {
    /* Note: Only free buffers that were allocated by RTL functions.
     * RtlInitUnicodeString does not allocate memory, so we should
     * only free buffers allocated by RtlAnsiStringToUnicodeString
     * with AllocateDestinationString=TRUE. Since our implementation
     * doesn't allocate, we just clear the structure fields. */
    if (UnicodeString) {
        UnicodeString->Buffer = NULL;
        UnicodeString->Length = 0;
        UnicodeString->MaximumLength = 0;
    }
}

void WINAPI RtlZeroMemory(LPVOID Destination, SIZE_T Length) {
    ntdll_memset(Destination, 0, Length);
}

void WINAPI RtlCopyMemory(LPVOID Destination, const LPVOID Source, SIZE_T Length) {
    ntdll_memcpy(Destination, Source, Length);
}

void WINAPI RtlMoveMemory(LPVOID Destination, const LPVOID Source, SIZE_T Length) {
    /* Handle overlapping regions */
    uint8_t* d = (uint8_t*)Destination;
    const uint8_t* s = (const uint8_t*)Source;
    
    if (d < s) {
        while (Length--) *d++ = *s++;
    } else {
        d += Length;
        s += Length;
        while (Length--) *--d = *--s;
    }
}

void WINAPI RtlFillMemory(LPVOID Destination, SIZE_T Length, BYTE Fill) {
    ntdll_memset(Destination, Fill, Length);
}

SIZE_T WINAPI RtlCompareMemory(const LPVOID Source1, const LPVOID Source2, SIZE_T Length) {
    const uint8_t* s1 = (const uint8_t*)Source1;
    const uint8_t* s2 = (const uint8_t*)Source2;
    SIZE_T matched = 0;
    
    while (Length-- && *s1++ == *s2++) matched++;
    return matched;
}

NTSTATUS WINAPI RtlGetVersion(void* lpVersionInformation) {
    if (!lpVersionInformation) return STATUS_INVALID_PARAMETER;
    
    /* Mimic OSVERSIONINFOEXW */
    DWORD* info = (DWORD*)lpVersionInformation;
    info[1] = 6;    /* dwMajorVersion */
    info[2] = 1;    /* dwMinorVersion */
    info[3] = 7601; /* dwBuildNumber */
    info[4] = 2;    /* dwPlatformId = VER_PLATFORM_WIN32_NT */
    
    return STATUS_SUCCESS;
}

/* Nt/Zw functions */

NTSTATUS WINAPI NtClose(HANDLE Handle) {
    if (CloseHandle(Handle)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI NtCreateFile(PHANDLE FileHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes,
                              PIO_STATUS_BLOCK IoStatusBlock,
                              void* AllocationSize, DWORD FileAttributes,
                              DWORD ShareAccess, DWORD CreateDisposition,
                              DWORD CreateOptions, LPVOID EaBuffer, DWORD EaLength) {
    (void)ObjectAttributes; (void)AllocationSize; (void)CreateOptions;
    (void)EaBuffer; (void)EaLength;
    
    if (!FileHandle || !IoStatusBlock) {
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Convert NT creation disposition to Win32 */
    DWORD win32_disposition;
    switch (CreateDisposition) {
        case 0: win32_disposition = CREATE_NEW; break;
        case 1: win32_disposition = CREATE_ALWAYS; break;
        case 2: win32_disposition = OPEN_EXISTING; break;
        case 3: win32_disposition = OPEN_ALWAYS; break;
        case 4: win32_disposition = TRUNCATE_EXISTING; break;
        default: win32_disposition = OPEN_EXISTING; break;
    }
    
    /* Use the file path from ObjectAttributes if available */
    LPCSTR filename = "\\??\\C:\\temp.dat";  /* Default for now */
    
    *FileHandle = CreateFileA(filename, DesiredAccess, ShareAccess, NULL,
                              win32_disposition, FileAttributes, NULL);
    
    if (*FileHandle == INVALID_HANDLE_VALUE) {
        IoStatusBlock->Status = STATUS_OBJECT_NAME_NOT_FOUND;
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    IoStatusBlock->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtReadFile(HANDLE FileHandle, HANDLE Event, void* ApcRoutine,
                            LPVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
                            LPVOID Buffer, DWORD Length, void* ByteOffset, void* Key) {
    (void)Event; (void)ApcRoutine; (void)ApcContext; (void)ByteOffset; (void)Key;
    
    if (!IoStatusBlock) return STATUS_INVALID_PARAMETER;
    
    DWORD bytesRead = 0;
    if (ReadFile(FileHandle, Buffer, Length, &bytesRead, NULL)) {
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = bytesRead;
        return STATUS_SUCCESS;
    }
    
    IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI NtWriteFile(HANDLE FileHandle, HANDLE Event, void* ApcRoutine,
                             LPVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
                             LPCVOID Buffer, DWORD Length, void* ByteOffset, void* Key) {
    (void)Event; (void)ApcRoutine; (void)ApcContext; (void)ByteOffset; (void)Key;
    
    if (!IoStatusBlock) return STATUS_INVALID_PARAMETER;
    
    DWORD bytesWritten = 0;
    if (WriteFile(FileHandle, Buffer, Length, &bytesWritten, NULL)) {
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = bytesWritten;
        return STATUS_SUCCESS;
    }
    
    IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI NtQueryInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
                                           LPVOID ProcessInformation, ULONG ProcessInformationLength,
                                           PULONG ReturnLength) {
    (void)ProcessHandle; (void)ProcessInformationLength;
    
    if (!ProcessInformation) return STATUS_INVALID_PARAMETER;
    
    switch (ProcessInformationClass) {
        case ProcessBasicInformation:
            if (ReturnLength) *ReturnLength = 24;
            ntdll_memset(ProcessInformation, 0, 24);
            ((DWORD*)ProcessInformation)[5] = GetCurrentProcessId();
            return STATUS_SUCCESS;
            
        case ProcessWow64Information:
            if (ReturnLength) *ReturnLength = sizeof(ULONG);
            *(ULONG*)ProcessInformation = 0;  /* Not WOW64 */
            return STATUS_SUCCESS;
            
        default:
            return STATUS_INVALID_INFO_CLASS;
    }
}

NTSTATUS WINAPI NtQueryInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass,
                                          LPVOID ThreadInformation, ULONG ThreadInformationLength,
                                          PULONG ReturnLength) {
    (void)ThreadHandle; (void)ThreadInformationLength;
    
    if (!ThreadInformation) return STATUS_INVALID_PARAMETER;
    
    switch (ThreadInformationClass) {
        case ThreadBasicInformation:
            if (ReturnLength) *ReturnLength = 28;
            ntdll_memset(ThreadInformation, 0, 28);
            return STATUS_SUCCESS;
            
        default:
            return STATUS_INVALID_INFO_CLASS;
    }
}

NTSTATUS WINAPI NtQuerySystemInformation(SYSTEM_INFORMATION_CLASS SystemInformationClass,
                                          LPVOID SystemInformation, ULONG SystemInformationLength,
                                          PULONG ReturnLength) {
    (void)SystemInformationLength;
    
    if (!SystemInformation) return STATUS_INVALID_PARAMETER;
    
    switch (SystemInformationClass) {
        case SystemBasicInformation:
            if (ReturnLength) *ReturnLength = 44;
            ntdll_memset(SystemInformation, 0, 44);
            ((DWORD*)SystemInformation)[1] = 4096;  /* PageSize */
            ((DWORD*)SystemInformation)[4] = 1;     /* NumberOfProcessors */
            return STATUS_SUCCESS;
            
        default:
            return STATUS_NOT_IMPLEMENTED;
    }
}

NTSTATUS WINAPI NtDelayExecution(BOOLEAN Alertable, void* DelayInterval) {
    (void)Alertable;
    
    if (DelayInterval) {
        /* DelayInterval is LARGE_INTEGER in 100-nanosecond units */
        /* Negative means relative time */
        /* Simplified: just extract lower 32 bits and convert roughly */
        uint32_t* interval = (uint32_t*)DelayInterval;
        uint32_t low_part = interval[0];
        /* Approximate: each tick ~= 100ns, 10000 ticks = 1ms */
        /* Use shift instead of division for freestanding environment */
        DWORD ms = low_part >> 14;  /* Approximation (divide by ~16000) */
        if (ms == 0 && low_part > 0) ms = 1;
        Sleep(ms);
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtAllocateVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                         ULONG ZeroBits, PSIZE_T RegionSize,
                                         ULONG AllocationType, ULONG Protect) {
    (void)ProcessHandle; (void)ZeroBits;
    
    if (!BaseAddress || !RegionSize) return STATUS_INVALID_PARAMETER;
    
    *BaseAddress = VirtualAlloc(*BaseAddress, *RegionSize, AllocationType, Protect);
    if (!*BaseAddress) {
        return STATUS_NO_MEMORY;
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtFreeVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                     PSIZE_T RegionSize, ULONG FreeType) {
    (void)ProcessHandle;
    
    if (!BaseAddress) return STATUS_INVALID_PARAMETER;
    
    if (VirtualFree(*BaseAddress, RegionSize ? *RegionSize : 0, FreeType)) {
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI NtQueryVirtualMemory(HANDLE ProcessHandle, LPVOID BaseAddress,
                                      int MemoryInformationClass, LPVOID MemoryInformation,
                                      SIZE_T MemoryInformationLength, PSIZE_T ReturnLength) {
    (void)ProcessHandle; (void)BaseAddress; (void)MemoryInformationClass;
    (void)MemoryInformation; (void)MemoryInformationLength; (void)ReturnLength;
    return STATUS_NOT_IMPLEMENTED;
}

/* Ldr functions */

NTSTATUS WINAPI LdrLoadDll(LPCWSTR PathToFile, PULONG Flags, PUNICODE_STRING ModuleFileName,
                            PHANDLE ModuleHandle) {
    (void)PathToFile; (void)Flags;
    
    if (!ModuleFileName || !ModuleHandle) return STATUS_INVALID_PARAMETER;
    
    /* Convert UNICODE_STRING to ASCII (simplified) */
    char ansi_name[256];
    int len = ModuleFileName->Length / sizeof(WCHAR);
    if (len >= 256) len = 255;
    
    for (int i = 0; i < len; i++) {
        ansi_name[i] = (char)ModuleFileName->Buffer[i];
    }
    ansi_name[len] = '\0';
    
    HMODULE hMod = LoadLibraryA(ansi_name);
    if (!hMod) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    *ModuleHandle = hMod;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LdrUnloadDll(HANDLE ModuleHandle) {
    if (FreeLibrary((HMODULE)ModuleHandle)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI LdrGetProcedureAddress(HANDLE ModuleHandle, void* ProcedureName,
                                        DWORD ProcedureNumber, FARPROC* ProcedureAddress) {
    if (!ProcedureAddress) return STATUS_INVALID_PARAMETER;
    
    if (ProcedureName) {
        /* ProcedureName is ANSI_STRING */
        *ProcedureAddress = GetProcAddress((HMODULE)ModuleHandle, (LPCSTR)ProcedureName);
    } else {
        /* Look up by ordinal */
        *ProcedureAddress = (FARPROC)dll_get_proc_address_ordinal((HMODULE)ModuleHandle, (uint16_t)ProcedureNumber);
    }
    
    if (!*ProcedureAddress) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI LdrGetDllHandle(LPCWSTR PathToFile, PULONG Flags, PUNICODE_STRING ModuleFileName,
                                 PHANDLE ModuleHandle) {
    (void)PathToFile; (void)Flags;
    
    if (!ModuleFileName || !ModuleHandle) return STATUS_INVALID_PARAMETER;
    
    /* Convert UNICODE_STRING to ASCII (simplified) */
    char ansi_name[256];
    int len = ModuleFileName->Length / sizeof(WCHAR);
    if (len >= 256) len = 255;
    
    for (int i = 0; i < len; i++) {
        ansi_name[i] = (char)ModuleFileName->Buffer[i];
    }
    ansi_name[len] = '\0';
    
    HMODULE hMod = GetModuleHandleA(ansi_name);
    if (!hMod) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }
    
    *ModuleHandle = hMod;
    return STATUS_SUCCESS;
}

/* NtCurrentTeb / NtCurrentPeb */

PTEB WINAPI NtCurrentTeb(void) {
    if (!g_ntdll_initialized) ntdll_init();
    return &g_teb;
}

PPEB WINAPI NtCurrentPeb(void) {
    if (!g_ntdll_initialized) ntdll_init();
    return &g_peb;
}

/* ============================================ */
/* Additional File operations                   */
/* ============================================ */

NTSTATUS WINAPI NtOpenFile(PHANDLE FileHandle, DWORD DesiredAccess,
                           POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock,
                           DWORD ShareAccess, DWORD OpenOptions) {
    if (!FileHandle || !ObjectAttributes || !IoStatusBlock) {
        return STATUS_INVALID_PARAMETER;
    }
    
    /* Use NtCreateFile with OPEN_EXISTING disposition */
    return NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
                        NULL, FILE_ATTRIBUTE_NORMAL, ShareAccess, 2 /* OPEN_EXISTING */,
                        OpenOptions, NULL, 0);
}

NTSTATUS WINAPI NtSetInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
                                     LPVOID FileInformation, ULONG Length,
                                     DWORD FileInformationClass) {
    (void)FileHandle; (void)FileInformation; (void)Length; (void)FileInformationClass;
    
    if (!IoStatusBlock) return STATUS_INVALID_PARAMETER;
    
    /* Basic stub - would implement file info setting in real implementation */
    IoStatusBlock->Status = STATUS_SUCCESS;
    IoStatusBlock->Information = 0;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtQueryInformationFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock,
                                       LPVOID FileInformation, ULONG Length,
                                       DWORD FileInformationClass) {
    (void)FileHandle; (void)Length;
    
    if (!IoStatusBlock || !FileInformation) return STATUS_INVALID_PARAMETER;
    
    switch (FileInformationClass) {
        case 4:  /* FileBasicInformation */
            ntdll_memset(FileInformation, 0, Length > 40 ? 40 : Length);
            IoStatusBlock->Status = STATUS_SUCCESS;
            IoStatusBlock->Information = 40;
            return STATUS_SUCCESS;
            
        case 5:  /* FileStandardInformation */
            ntdll_memset(FileInformation, 0, Length > 24 ? 24 : Length);
            IoStatusBlock->Status = STATUS_SUCCESS;
            IoStatusBlock->Information = 24;
            return STATUS_SUCCESS;
            
        case 14: /* FilePositionInformation */
            ntdll_memset(FileInformation, 0, Length > 8 ? 8 : Length);
            IoStatusBlock->Status = STATUS_SUCCESS;
            IoStatusBlock->Information = 8;
            return STATUS_SUCCESS;
            
        default:
            return STATUS_NOT_IMPLEMENTED;
    }
}

NTSTATUS WINAPI NtFlushBuffersFile(HANDLE FileHandle, PIO_STATUS_BLOCK IoStatusBlock) {
    (void)FileHandle;
    
    if (!IoStatusBlock) return STATUS_INVALID_PARAMETER;
    
    /* Simplified - just mark as successful */
    IoStatusBlock->Status = STATUS_SUCCESS;
    IoStatusBlock->Information = 0;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtDeleteFile(POBJECT_ATTRIBUTES ObjectAttributes) {
    (void)ObjectAttributes;
    /* Would delete file in real implementation */
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtQueryDirectoryFile(HANDLE FileHandle, HANDLE Event, void* ApcRoutine,
                                     LPVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock,
                                     LPVOID FileInformation, ULONG Length,
                                     DWORD FileInformationClass, BOOLEAN ReturnSingleEntry,
                                     PUNICODE_STRING FileName, BOOLEAN RestartScan) {
    (void)FileHandle; (void)Event; (void)ApcRoutine; (void)ApcContext;
    (void)FileInformation; (void)Length; (void)FileInformationClass;
    (void)ReturnSingleEntry; (void)FileName; (void)RestartScan;
    
    if (!IoStatusBlock) return STATUS_INVALID_PARAMETER;
    
    /* Return no more entries - empty directory stub */
    IoStatusBlock->Status = STATUS_NO_MORE_FILES;
    return STATUS_NO_MORE_FILES;
}

/* ============================================ */
/* Additional Memory operations                 */
/* ============================================ */

NTSTATUS WINAPI NtProtectVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                       PSIZE_T RegionSize, ULONG NewProtect, PULONG OldProtect) {
    (void)ProcessHandle;
    
    if (!BaseAddress || !RegionSize || !OldProtect) return STATUS_INVALID_PARAMETER;
    
    /* Store old protection and apply new (simplified) */
    *OldProtect = PAGE_READWRITE;  /* Default old protection */
    
    if (VirtualProtect(*BaseAddress, *RegionSize, NewProtect, OldProtect)) {
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI NtLockVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                    PSIZE_T RegionSize, ULONG MapType) {
    (void)ProcessHandle; (void)BaseAddress; (void)RegionSize; (void)MapType;
    /* Memory locking stub - would lock pages in real implementation */
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtUnlockVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                      PSIZE_T RegionSize, ULONG MapType) {
    (void)ProcessHandle; (void)BaseAddress; (void)RegionSize; (void)MapType;
    /* Memory unlocking stub */
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtFlushVirtualMemory(HANDLE ProcessHandle, LPVOID* BaseAddress,
                                     PSIZE_T RegionSize, PIO_STATUS_BLOCK IoStatusBlock) {
    (void)ProcessHandle; (void)BaseAddress; (void)RegionSize;
    
    if (IoStatusBlock) {
        IoStatusBlock->Status = STATUS_SUCCESS;
        IoStatusBlock->Information = 0;
    }
    return STATUS_SUCCESS;
}

/* ============================================ */
/* Section (memory-mapped file) operations      */
/* ============================================ */

static DWORD g_next_section_handle = 0xE0000;

NTSTATUS WINAPI NtCreateSection(PHANDLE SectionHandle, DWORD DesiredAccess,
                                POBJECT_ATTRIBUTES ObjectAttributes, void* MaximumSize,
                                ULONG SectionPageProtection, ULONG AllocationAttributes,
                                HANDLE FileHandle) {
    (void)DesiredAccess; (void)ObjectAttributes; (void)MaximumSize;
    (void)SectionPageProtection; (void)AllocationAttributes; (void)FileHandle;
    
    if (!SectionHandle) return STATUS_INVALID_PARAMETER;
    
    *SectionHandle = (HANDLE)(uintptr_t)(g_next_section_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtOpenSection(PHANDLE SectionHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!SectionHandle) return STATUS_INVALID_PARAMETER;
    
    /* Stub - return a fake handle */
    *SectionHandle = (HANDLE)(uintptr_t)(g_next_section_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtMapViewOfSection(HANDLE SectionHandle, HANDLE ProcessHandle,
                                   LPVOID* BaseAddress, ULONG ZeroBits, SIZE_T CommitSize,
                                   void* SectionOffset, PSIZE_T ViewSize,
                                   DWORD InheritDisposition, ULONG AllocationType, ULONG Win32Protect) {
    (void)SectionHandle; (void)ProcessHandle; (void)ZeroBits; (void)CommitSize;
    (void)SectionOffset; (void)InheritDisposition; (void)AllocationType;
    
    if (!BaseAddress || !ViewSize) return STATUS_INVALID_PARAMETER;
    
    /* Allocate memory for the mapped view */
    *BaseAddress = VirtualAlloc(*BaseAddress, *ViewSize, MEM_COMMIT | MEM_RESERVE, Win32Protect);
    if (!*BaseAddress) {
        return STATUS_NO_MEMORY;
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtUnmapViewOfSection(HANDLE ProcessHandle, LPVOID BaseAddress) {
    (void)ProcessHandle;
    
    if (!BaseAddress) return STATUS_INVALID_PARAMETER;
    
    if (VirtualFree(BaseAddress, 0, MEM_RELEASE)) {
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI NtExtendSection(HANDLE SectionHandle, void* NewSectionSize) {
    (void)SectionHandle; (void)NewSectionSize;
    return STATUS_SUCCESS;
}

/* ============================================ */
/* Process operations                           */
/* ============================================ */

NTSTATUS WINAPI NtTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus) {
    (void)ExitStatus;
    
    if (ProcessHandle == (HANDLE)-1 || ProcessHandle == GetCurrentProcess()) {
        /* Terminate current process */
        ExitProcess((DWORD)ExitStatus);
        return STATUS_SUCCESS;  /* Never reached */
    }
    
    if (TerminateProcess(ProcessHandle, (DWORD)ExitStatus)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI NtOpenProcess(PHANDLE ProcessHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes, void* ClientId) {
    (void)ObjectAttributes;
    
    if (!ProcessHandle || !ClientId) return STATUS_INVALID_PARAMETER;
    
    /* Extract process ID from CLIENT_ID structure */
    PCLIENT_ID cid = (PCLIENT_ID)ClientId;
    DWORD processId = (DWORD)(uintptr_t)cid->UniqueProcess;
    
    *ProcessHandle = OpenProcess(DesiredAccess, FALSE, processId);
    if (*ProcessHandle == NULL) {
        return STATUS_INVALID_HANDLE;
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtSuspendProcess(HANDLE ProcessHandle) {
    (void)ProcessHandle;
    /* Would suspend all threads in process */
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtResumeProcess(HANDLE ProcessHandle) {
    (void)ProcessHandle;
    /* Would resume all threads in process */
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtSetInformationProcess(HANDLE ProcessHandle, PROCESSINFOCLASS ProcessInformationClass,
                                        LPVOID ProcessInformation, ULONG ProcessInformationLength) {
    (void)ProcessHandle; (void)ProcessInformationClass;
    (void)ProcessInformation; (void)ProcessInformationLength;
    return STATUS_SUCCESS;
}

/* ============================================ */
/* Thread operations                            */
/* ============================================ */

static DWORD g_next_thread_handle = 0xF0000;

NTSTATUS WINAPI NtCreateThread(PHANDLE ThreadHandle, DWORD DesiredAccess,
                               POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ProcessHandle,
                               void* ClientId, void* ThreadContext, void* InitialTeb,
                               BOOLEAN CreateSuspended) {
    (void)DesiredAccess; (void)ObjectAttributes; (void)ProcessHandle;
    (void)ClientId; (void)ThreadContext; (void)InitialTeb; (void)CreateSuspended;
    
    if (!ThreadHandle) return STATUS_INVALID_PARAMETER;
    
    /* Return fake handle - real implementation would create thread */
    *ThreadHandle = (HANDLE)(uintptr_t)(g_next_thread_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtCreateThreadEx(PHANDLE ThreadHandle, DWORD DesiredAccess,
                                 POBJECT_ATTRIBUTES ObjectAttributes, HANDLE ProcessHandle,
                                 void* StartRoutine, LPVOID Argument, ULONG CreateFlags,
                                 SIZE_T ZeroBits, SIZE_T StackSize, SIZE_T MaximumStackSize,
                                 void* AttributeList) {
    (void)DesiredAccess; (void)ObjectAttributes; (void)ProcessHandle;
    (void)StartRoutine; (void)Argument; (void)CreateFlags; (void)ZeroBits;
    (void)StackSize; (void)MaximumStackSize; (void)AttributeList;
    
    if (!ThreadHandle) return STATUS_INVALID_PARAMETER;
    
    /* Return fake handle */
    *ThreadHandle = (HANDLE)(uintptr_t)(g_next_thread_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtOpenThread(PHANDLE ThreadHandle, DWORD DesiredAccess,
                             POBJECT_ATTRIBUTES ObjectAttributes, void* ClientId) {
    (void)DesiredAccess; (void)ObjectAttributes; (void)ClientId;
    
    if (!ThreadHandle) return STATUS_INVALID_PARAMETER;
    
    *ThreadHandle = (HANDLE)(uintptr_t)(g_next_thread_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtTerminateThread(HANDLE ThreadHandle, NTSTATUS ExitStatus) {
    if (TerminateThread(ThreadHandle, (DWORD)ExitStatus)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI NtSuspendThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount) {
    DWORD count = SuspendThread(ThreadHandle);
    if (count == (DWORD)-1) {
        return STATUS_INVALID_HANDLE;
    }
    if (PreviousSuspendCount) *PreviousSuspendCount = count;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount) {
    DWORD count = ResumeThread(ThreadHandle);
    if (count == (DWORD)-1) {
        return STATUS_INVALID_HANDLE;
    }
    if (PreviousSuspendCount) *PreviousSuspendCount = count;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtSetInformationThread(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass,
                                       LPVOID ThreadInformation, ULONG ThreadInformationLength) {
    (void)ThreadHandle; (void)ThreadInformationClass;
    (void)ThreadInformation; (void)ThreadInformationLength;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtAlertThread(HANDLE ThreadHandle) {
    (void)ThreadHandle;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtAlertResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount) {
    return NtResumeThread(ThreadHandle, PreviousSuspendCount);
}

NTSTATUS WINAPI NtGetContextThread(HANDLE ThreadHandle, void* ThreadContext) {
    (void)ThreadHandle; (void)ThreadContext;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtSetContextThread(HANDLE ThreadHandle, void* ThreadContext) {
    (void)ThreadHandle; (void)ThreadContext;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS WINAPI NtYieldExecution(void) {
    /* Yield to other threads */
    Sleep(0);
    return STATUS_SUCCESS;
}

/* ============================================ */
/* Synchronization objects                      */
/* ============================================ */

static DWORD g_next_event_handle = 0x100000;
static DWORD g_next_mutant_handle = 0x110000;
static DWORD g_next_semaphore_handle = 0x120000;

NTSTATUS WINAPI NtCreateEvent(PHANDLE EventHandle, DWORD DesiredAccess,
                              POBJECT_ATTRIBUTES ObjectAttributes, DWORD EventType,
                              BOOLEAN InitialState) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!EventHandle) return STATUS_INVALID_PARAMETER;
    
    HANDLE h = CreateEventA(NULL, EventType == 0 /* NotificationEvent */, InitialState, NULL);
    if (!h) {
        *EventHandle = (HANDLE)(uintptr_t)(g_next_event_handle++);
    } else {
        *EventHandle = h;
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtOpenEvent(PHANDLE EventHandle, DWORD DesiredAccess,
                            POBJECT_ATTRIBUTES ObjectAttributes) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!EventHandle) return STATUS_INVALID_PARAMETER;
    
    *EventHandle = (HANDLE)(uintptr_t)(g_next_event_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtSetEvent(HANDLE EventHandle, PLONG PreviousState) {
    if (PreviousState) *PreviousState = 0;
    
    if (SetEvent(EventHandle)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI NtResetEvent(HANDLE EventHandle, PLONG PreviousState) {
    if (PreviousState) *PreviousState = 0;
    
    if (ResetEvent(EventHandle)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI NtPulseEvent(HANDLE EventHandle, PLONG PreviousState) {
    if (PreviousState) *PreviousState = 0;
    
    if (PulseEvent(EventHandle)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI NtClearEvent(HANDLE EventHandle) {
    return NtResetEvent(EventHandle, NULL);
}

NTSTATUS WINAPI NtCreateMutant(PHANDLE MutantHandle, DWORD DesiredAccess,
                               POBJECT_ATTRIBUTES ObjectAttributes, BOOLEAN InitialOwner) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!MutantHandle) return STATUS_INVALID_PARAMETER;
    
    HANDLE h = CreateMutexA(NULL, InitialOwner, NULL);
    if (!h) {
        *MutantHandle = (HANDLE)(uintptr_t)(g_next_mutant_handle++);
    } else {
        *MutantHandle = h;
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtOpenMutant(PHANDLE MutantHandle, DWORD DesiredAccess,
                             POBJECT_ATTRIBUTES ObjectAttributes) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!MutantHandle) return STATUS_INVALID_PARAMETER;
    
    *MutantHandle = (HANDLE)(uintptr_t)(g_next_mutant_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtReleaseMutant(HANDLE MutantHandle, PLONG PreviousCount) {
    if (PreviousCount) *PreviousCount = 0;
    
    if (ReleaseMutex(MutantHandle)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

NTSTATUS WINAPI NtCreateSemaphore(PHANDLE SemaphoreHandle, DWORD DesiredAccess,
                                  POBJECT_ATTRIBUTES ObjectAttributes, LONG InitialCount, LONG MaximumCount) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!SemaphoreHandle) return STATUS_INVALID_PARAMETER;
    
    HANDLE h = CreateSemaphoreA(NULL, InitialCount, MaximumCount, NULL);
    if (!h) {
        *SemaphoreHandle = (HANDLE)(uintptr_t)(g_next_semaphore_handle++);
    } else {
        *SemaphoreHandle = h;
    }
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtOpenSemaphore(PHANDLE SemaphoreHandle, DWORD DesiredAccess,
                                POBJECT_ATTRIBUTES ObjectAttributes) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!SemaphoreHandle) return STATUS_INVALID_PARAMETER;
    
    *SemaphoreHandle = (HANDLE)(uintptr_t)(g_next_semaphore_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtReleaseSemaphore(HANDLE SemaphoreHandle, LONG ReleaseCount, PLONG PreviousCount) {
    if (ReleaseSemaphore(SemaphoreHandle, ReleaseCount, PreviousCount)) {
        return STATUS_SUCCESS;
    }
    return STATUS_INVALID_HANDLE;
}

/* ============================================ */
/* Wait operations                              */
/* ============================================ */

NTSTATUS WINAPI NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, void* Timeout) {
    (void)Alertable;
    
    DWORD waitMs = INFINITE;
    if (Timeout) {
        /* Convert 100-nanosecond units to milliseconds */
        uint64_t* timeout_val = (uint64_t*)Timeout;
        if (*timeout_val < 0) {
            /* Negative = relative time */
            uint64_t abs_val = (uint64_t)(-(int64_t)*timeout_val);
            waitMs = (DWORD)(abs_val / 10000);
        } else {
            /* Positive = absolute time (not fully supported) */
            waitMs = (DWORD)(*timeout_val / 10000);
        }
    }
    
    DWORD result = WaitForSingleObject(Handle, waitMs);
    
    switch (result) {
        case WAIT_OBJECT_0:  return STATUS_SUCCESS;
        case WAIT_TIMEOUT:   return STATUS_TIMEOUT;
        case WAIT_ABANDONED: return STATUS_ABANDONED;
        default:             return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS WINAPI NtWaitForMultipleObjects(ULONG Count, const HANDLE* Handles,
                                         DWORD WaitType, BOOLEAN Alertable, void* Timeout) {
    (void)Alertable;
    
    DWORD waitMs = INFINITE;
    if (Timeout) {
        uint64_t* timeout_val = (uint64_t*)Timeout;
        if (*timeout_val < 0) {
            uint64_t abs_val = (uint64_t)(-(int64_t)*timeout_val);
            waitMs = (DWORD)(abs_val / 10000);
        } else {
            waitMs = (DWORD)(*timeout_val / 10000);
        }
    }
    
    DWORD result = WaitForMultipleObjects(Count, Handles, WaitType == 0 /* WaitAll */, waitMs);
    
    if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + Count) {
        return STATUS_SUCCESS;
    }
    if (result == WAIT_TIMEOUT) return STATUS_TIMEOUT;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS WINAPI NtSignalAndWaitForSingleObject(HANDLE SignalHandle, HANDLE WaitHandle,
                                               BOOLEAN Alertable, void* Timeout) {
    /* Signal the first object */
    NtSetEvent(SignalHandle, NULL);
    
    /* Wait for the second object */
    return NtWaitForSingleObject(WaitHandle, Alertable, Timeout);
}

/* ============================================ */
/* Registry operations                          */
/* ============================================ */

static DWORD g_next_key_handle = 0x130000;

NTSTATUS WINAPI NtCreateKey(PHANDLE KeyHandle, DWORD DesiredAccess,
                            POBJECT_ATTRIBUTES ObjectAttributes, ULONG TitleIndex,
                            PUNICODE_STRING Class, ULONG CreateOptions, PULONG Disposition) {
    (void)DesiredAccess; (void)ObjectAttributes; (void)TitleIndex;
    (void)Class; (void)CreateOptions;
    
    if (!KeyHandle) return STATUS_INVALID_PARAMETER;
    
    *KeyHandle = (HANDLE)(uintptr_t)(g_next_key_handle++);
    if (Disposition) *Disposition = 1;  /* REG_CREATED_NEW_KEY */
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtOpenKey(PHANDLE KeyHandle, DWORD DesiredAccess,
                          POBJECT_ATTRIBUTES ObjectAttributes) {
    (void)DesiredAccess; (void)ObjectAttributes;
    
    if (!KeyHandle) return STATUS_INVALID_PARAMETER;
    
    *KeyHandle = (HANDLE)(uintptr_t)(g_next_key_handle++);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtDeleteKey(HANDLE KeyHandle) {
    (void)KeyHandle;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
                              ULONG TitleIndex, ULONG Type, LPVOID Data, ULONG DataSize) {
    (void)KeyHandle; (void)ValueName; (void)TitleIndex;
    (void)Type; (void)Data; (void)DataSize;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtQueryValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
                                DWORD KeyValueInformationClass, LPVOID KeyValueInformation,
                                ULONG Length, PULONG ResultLength) {
    (void)KeyHandle; (void)ValueName; (void)KeyValueInformationClass;
    (void)KeyValueInformation; (void)Length;
    
    if (ResultLength) *ResultLength = 0;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS WINAPI NtDeleteValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName) {
    (void)KeyHandle; (void)ValueName;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtEnumerateKey(HANDLE KeyHandle, ULONG Index,
                               DWORD KeyInformationClass, LPVOID KeyInformation,
                               ULONG Length, PULONG ResultLength) {
    (void)KeyHandle; (void)Index; (void)KeyInformationClass;
    (void)KeyInformation; (void)Length;
    
    if (ResultLength) *ResultLength = 0;
    return STATUS_NO_MORE_ENTRIES;
}

NTSTATUS WINAPI NtEnumerateValueKey(HANDLE KeyHandle, ULONG Index,
                                    DWORD KeyValueInformationClass, LPVOID KeyValueInformation,
                                    ULONG Length, PULONG ResultLength) {
    (void)KeyHandle; (void)Index; (void)KeyValueInformationClass;
    (void)KeyValueInformation; (void)Length;
    
    if (ResultLength) *ResultLength = 0;
    return STATUS_NO_MORE_ENTRIES;
}

NTSTATUS WINAPI NtFlushKey(HANDLE KeyHandle) {
    (void)KeyHandle;
    return STATUS_SUCCESS;
}

/* ============================================ */
/* RTL additional functions                     */
/* ============================================ */

void WINAPI RtlInitAnsiString(void* DestinationString, LPCSTR SourceString) {
    PANSI_STRING dest = (PANSI_STRING)DestinationString;
    
    if (!dest) return;
    
    dest->Buffer = (LPSTR)SourceString;
    if (SourceString) {
        SIZE_T len = 0;
        while (SourceString[len]) len++;
        dest->Length = (USHORT)len;
        dest->MaximumLength = (USHORT)(len + 1);
    } else {
        dest->Length = 0;
        dest->MaximumLength = 0;
    }
}

NTSTATUS WINAPI RtlUnicodeStringToAnsiString(void* DestinationString,
                                              PUNICODE_STRING SourceString,
                                              BOOLEAN AllocateDestinationString) {
    (void)AllocateDestinationString;
    
    PANSI_STRING dest = (PANSI_STRING)DestinationString;
    
    if (!dest || !SourceString) return STATUS_INVALID_PARAMETER;
    
    /* Simplified conversion - just copy lower bytes */
    SIZE_T len = SourceString->Length / sizeof(WCHAR);
    
    if (AllocateDestinationString) {
        /* Would allocate memory here */
        return STATUS_NOT_IMPLEMENTED;
    }
    
    if (dest->MaximumLength < len + 1) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    for (SIZE_T i = 0; i < len; i++) {
        dest->Buffer[i] = (char)SourceString->Buffer[i];
    }
    dest->Buffer[len] = '\0';
    dest->Length = (USHORT)len;
    
    return STATUS_SUCCESS;
}

void WINAPI RtlFreeAnsiString(void* AnsiString) {
    PANSI_STRING str = (PANSI_STRING)AnsiString;
    if (str) {
        str->Buffer = NULL;
        str->Length = 0;
        str->MaximumLength = 0;
    }
}

LONG WINAPI RtlCompareUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive) {
    if (!String1 || !String2) return 0;
    
    SIZE_T len1 = String1->Length / sizeof(WCHAR);
    SIZE_T len2 = String2->Length / sizeof(WCHAR);
    SIZE_T min_len = len1 < len2 ? len1 : len2;
    
    for (SIZE_T i = 0; i < min_len; i++) {
        WCHAR c1 = String1->Buffer[i];
        WCHAR c2 = String2->Buffer[i];
        
        if (CaseInSensitive) {
            if (c1 >= 'a' && c1 <= 'z') c1 -= 32;
            if (c2 >= 'a' && c2 <= 'z') c2 -= 32;
        }
        
        if (c1 != c2) return (LONG)c1 - (LONG)c2;
    }
    
    return (LONG)len1 - (LONG)len2;
}

BOOLEAN WINAPI RtlEqualUnicodeString(PUNICODE_STRING String1, PUNICODE_STRING String2, BOOLEAN CaseInSensitive) {
    return RtlCompareUnicodeString(String1, String2, CaseInSensitive) == 0;
}

NTSTATUS WINAPI RtlAppendUnicodeStringToString(PUNICODE_STRING Destination, PUNICODE_STRING Source) {
    if (!Destination || !Source) return STATUS_INVALID_PARAMETER;
    
    SIZE_T dest_len = Destination->Length / sizeof(WCHAR);
    SIZE_T src_len = Source->Length / sizeof(WCHAR);
    SIZE_T new_len = dest_len + src_len;
    
    if ((new_len + 1) * sizeof(WCHAR) > Destination->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    for (SIZE_T i = 0; i < src_len; i++) {
        Destination->Buffer[dest_len + i] = Source->Buffer[i];
    }
    Destination->Buffer[new_len] = 0;
    Destination->Length = (USHORT)(new_len * sizeof(WCHAR));
    
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlAppendUnicodeToString(PUNICODE_STRING Destination, LPCWSTR Source) {
    if (!Destination) return STATUS_INVALID_PARAMETER;
    if (!Source) return STATUS_SUCCESS;
    
    UNICODE_STRING src;
    RtlInitUnicodeString(&src, Source);
    return RtlAppendUnicodeStringToString(Destination, &src);
}

NTSTATUS WINAPI RtlUpcaseUnicodeString(PUNICODE_STRING DestinationString,
                                        PUNICODE_STRING SourceString, BOOLEAN AllocateDestinationString) {
    (void)AllocateDestinationString;
    
    if (!DestinationString || !SourceString) return STATUS_INVALID_PARAMETER;
    
    SIZE_T len = SourceString->Length / sizeof(WCHAR);
    
    for (SIZE_T i = 0; i < len; i++) {
        WCHAR c = SourceString->Buffer[i];
        DestinationString->Buffer[i] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
    }
    DestinationString->Length = SourceString->Length;
    
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlDowncaseUnicodeString(PUNICODE_STRING DestinationString,
                                          PUNICODE_STRING SourceString, BOOLEAN AllocateDestinationString) {
    (void)AllocateDestinationString;
    
    if (!DestinationString || !SourceString) return STATUS_INVALID_PARAMETER;
    
    SIZE_T len = SourceString->Length / sizeof(WCHAR);
    
    for (SIZE_T i = 0; i < len; i++) {
        WCHAR c = SourceString->Buffer[i];
        DestinationString->Buffer[i] = (c >= 'A' && c <= 'Z') ? (c + 32) : c;
    }
    DestinationString->Length = SourceString->Length;
    
    return STATUS_SUCCESS;
}

WCHAR WINAPI RtlUpcaseUnicodeChar(WCHAR SourceCharacter) {
    return (SourceCharacter >= 'a' && SourceCharacter <= 'z') ? (SourceCharacter - 32) : SourceCharacter;
}

WCHAR WINAPI RtlDowncaseUnicodeChar(WCHAR SourceCharacter) {
    return (SourceCharacter >= 'A' && SourceCharacter <= 'Z') ? (SourceCharacter + 32) : SourceCharacter;
}

NTSTATUS WINAPI RtlIntegerToUnicodeString(ULONG Value, ULONG Base, PUNICODE_STRING String) {
    if (!String || !String->Buffer) return STATUS_INVALID_PARAMETER;
    if (Base == 0) Base = 10;
    if (Base != 2 && Base != 8 && Base != 10 && Base != 16) return STATUS_INVALID_PARAMETER;
    
    /* Use array initialization instead of wide string literal */
    WCHAR digits[17] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 0};
    WCHAR buffer[33];
    int pos = 32;
    buffer[pos] = 0;
    
    if (Value == 0) {
        buffer[--pos] = '0';
    } else {
        while (Value > 0 && pos > 0) {
            buffer[--pos] = digits[Value % Base];
            Value /= Base;
        }
    }
    
    SIZE_T len = 32 - pos;
    if ((len + 1) * sizeof(WCHAR) > String->MaximumLength) {
        return STATUS_BUFFER_TOO_SMALL;
    }
    
    for (SIZE_T i = 0; i <= len; i++) {
        String->Buffer[i] = buffer[pos + i];
    }
    String->Length = (USHORT)(len * sizeof(WCHAR));
    
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlUnicodeStringToInteger(PUNICODE_STRING String, ULONG Base, PULONG Value) {
    if (!String || !Value) return STATUS_INVALID_PARAMETER;
    if (Base == 0) Base = 10;
    
    *Value = 0;
    SIZE_T len = String->Length / sizeof(WCHAR);
    
    for (SIZE_T i = 0; i < len; i++) {
        WCHAR c = String->Buffer[i];
        ULONG digit;
        
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'A' && c <= 'F') digit = 10 + c - 'A';
        else if (c >= 'a' && c <= 'f') digit = 10 + c - 'a';
        else break;
        
        if (digit >= Base) break;
        *Value = *Value * Base + digit;
    }
    
    return STATUS_SUCCESS;
}

/* ============================================ */
/* Heap management                              */
/* ============================================ */

HANDLE WINAPI RtlCreateHeap(ULONG Flags, LPVOID HeapBase, SIZE_T ReserveSize,
                            SIZE_T CommitSize, LPVOID Lock, void* Parameters) {
    (void)HeapBase; (void)ReserveSize; (void)CommitSize; (void)Lock; (void)Parameters;
    return HeapCreate(Flags, 0, 0);
}

HANDLE WINAPI RtlDestroyHeap(HANDLE HeapHandle) {
    HeapDestroy(HeapHandle);
    return NULL;
}

LPVOID WINAPI RtlAllocateHeap(HANDLE HeapHandle, ULONG Flags, SIZE_T Size) {
    return HeapAlloc(HeapHandle, Flags, Size);
}

BOOLEAN WINAPI RtlFreeHeap(HANDLE HeapHandle, ULONG Flags, LPVOID HeapBase) {
    return HeapFree(HeapHandle, Flags, HeapBase);
}

LPVOID WINAPI RtlReAllocateHeap(HANDLE HeapHandle, ULONG Flags, LPVOID BaseAddress, SIZE_T Size) {
    return HeapReAlloc(HeapHandle, Flags, BaseAddress, Size);
}

SIZE_T WINAPI RtlSizeHeap(HANDLE HeapHandle, ULONG Flags, LPVOID BaseAddress) {
    return HeapSize(HeapHandle, Flags, BaseAddress);
}

BOOLEAN WINAPI RtlValidateHeap(HANDLE HeapHandle, ULONG Flags, LPVOID BaseAddress) {
    return HeapValidate(HeapHandle, Flags, BaseAddress);
}

ULONG WINAPI RtlGetProcessHeaps(ULONG NumberOfHeaps, HANDLE* ProcessHeaps) {
    if (NumberOfHeaps > 0 && ProcessHeaps) {
        ProcessHeaps[0] = GetProcessHeap();
        return 1;
    }
    return 1;
}

/* ============================================ */
/* Exception handling                           */
/* ============================================ */

BOOLEAN WINAPI RtlDispatchException(void* ExceptionRecord, void* Context) {
    (void)ExceptionRecord; (void)Context;
    return FALSE;  /* Exception not handled */
}

void WINAPI RtlRaiseException(void* ExceptionRecord) {
    (void)ExceptionRecord;
    /* Would raise exception */
}

void WINAPI RtlUnwind(LPVOID TargetFrame, LPVOID TargetIp, void* ExceptionRecord, LPVOID ReturnValue) {
    (void)TargetFrame; (void)TargetIp; (void)ExceptionRecord; (void)ReturnValue;
    /* Would unwind stack */
}

/* ============================================ */
/* Critical sections                            */
/* ============================================ */

NTSTATUS WINAPI RtlInitializeCriticalSection(void* CriticalSection) {
    InitializeCriticalSection((LPCRITICAL_SECTION)CriticalSection);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlInitializeCriticalSectionAndSpinCount(void* CriticalSection, ULONG SpinCount) {
    (void)SpinCount;
    InitializeCriticalSection((LPCRITICAL_SECTION)CriticalSection);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlDeleteCriticalSection(void* CriticalSection) {
    DeleteCriticalSection((LPCRITICAL_SECTION)CriticalSection);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlEnterCriticalSection(void* CriticalSection) {
    EnterCriticalSection((LPCRITICAL_SECTION)CriticalSection);
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlLeaveCriticalSection(void* CriticalSection) {
    LeaveCriticalSection((LPCRITICAL_SECTION)CriticalSection);
    return STATUS_SUCCESS;
}

BOOLEAN WINAPI RtlTryEnterCriticalSection(void* CriticalSection) {
    return TryEnterCriticalSection((LPCRITICAL_SECTION)CriticalSection);
}

/* ============================================ */
/* Environment functions                        */
/* ============================================ */

NTSTATUS WINAPI RtlCreateEnvironment(BOOLEAN CloneCurrentEnvironment, LPVOID* Environment) {
    (void)CloneCurrentEnvironment;
    if (!Environment) return STATUS_INVALID_PARAMETER;
    *Environment = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlDestroyEnvironment(LPVOID Environment) {
    (void)Environment;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlSetCurrentEnvironment(LPVOID Environment, LPVOID* PreviousEnvironment) {
    (void)Environment;
    if (PreviousEnvironment) *PreviousEnvironment = NULL;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI RtlQueryEnvironmentVariable_U(LPVOID Environment, PUNICODE_STRING Name, PUNICODE_STRING Value) {
    (void)Environment; (void)Name; (void)Value;
    return STATUS_OBJECT_NAME_NOT_FOUND;
}

NTSTATUS WINAPI RtlSetEnvironmentVariable(LPVOID* Environment, PUNICODE_STRING Name, PUNICODE_STRING Value) {
    (void)Environment; (void)Name; (void)Value;
    return STATUS_SUCCESS;
}

/* ============================================ */
/* Path functions                               */
/* ============================================ */

ULONG WINAPI RtlGetCurrentDirectory_U(ULONG BufferLength, LPWSTR Buffer) {
    (void)Buffer;
    if (BufferLength < 4) return 4;
    if (Buffer) {
        Buffer[0] = 'C';
        Buffer[1] = ':';
        Buffer[2] = '\\';
        Buffer[3] = 0;
    }
    return 3;
}

NTSTATUS WINAPI RtlSetCurrentDirectory_U(PUNICODE_STRING PathName) {
    (void)PathName;
    return STATUS_SUCCESS;
}

ULONG WINAPI RtlGetFullPathName_U(LPCWSTR FileName, ULONG BufferLength, LPWSTR Buffer, LPWSTR* FilePart) {
    if (!FileName || !Buffer) return 0;
    
    /* Simple implementation - just copy the filename */
    SIZE_T len = ntdll_wcslen(FileName);
    if ((len + 1) * sizeof(WCHAR) > BufferLength) {
        return (ULONG)((len + 1) * sizeof(WCHAR));
    }
    
    for (SIZE_T i = 0; i <= len; i++) {
        Buffer[i] = FileName[i];
    }
    
    if (FilePart) {
        *FilePart = Buffer;
        for (SIZE_T i = len; i > 0; i--) {
            if (Buffer[i-1] == '\\' || Buffer[i-1] == '/') {
                *FilePart = &Buffer[i];
                break;
            }
        }
    }
    
    return (ULONG)(len * sizeof(WCHAR));
}

BOOLEAN WINAPI RtlDosPathNameToNtPathName_U(LPCWSTR DosFileName, PUNICODE_STRING NtFileName,
                                            LPWSTR* FilePart, void* RelativeName) {
    (void)FilePart; (void)RelativeName;
    
    if (!DosFileName || !NtFileName) return FALSE;
    
    /* Simple conversion - just copy with \\??\\ prefix */
    RtlInitUnicodeString(NtFileName, DosFileName);
    return TRUE;
}

void WINAPI RtlFreeHeapString(HANDLE HeapHandle, PUNICODE_STRING String) {
    if (String && String->Buffer) {
        RtlFreeHeap(HeapHandle, 0, String->Buffer);
        String->Buffer = NULL;
        String->Length = 0;
        String->MaximumLength = 0;
    }
}

/* ============================================ */
/* System time functions                        */
/* ============================================ */

void WINAPI RtlTimeToTimeFields(void* Time, void* TimeFields) {
    (void)Time; (void)TimeFields;
    /* Would convert LARGE_INTEGER to TIME_FIELDS */
}

BOOLEAN WINAPI RtlTimeFieldsToTime(void* TimeFields, void* Time) {
    (void)TimeFields; (void)Time;
    return TRUE;
}

void WINAPI RtlSecondsSince1970ToTime(ULONG ElapsedSeconds, void* Time) {
    (void)ElapsedSeconds; (void)Time;
}

void WINAPI RtlSecondsSince1980ToTime(ULONG ElapsedSeconds, void* Time) {
    (void)ElapsedSeconds; (void)Time;
}

BOOLEAN WINAPI RtlTimeToSecondsSince1970(void* Time, PULONG ElapsedSeconds) {
    (void)Time;
    if (ElapsedSeconds) *ElapsedSeconds = 0;
    return TRUE;
}

BOOLEAN WINAPI RtlTimeToSecondsSince1980(void* Time, PULONG ElapsedSeconds) {
    (void)Time;
    if (ElapsedSeconds) *ElapsedSeconds = 0;
    return TRUE;
}

/* ============================================ */
/* System information                           */
/* ============================================ */

NTSTATUS WINAPI NtQuerySystemTime(void* SystemTime) {
    (void)SystemTime;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtSetSystemTime(void* SystemTime, void* PreviousTime) {
    (void)SystemTime; (void)PreviousTime;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtQueryPerformanceCounter(void* PerformanceCounter, void* PerformanceFrequency) {
    QueryPerformanceCounter(PerformanceCounter);
    if (PerformanceFrequency) {
        QueryPerformanceFrequency(PerformanceFrequency);
    }
    return STATUS_SUCCESS;
}

/* ============================================ */
/* Debugging                                    */
/* ============================================ */

NTSTATUS WINAPI NtSetDebugFilterState(ULONG ComponentId, ULONG Level, BOOLEAN State) {
    (void)ComponentId; (void)Level; (void)State;
    return STATUS_SUCCESS;
}

NTSTATUS WINAPI NtQueryDebugFilterState(ULONG ComponentId, ULONG Level) {
    (void)ComponentId; (void)Level;
    return STATUS_SUCCESS;
}

void WINAPI DbgPrint(LPCSTR Format, ...) {
    (void)Format;
    /* Would format and output debug string */
}

void WINAPI DbgBreakPoint(void) {
    DebugBreak();
}
