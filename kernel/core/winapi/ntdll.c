/**
 * Aurora OS - NTDLL.dll API Compatibility Layer Implementation
 * 
 * Stub implementations of Windows NTDLL.dll (Native API) functions
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
    if (UnicodeString && UnicodeString->Buffer) {
        kfree(UnicodeString->Buffer);
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
