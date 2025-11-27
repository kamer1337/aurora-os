/**
 * Aurora OS - Kernel32.dll API Compatibility Layer Implementation
 * 
 * Implementation of common Kernel32.dll functions
 */

#include "kernel32.h"
#include "winapi.h"
#include "../../memory/memory.h"
#include "../../drivers/vga.h"
#include "../dll_loader.h"
#include "../../../filesystem/vfs/vfs.h"
#include <stddef.h>

/* Simple handle counter */
static DWORD g_next_handle = 0x1000;

/* Handle to VFS file descriptor mapping */
#define MAX_FILE_HANDLES 64
static struct {
    HANDLE handle;
    int vfs_fd;
    int in_use;
} file_handle_table[MAX_FILE_HANDLES];

/* String helper functions */
static int k32_strlen(const char* s) {
    int len = 0;
    while (s && *s++) len++;
    return len;
}

static void k32_strcpy(char* dest, const char* src) {
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static int k32_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

/* Initialize file handle table */
static void init_file_handle_table(void) {
    for (int i = 0; i < MAX_FILE_HANDLES; i++) {
        file_handle_table[i].handle = INVALID_HANDLE_VALUE;
        file_handle_table[i].vfs_fd = -1;
        file_handle_table[i].in_use = 0;
    }
}

/* Allocate a new file handle */
static HANDLE alloc_file_handle(int vfs_fd) {
    for (int i = 0; i < MAX_FILE_HANDLES; i++) {
        if (!file_handle_table[i].in_use) {
            file_handle_table[i].handle = (HANDLE)(g_next_handle++);
            file_handle_table[i].vfs_fd = vfs_fd;
            file_handle_table[i].in_use = 1;
            return file_handle_table[i].handle;
        }
    }
    return INVALID_HANDLE_VALUE;
}

/* Get VFS file descriptor from handle */
static int get_vfs_fd(HANDLE handle) {
    for (int i = 0; i < MAX_FILE_HANDLES; i++) {
        if (file_handle_table[i].in_use && file_handle_table[i].handle == handle) {
            return file_handle_table[i].vfs_fd;
        }
    }
    return -1;
}

/* Free a file handle */
static void free_file_handle(HANDLE handle) {
    for (int i = 0; i < MAX_FILE_HANDLES; i++) {
        if (file_handle_table[i].in_use && file_handle_table[i].handle == handle) {
            file_handle_table[i].handle = INVALID_HANDLE_VALUE;
            file_handle_table[i].vfs_fd = -1;
            file_handle_table[i].in_use = 0;
            return;
        }
    }
}

/**
 * Initialize Kernel32.dll compatibility layer
 */
void kernel32_init(void) {
    init_file_handle_table();
    vga_write("Kernel32: Initialized\n");
}

/* ========== Process and Thread Functions ========== */

HANDLE WINAPI GetCurrentProcess(void) {
    return (HANDLE)0xFFFFFFFF;  /* Special pseudo-handle */
}

DWORD WINAPI GetCurrentProcessId(void) {
    return 1;  /* Return stub PID */
}

HANDLE WINAPI GetCurrentThread(void) {
    return (HANDLE)0xFFFFFFFE;  /* Special pseudo-handle */
}

DWORD WINAPI GetCurrentThreadId(void) {
    return 1;  /* Return stub thread ID */
}

void WINAPI ExitProcess(DWORD exit_code) {
    (void)exit_code;
    vga_write("Kernel32: ExitProcess called\n");
    /* In a real implementation, this would terminate the process */
}

BOOL WINAPI TerminateProcess(HANDLE process, DWORD exit_code) {
    (void)process;
    (void)exit_code;
    vga_write("Kernel32: TerminateProcess called\n");
    return TRUE;
}

/* ========== Memory Management Functions ========== */

LPVOID WINAPI VirtualAlloc(LPVOID address, DWORD size, DWORD alloc_type, DWORD protect) {
    (void)address;
    (void)alloc_type;
    (void)protect;
    
    if (size == 0) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    void* ptr = kmalloc(size);
    if (!ptr) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return ptr;
}

BOOL WINAPI VirtualFree(LPVOID address, DWORD size, DWORD free_type) {
    (void)size;
    (void)free_type;
    
    if (!address) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    kfree(address);
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HGLOBAL WINAPI GlobalAlloc(DWORD flags, DWORD bytes) {
    (void)flags;
    
    if (bytes == 0) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    void* ptr = kmalloc(bytes);
    if (!ptr) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return (HGLOBAL)ptr;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL mem) {
    if (!mem) {
        return NULL;
    }
    
    kfree((void*)mem);
    winapi_set_last_error(ERROR_SUCCESS);
    return NULL;
}

HLOCAL WINAPI LocalAlloc(DWORD flags, DWORD bytes) {
    (void)flags;
    
    if (bytes == 0) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    void* ptr = kmalloc(bytes);
    if (!ptr) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return (HLOCAL)ptr;
}

HLOCAL WINAPI LocalFree(HLOCAL mem) {
    if (!mem) {
        return NULL;
    }
    
    kfree((void*)mem);
    winapi_set_last_error(ERROR_SUCCESS);
    return NULL;
}

/* ========== File Management Functions ========== */

HANDLE WINAPI CreateFileA(LPCSTR filename, DWORD access, DWORD share_mode,
                          void* security, DWORD creation, DWORD flags, HANDLE template_file) {
    (void)share_mode;
    (void)security;
    (void)flags;
    (void)template_file;
    
    if (!filename) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    
    vga_write("Kernel32: CreateFileA called for: ");
    vga_write(filename);
    vga_write("\n");
    
    /* Convert Windows access flags to VFS flags */
    int vfs_flags = 0;
    if (access & GENERIC_READ) {
        vfs_flags |= O_RDONLY;
    }
    if (access & GENERIC_WRITE) {
        vfs_flags |= O_WRONLY;
    }
    if ((access & GENERIC_READ) && (access & GENERIC_WRITE)) {
        vfs_flags = O_RDWR;
    }
    
    /* Handle creation disposition */
    switch (creation) {
        case CREATE_NEW:
        case CREATE_ALWAYS:
            vfs_flags |= O_CREAT;
            break;
        case OPEN_EXISTING:
            /* No additional flags needed */
            break;
        case OPEN_ALWAYS:
            vfs_flags |= O_CREAT;
            break;
        case TRUNCATE_EXISTING:
            vfs_flags |= O_TRUNC;
            break;
        default:
            break;
    }
    
    /* Open file using VFS */
    int vfs_fd = vfs_open(filename, vfs_flags);
    if (vfs_fd < 0) {
        /* Try to create if O_CREAT was set */
        if (vfs_flags & O_CREAT) {
            if (vfs_create(filename) == 0) {
                vfs_fd = vfs_open(filename, vfs_flags & ~O_CREAT);
            }
        }
        
        if (vfs_fd < 0) {
            winapi_set_last_error(ERROR_FILE_NOT_FOUND);
            return INVALID_HANDLE_VALUE;
        }
    }
    
    /* Allocate Windows handle for VFS fd */
    HANDLE handle = alloc_file_handle(vfs_fd);
    if (handle == INVALID_HANDLE_VALUE) {
        vfs_close(vfs_fd);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return handle;
}

BOOL WINAPI ReadFile(HANDLE file, LPVOID buffer, DWORD bytes_to_read,
                     DWORD* bytes_read, void* overlapped) {
    (void)overlapped;
    
    if (bytes_read) {
        *bytes_read = 0;
    }
    
    if (!buffer) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Get VFS file descriptor */
    int vfs_fd = get_vfs_fd(file);
    if (vfs_fd < 0) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* Read from VFS */
    int result = vfs_read(vfs_fd, buffer, bytes_to_read);
    if (result < 0) {
        winapi_set_last_error(ERROR_READ_FAULT);
        return FALSE;
    }
    
    if (bytes_read) {
        *bytes_read = (DWORD)result;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI WriteFile(HANDLE file, LPCVOID buffer, DWORD bytes_to_write,
                      DWORD* bytes_written, void* overlapped) {
    (void)overlapped;
    
    if (bytes_written) {
        *bytes_written = 0;
    }
    
    if (!buffer) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Get VFS file descriptor */
    int vfs_fd = get_vfs_fd(file);
    if (vfs_fd < 0) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* Write to VFS */
    int result = vfs_write(vfs_fd, buffer, bytes_to_write);
    if (result < 0) {
        winapi_set_last_error(ERROR_WRITE_FAULT);
        return FALSE;
    }
    
    if (bytes_written) {
        *bytes_written = (DWORD)result;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI CloseHandle(HANDLE handle) {
    /* Check for pseudo-handles */
    if (handle == (HANDLE)0xFFFFFFFF || handle == (HANDLE)0xFFFFFFFE) {
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    /* Check for standard handles */
    if (handle == (HANDLE)0x10 || handle == (HANDLE)0x11 || handle == (HANDLE)0x12) {
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    /* Get VFS file descriptor */
    int vfs_fd = get_vfs_fd(handle);
    if (vfs_fd >= 0) {
        vfs_close(vfs_fd);
        free_file_handle(handle);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetFileSize(HANDLE file, DWORD* high_size) {
    if (high_size) {
        *high_size = 0;
    }
    
    /* Get VFS file descriptor */
    int vfs_fd = get_vfs_fd(file);
    if (vfs_fd < 0) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return 0xFFFFFFFF;
    }
    
    /* Get file info using stat */
    /* For now, we use seek to determine size */
    long current_pos = vfs_seek(vfs_fd, 0, SEEK_CUR);
    if (current_pos < 0) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return 0xFFFFFFFF;
    }
    
    long file_size = vfs_seek(vfs_fd, 0, SEEK_END);
    vfs_seek(vfs_fd, current_pos, SEEK_SET); /* Restore position */
    
    if (file_size < 0) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return 0xFFFFFFFF;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return (DWORD)file_size;
}

BOOL WINAPI DeleteFileA(LPCSTR filename) {
    if (!filename) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    vga_write("Kernel32: DeleteFileA called for: ");
    vga_write(filename);
    vga_write("\n");
    
    int result = vfs_unlink(filename);
    if (result < 0) {
        winapi_set_last_error(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ========== Console Functions ========== */

HANDLE WINAPI GetStdHandle(DWORD std_handle) {
    /* Return special handle values for standard streams */
    switch (std_handle) {
        case STD_INPUT_HANDLE:
            return (HANDLE)0x10;
        case STD_OUTPUT_HANDLE:
            return (HANDLE)0x11;
        case STD_ERROR_HANDLE:
            return (HANDLE)0x12;
        default:
            winapi_set_last_error(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
    }
}

BOOL WINAPI WriteConsoleA(HANDLE console, LPCVOID buffer, DWORD length,
                          DWORD* written, void* reserved) {
    (void)console;
    (void)reserved;
    
    if (!buffer || length == 0) {
        if (written) *written = 0;
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Write to VGA console */
    const char* str = (const char*)buffer;
    for (DWORD i = 0; i < length && str[i]; i++) {
        char c[2] = {str[i], '\0'};
        vga_write(c);
    }
    
    if (written) {
        *written = length;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ========== String Functions ========== */

int WINAPI lstrlenA(LPCSTR string) {
    return k32_strlen(string);
}

LPSTR WINAPI lstrcpyA(LPSTR dest, LPCSTR src) {
    k32_strcpy(dest, src);
    return dest;
}

LPSTR WINAPI lstrcatA(LPSTR dest, LPCSTR src) {
    char* p = dest;
    while (*p) p++;
    k32_strcpy(p, src);
    return dest;
}

int WINAPI lstrcmpA(LPCSTR str1, LPCSTR str2) {
    return k32_strcmp(str1, str2);
}

/* ========== Module Loading Functions ========== */

HMODULE WINAPI LoadLibraryA(LPCSTR lib_filename) {
    if (!lib_filename) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    vga_write("Kernel32: LoadLibraryA called for: ");
    vga_write(lib_filename);
    vga_write("\n");
    
    HMODULE handle = dll_load(lib_filename);
    if (!handle) {
        winapi_set_last_error(ERROR_FILE_NOT_FOUND);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return handle;
}

BOOL WINAPI FreeLibrary(HMODULE module) {
    if (!module) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    int result = dll_free(module);
    if (result != 0) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

FARPROC WINAPI GetProcAddress(HMODULE module, LPCSTR proc_name) {
    if (!module || !proc_name) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    void* proc = dll_get_proc_address(module, proc_name);
    if (!proc) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return (FARPROC)proc;
}

HMODULE WINAPI GetModuleHandleA(LPCSTR module_name) {
    if (!module_name) {
        /* Return handle to current executable */
        return (HMODULE)0x400000;
    }
    
    HMODULE handle = dll_get_module_handle(module_name);
    if (!handle) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return handle;
}

/* ========== Error Functions ========== */

DWORD WINAPI GetLastError(void) {
    return winapi_get_last_error();
}

void WINAPI SetLastError(DWORD error_code) {
    winapi_set_last_error(error_code);
}

/* ========== System Information Functions ========== */

/* SYSTEM_INFO structure */
typedef struct {
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
} SYSTEM_INFO;

void WINAPI GetSystemInfo(void* system_info) {
    if (!system_info) {
        return;
    }
    
    SYSTEM_INFO* info = (SYSTEM_INFO*)system_info;
    
    /* Fill in system information */
    info->wProcessorArchitecture = 0; /* PROCESSOR_ARCHITECTURE_INTEL */
    info->wReserved = 0;
    info->dwPageSize = 4096;
    info->lpMinimumApplicationAddress = (LPVOID)0x10000;
    info->lpMaximumApplicationAddress = (LPVOID)0x7FFEFFFF;
    info->dwActiveProcessorMask = 1;
    info->dwNumberOfProcessors = 1;
    info->dwProcessorType = 586; /* PROCESSOR_INTEL_PENTIUM */
    info->dwAllocationGranularity = 65536;
    info->wProcessorLevel = 6;
    info->wProcessorRevision = 0;
    
    vga_write("Kernel32: GetSystemInfo called\n");
}

/* Static tick counter - would be updated by timer interrupt */
static volatile DWORD g_tick_count = 0;

DWORD WINAPI GetTickCount(void) {
    /* Read PIT counter for more accurate timing */
    /* PIT runs at approximately 1.193182 MHz */
    /* Each tick is approximately 1/1000 of a second when configured properly */
    
    /* For now, use a simple incrementing counter */
    /* In a real implementation, this would be updated by the timer interrupt handler */
    g_tick_count++;
    
    return g_tick_count;
}

/* Function to update tick count from timer interrupt */
void kernel32_update_tick_count(DWORD delta_ms) {
    g_tick_count += delta_ms;
}
