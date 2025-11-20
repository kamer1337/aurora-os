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
#include <stddef.h>

/* Simple handle counter */
static DWORD g_next_handle = 0x1000;

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

/**
 * Initialize Kernel32.dll compatibility layer
 */
void kernel32_init(void) {
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
    (void)filename;
    (void)access;
    (void)share_mode;
    (void)security;
    (void)creation;
    (void)flags;
    (void)template_file;
    
    /* Stub implementation - return invalid handle */
    vga_write("Kernel32: CreateFileA called for: ");
    if (filename) vga_write(filename);
    vga_write("\n");
    
    winapi_set_last_error(ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

BOOL WINAPI ReadFile(HANDLE file, LPVOID buffer, DWORD bytes_to_read,
                     DWORD* bytes_read, void* overlapped) {
    (void)file;
    (void)buffer;
    (void)bytes_to_read;
    (void)overlapped;
    
    if (bytes_read) {
        *bytes_read = 0;
    }
    
    winapi_set_last_error(ERROR_INVALID_HANDLE);
    return FALSE;
}

BOOL WINAPI WriteFile(HANDLE file, LPCVOID buffer, DWORD bytes_to_write,
                      DWORD* bytes_written, void* overlapped) {
    (void)file;
    (void)buffer;
    (void)bytes_to_write;
    (void)overlapped;
    
    if (bytes_written) {
        *bytes_written = 0;
    }
    
    winapi_set_last_error(ERROR_INVALID_HANDLE);
    return FALSE;
}

BOOL WINAPI CloseHandle(HANDLE handle) {
    (void)handle;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetFileSize(HANDLE file, DWORD* high_size) {
    (void)file;
    
    if (high_size) {
        *high_size = 0;
    }
    
    winapi_set_last_error(ERROR_INVALID_HANDLE);
    return 0xFFFFFFFF;
}

BOOL WINAPI DeleteFileA(LPCSTR filename) {
    (void)filename;
    
    vga_write("Kernel32: DeleteFileA called\n");
    winapi_set_last_error(ERROR_FILE_NOT_FOUND);
    return FALSE;
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

void WINAPI GetSystemInfo(void* system_info) {
    (void)system_info;
    /* Stub implementation */
    vga_write("Kernel32: GetSystemInfo called\n");
}

DWORD WINAPI GetTickCount(void) {
    /* Return stub tick count */
    return 0;
}
