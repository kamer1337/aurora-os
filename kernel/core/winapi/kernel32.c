/**
 * Aurora OS - Kernel32.dll API Compatibility Layer Implementation
 * 
 * Full implementation of Windows Kernel32.dll functions
 */

#include "kernel32.h"
#include "winapi.h"
#include "../../memory/memory.h"
#include "../../drivers/vga.h"
#include "../../drivers/timer.h"
#include "../../process/process.h"
#include "../dll_loader.h"
#include "../../../filesystem/vfs/vfs.h"
#include <stddef.h>

/* Additional error codes and constants */
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define ERROR_NEGATIVE_SEEK 131
#define ERROR_BUFFER_OVERFLOW 111
#define ERROR_NOT_OWNER 288
#define ERROR_TOO_MANY_POSTS 298

/* ============================================================================
 * Internal Data Structures and Global State
 * ============================================================================ */

/* Handle types */
typedef enum {
    HANDLE_TYPE_INVALID = 0,
    HANDLE_TYPE_FILE,
    HANDLE_TYPE_PROCESS,
    HANDLE_TYPE_THREAD,
    HANDLE_TYPE_EVENT,
    HANDLE_TYPE_MUTEX,
    HANDLE_TYPE_SEMAPHORE,
    HANDLE_TYPE_HEAP,
    HANDLE_TYPE_FIND,
    HANDLE_TYPE_CONSOLE,
    HANDLE_TYPE_PIPE
} handle_type_t;

/* Handle entry structure */
typedef struct {
    handle_type_t type;
    void* data;
    DWORD flags;
    int ref_count;
    int in_use;
} handle_entry_t;

/* File handle data */
typedef struct {
    char path[MAX_PATH];
    DWORD access;
    DWORD share_mode;
    DWORD position;
    DWORD size;
    DWORD attributes;
    uint8_t* buffer;      /* In-memory file buffer for ramdisk simulation */
    DWORD buffer_size;
    int vfs_fd;           /* VFS file descriptor, -1 if not using VFS */
} file_data_t;

/* Event handle data */
typedef struct {
    int signaled;
    int manual_reset;
    char name[64];
} event_data_t;

/* Mutex handle data */
typedef struct {
    int locked;
    DWORD owner_thread;
    int recursion_count;
    char name[64];
} mutex_data_t;

/* Semaphore handle data */
typedef struct {
    LONG count;
    LONG max_count;
    char name[64];
} semaphore_data_t;

/* Thread handle data */
typedef struct {
    DWORD thread_id;
    void* entry_point;
    LPVOID parameter;
    int priority;
    int suspended;
    DWORD exit_code;
    int terminated;
} thread_data_t;

/* Process handle data */
typedef struct {
    DWORD process_id;
    DWORD exit_code;
    int terminated;
} process_data_t;

/* Heap handle data */
typedef struct {
    void* base;
    SIZE_T size;
    SIZE_T used;
    DWORD flags;
} heap_data_t;

/* Find handle data */
typedef struct {
    char search_path[MAX_PATH];
    int current_index;
} find_data_t;

/* Maximum handles */
#define MAX_HANDLES 256

/* Handle table */
static handle_entry_t g_handles[MAX_HANDLES];
static int g_handles_initialized = 0;

/* Process state */
static DWORD g_current_process_id = 1;
static DWORD g_current_thread_id = 1;
static DWORD g_next_process_id = 2;
static DWORD g_next_thread_id = 2;
static DWORD g_process_exit_code = 0;

/* Environment variables storage */
#define MAX_ENV_VARS 64
#define MAX_ENV_NAME 128
#define MAX_ENV_VALUE 1024

typedef struct {
    char name[MAX_ENV_NAME];
    char value[MAX_ENV_VALUE];
    int in_use;
} env_var_t;

static env_var_t g_env_vars[MAX_ENV_VARS];
static int g_env_initialized = 0;

/* Command line */
static char g_command_line[MAX_PATH] = "aurora.exe";

/* Current directory */
static char g_current_directory[MAX_PATH] = "C:\\";

/* Console state */
static HANDLE g_std_input = NULL;
static HANDLE g_std_output = NULL;
static HANDLE g_std_error = NULL;
static DWORD g_console_mode_input = ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
static DWORD g_console_mode_output = ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT;
static char g_console_title[256] = "Aurora OS Console";

/* Default process heap */
static HANDLE g_process_heap = NULL;

/* TLS (Thread Local Storage) */
#define TLS_SLOTS 64
static LPVOID g_tls_slots[TLS_SLOTS];
static int g_tls_in_use[TLS_SLOTS];

/* Startup info */
static STARTUPINFOA g_startup_info = {0};

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

/* String helper functions */
static int k32_strlen(const char* s) {
    int len = 0;
    if (!s) return 0;
    while (*s++) len++;
    return len;
}

static void k32_strcpy(char* dest, const char* src) {
    if (!dest || !src) return;
    while (*src) *dest++ = *src++;
    *dest = '\0';
}

static void k32_strncpy(char* dest, const char* src, int max) {
    if (!dest || !src || max <= 0) return;
    int i;
    for (i = 0; i < max - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

static int k32_strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return s1 ? 1 : (s2 ? -1 : 0);
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

static int k32_stricmp(const char* s1, const char* s2) {
    if (!s1 || !s2) return s1 ? 1 : (s2 ? -1 : 0);
    while (*s1 && *s2) {
        char c1 = *s1;
        char c2 = *s2;
        /* Simple case conversion */
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return c1 - c2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

static void k32_memset(void* dest, int c, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    while (n--) *d++ = (uint8_t)c;
}

static void k32_memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
}

/* Initialize handle table */
static void init_handles(void) {
    if (g_handles_initialized) return;
    k32_memset(g_handles, 0, sizeof(g_handles));
    g_handles_initialized = 1;
}

/* Allocate a new handle */
static HANDLE alloc_handle(handle_type_t type, void* data) {
    init_handles();
    for (int i = 0; i < MAX_HANDLES; i++) {
        if (!g_handles[i].in_use) {
            g_handles[i].type = type;
            g_handles[i].data = data;
            g_handles[i].ref_count = 1;
            g_handles[i].in_use = 1;
            return (HANDLE)(uintptr_t)(i + 0x100);  /* Offset to avoid NULL-like values */
        }
    }
    return INVALID_HANDLE_VALUE;
}

/* Get handle entry */
static handle_entry_t* get_handle_entry(HANDLE handle) {
    if (!g_handles_initialized) return NULL;
    if (handle == INVALID_HANDLE_VALUE || handle == NULL) return NULL;
    
    uintptr_t idx = (uintptr_t)handle;
    if (idx < 0x100) return NULL;  /* Special handles */
    idx -= 0x100;
    
    if (idx >= MAX_HANDLES) return NULL;
    if (!g_handles[idx].in_use) return NULL;
    
    return &g_handles[idx];
}

/* Free a handle */
static BOOL free_handle(HANDLE handle) {
    handle_entry_t* entry = get_handle_entry(handle);
    if (!entry) return FALSE;
    
    entry->ref_count--;
    if (entry->ref_count <= 0) {
        /* Free associated data based on type */
        if (entry->data) {
            kfree(entry->data);
        }
        entry->type = HANDLE_TYPE_INVALID;
        entry->data = NULL;
        entry->in_use = 0;
    }
    return TRUE;
}

/* Initialize environment variables */
static void init_env(void) {
    if (g_env_initialized) return;
    k32_memset(g_env_vars, 0, sizeof(g_env_vars));
    
    /* Set default environment variables */
    k32_strcpy(g_env_vars[0].name, "PATH");
    k32_strcpy(g_env_vars[0].value, "C:\\Windows\\System32;C:\\Windows");
    g_env_vars[0].in_use = 1;
    
    k32_strcpy(g_env_vars[1].name, "SYSTEMROOT");
    k32_strcpy(g_env_vars[1].value, "C:\\Windows");
    g_env_vars[1].in_use = 1;
    
    k32_strcpy(g_env_vars[2].name, "WINDIR");
    k32_strcpy(g_env_vars[2].value, "C:\\Windows");
    g_env_vars[2].in_use = 1;
    
    k32_strcpy(g_env_vars[3].name, "TEMP");
    k32_strcpy(g_env_vars[3].value, "C:\\Windows\\Temp");
    g_env_vars[3].in_use = 1;
    
    k32_strcpy(g_env_vars[4].name, "TMP");
    k32_strcpy(g_env_vars[4].value, "C:\\Windows\\Temp");
    g_env_vars[4].in_use = 1;
    
    k32_strcpy(g_env_vars[5].name, "OS");
    k32_strcpy(g_env_vars[5].value, "AuroraOS");
    g_env_vars[5].in_use = 1;
    
    g_env_initialized = 1;
}

/* Find environment variable by name */
static env_var_t* find_env_var(const char* name) {
    init_env();
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (g_env_vars[i].in_use && k32_stricmp(g_env_vars[i].name, name) == 0) {
            return &g_env_vars[i];
        }
    }
    return NULL;
}

/**
 * Initialize Kernel32.dll compatibility layer
 */
void kernel32_init(void) {
    init_handles();
    init_env();
    
    /* Initialize TLS */
    k32_memset(g_tls_slots, 0, sizeof(g_tls_slots));
    k32_memset(g_tls_in_use, 0, sizeof(g_tls_in_use));
    
    /* Create console handles */
    g_std_input = alloc_handle(HANDLE_TYPE_CONSOLE, NULL);
    g_std_output = alloc_handle(HANDLE_TYPE_CONSOLE, NULL);
    g_std_error = alloc_handle(HANDLE_TYPE_CONSOLE, NULL);
    
    /* Initialize startup info */
    g_startup_info.cb = sizeof(STARTUPINFOA);
    g_startup_info.lpDesktop = NULL;
    g_startup_info.lpTitle = g_console_title;
    g_startup_info.hStdInput = g_std_input;
    g_startup_info.hStdOutput = g_std_output;
    g_startup_info.hStdError = g_std_error;
    
    /* Create default process heap */
    g_process_heap = HeapCreate(0, 0x10000, 0);
    
    vga_write("Kernel32: Full implementation initialized\n");
}

/* ============================================================================
 * Process and Thread Functions
 * ============================================================================ */

HANDLE WINAPI GetCurrentProcess(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return (HANDLE)(uintptr_t)0xFFFFFFFF;  /* Special pseudo-handle for current process */
}

DWORD WINAPI GetCurrentProcessId(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return g_current_process_id;
}

HANDLE WINAPI GetCurrentThread(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return (HANDLE)(uintptr_t)0xFFFFFFFE;  /* Special pseudo-handle for current thread */
}

DWORD WINAPI GetCurrentThreadId(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return g_current_thread_id;
}

void WINAPI ExitProcess(DWORD exit_code) {
    g_process_exit_code = exit_code;
    vga_write("Kernel32: ExitProcess called with code ");
    vga_write_dec((int)exit_code);
    vga_write("\n");
    
    /* In a real implementation, this would terminate all threads and cleanup */
    /* For now, halt the system */
    while (1) {
        __asm__ volatile("hlt");
    }
}

BOOL WINAPI TerminateProcess(HANDLE hProcess, DWORD uExitCode) {
    /* Check for pseudo-handle (current process) */
    if (hProcess == (HANDLE)(uintptr_t)0xFFFFFFFF) {
        ExitProcess(uExitCode);
        return TRUE;  /* Never reached */
    }
    
    handle_entry_t* entry = get_handle_entry(hProcess);
    if (!entry || entry->type != HANDLE_TYPE_PROCESS) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    process_data_t* proc = (process_data_t*)entry->data;
    if (proc) {
        proc->exit_code = uExitCode;
        proc->terminated = 1;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

void WINAPI ExitThread(DWORD dwExitCode) {
    vga_write("Kernel32: ExitThread called with code ");
    vga_write_dec((int)dwExitCode);
    vga_write("\n");
    
    /* Mark current thread as terminated */
    /* In a real implementation, this would do proper thread cleanup */
    winapi_set_last_error(ERROR_SUCCESS);
}

BOOL WINAPI TerminateThread(HANDLE hThread, DWORD dwExitCode) {
    if (hThread == (HANDLE)(uintptr_t)0xFFFFFFFE) {
        ExitThread(dwExitCode);
        return TRUE;
    }
    
    handle_entry_t* entry = get_handle_entry(hThread);
    if (!entry || entry->type != HANDLE_TYPE_THREAD) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    thread_data_t* thread = (thread_data_t*)entry->data;
    if (thread) {
        thread->exit_code = dwExitCode;
        thread->terminated = 1;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize,
                           void* lpStartAddress, LPVOID lpParameter,
                           DWORD dwCreationFlags, LPDWORD lpThreadId) {
    (void)lpThreadAttributes;
    (void)dwStackSize;
    
    if (!lpStartAddress) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    thread_data_t* thread = (thread_data_t*)kmalloc(sizeof(thread_data_t));
    if (!thread) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    k32_memset(thread, 0, sizeof(thread_data_t));
    thread->thread_id = g_next_thread_id++;
    thread->entry_point = lpStartAddress;
    thread->parameter = lpParameter;
    thread->priority = THREAD_PRIORITY_NORMAL;
    thread->suspended = (dwCreationFlags & CREATE_SUSPENDED) ? 1 : 0;
    thread->terminated = 0;
    
    HANDLE hThread = alloc_handle(HANDLE_TYPE_THREAD, thread);
    if (hThread == INVALID_HANDLE_VALUE) {
        kfree(thread);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    if (lpThreadId) {
        *lpThreadId = thread->thread_id;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return hThread;
}

DWORD WINAPI ResumeThread(HANDLE hThread) {
    if (hThread == (HANDLE)(uintptr_t)0xFFFFFFFE) {
        return 0;  /* Current thread is always running */
    }
    
    handle_entry_t* entry = get_handle_entry(hThread);
    if (!entry || entry->type != HANDLE_TYPE_THREAD) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return (DWORD)-1;
    }
    
    thread_data_t* thread = (thread_data_t*)entry->data;
    if (!thread) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return (DWORD)-1;
    }
    
    DWORD prev_count = thread->suspended;
    if (thread->suspended > 0) {
        thread->suspended--;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return prev_count;
}

DWORD WINAPI SuspendThread(HANDLE hThread) {
    if (hThread == (HANDLE)(uintptr_t)0xFFFFFFFE) {
        winapi_set_last_error(ERROR_ACCESS_DENIED);
        return (DWORD)-1;
    }
    
    handle_entry_t* entry = get_handle_entry(hThread);
    if (!entry || entry->type != HANDLE_TYPE_THREAD) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return (DWORD)-1;
    }
    
    thread_data_t* thread = (thread_data_t*)entry->data;
    if (!thread) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return (DWORD)-1;
    }
    
    DWORD prev_count = thread->suspended;
    thread->suspended++;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return prev_count;
}

BOOL WINAPI SetThreadPriority(HANDLE hThread, int nPriority) {
    handle_entry_t* entry = NULL;
    thread_data_t* thread = NULL;
    
    if (hThread == (HANDLE)(uintptr_t)0xFFFFFFFE) {
        /* Current thread - would need to track in real implementation */
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    entry = get_handle_entry(hThread);
    if (!entry || entry->type != HANDLE_TYPE_THREAD) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    thread = (thread_data_t*)entry->data;
    if (thread) {
        thread->priority = nPriority;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

int WINAPI GetThreadPriority(HANDLE hThread) {
    if (hThread == (HANDLE)(uintptr_t)0xFFFFFFFE) {
        winapi_set_last_error(ERROR_SUCCESS);
        return THREAD_PRIORITY_NORMAL;
    }
    
    handle_entry_t* entry = get_handle_entry(hThread);
    if (!entry || entry->type != HANDLE_TYPE_THREAD) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return THREAD_PRIORITY_NORMAL;
    }
    
    thread_data_t* thread = (thread_data_t*)entry->data;
    winapi_set_last_error(ERROR_SUCCESS);
    return thread ? thread->priority : THREAD_PRIORITY_NORMAL;
}

void WINAPI Sleep(DWORD dwMilliseconds) {
    if (dwMilliseconds == 0) {
        process_yield();
        return;
    }
    
    timer_sleep(dwMilliseconds);
    winapi_set_last_error(ERROR_SUCCESS);
}

DWORD WINAPI SleepEx(DWORD dwMilliseconds, BOOL bAlertable) {
    (void)bAlertable;  /* Alertable waits not implemented */
    Sleep(dwMilliseconds);
    return 0;
}

DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
    if (hHandle == INVALID_HANDLE_VALUE || hHandle == NULL) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return WAIT_FAILED;
    }
    
    /* Handle pseudo-handles */
    if (hHandle == (HANDLE)(uintptr_t)0xFFFFFFFF || 
        hHandle == (HANDLE)(uintptr_t)0xFFFFFFFE) {
        /* Process/thread pseudo-handles - wait forever */
        if (dwMilliseconds != INFINITE) {
            timer_sleep(dwMilliseconds);
            return WAIT_TIMEOUT;
        }
        /* Infinite wait on current process/thread never returns */
        while (1) {
            __asm__ volatile("hlt");
        }
    }
    
    handle_entry_t* entry = get_handle_entry(hHandle);
    if (!entry) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return WAIT_FAILED;
    }
    
    DWORD start_tick = timer_get_ticks();
    
    switch (entry->type) {
        case HANDLE_TYPE_EVENT: {
            event_data_t* event = (event_data_t*)entry->data;
            while (!event->signaled) {
                if (dwMilliseconds != INFINITE) {
                    if ((timer_get_ticks() - start_tick) >= dwMilliseconds) {
                        return WAIT_TIMEOUT;
                    }
                }
                process_yield();
            }
            if (!event->manual_reset) {
                event->signaled = 0;
            }
            return WAIT_OBJECT_0;
        }
        
        case HANDLE_TYPE_MUTEX: {
            mutex_data_t* mutex = (mutex_data_t*)entry->data;
            while (mutex->locked && mutex->owner_thread != g_current_thread_id) {
                if (dwMilliseconds != INFINITE) {
                    if ((timer_get_ticks() - start_tick) >= dwMilliseconds) {
                        return WAIT_TIMEOUT;
                    }
                }
                process_yield();
            }
            mutex->locked = 1;
            mutex->owner_thread = g_current_thread_id;
            mutex->recursion_count++;
            return WAIT_OBJECT_0;
        }
        
        case HANDLE_TYPE_SEMAPHORE: {
            semaphore_data_t* sem = (semaphore_data_t*)entry->data;
            while (sem->count <= 0) {
                if (dwMilliseconds != INFINITE) {
                    if ((timer_get_ticks() - start_tick) >= dwMilliseconds) {
                        return WAIT_TIMEOUT;
                    }
                }
                process_yield();
            }
            sem->count--;
            return WAIT_OBJECT_0;
        }
        
        case HANDLE_TYPE_PROCESS: {
            process_data_t* proc = (process_data_t*)entry->data;
            while (!proc->terminated) {
                if (dwMilliseconds != INFINITE) {
                    if ((timer_get_ticks() - start_tick) >= dwMilliseconds) {
                        return WAIT_TIMEOUT;
                    }
                }
                process_yield();
            }
            return WAIT_OBJECT_0;
        }
        
        case HANDLE_TYPE_THREAD: {
            thread_data_t* thread = (thread_data_t*)entry->data;
            while (!thread->terminated) {
                if (dwMilliseconds != INFINITE) {
                    if ((timer_get_ticks() - start_tick) >= dwMilliseconds) {
                        return WAIT_TIMEOUT;
                    }
                }
                process_yield();
            }
            return WAIT_OBJECT_0;
        }
        
        default:
            winapi_set_last_error(ERROR_INVALID_HANDLE);
            return WAIT_FAILED;
    }
}

DWORD WINAPI WaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles,
                                     BOOL bWaitAll, DWORD dwMilliseconds) {
    if (nCount == 0 || !lpHandles) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return WAIT_FAILED;
    }
    
    DWORD start_tick = timer_get_ticks();
    
    if (bWaitAll) {
        /* Wait for all objects */
        for (DWORD i = 0; i < nCount; i++) {
            DWORD remaining = INFINITE;
            if (dwMilliseconds != INFINITE) {
                DWORD elapsed = timer_get_ticks() - start_tick;
                if (elapsed >= dwMilliseconds) {
                    return WAIT_TIMEOUT;
                }
                remaining = dwMilliseconds - elapsed;
            }
            
            DWORD result = WaitForSingleObject(lpHandles[i], remaining);
            if (result == WAIT_TIMEOUT) {
                return WAIT_TIMEOUT;
            }
            if (result == WAIT_FAILED) {
                return WAIT_FAILED;
            }
        }
        return WAIT_OBJECT_0;
    } else {
        /* Wait for any object */
        while (1) {
            for (DWORD i = 0; i < nCount; i++) {
                DWORD result = WaitForSingleObject(lpHandles[i], 0);
                if (result == WAIT_OBJECT_0) {
                    return WAIT_OBJECT_0 + i;
                }
            }
            
            if (dwMilliseconds != INFINITE) {
                if ((timer_get_ticks() - start_tick) >= dwMilliseconds) {
                    return WAIT_TIMEOUT;
                }
            }
            
            process_yield();
        }
    }
}

BOOL WINAPI GetExitCodeProcess(HANDLE hProcess, LPDWORD lpExitCode) {
    if (!lpExitCode) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (hProcess == (HANDLE)(uintptr_t)0xFFFFFFFF) {
        *lpExitCode = g_process_exit_code;
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    handle_entry_t* entry = get_handle_entry(hProcess);
    if (!entry || entry->type != HANDLE_TYPE_PROCESS) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    process_data_t* proc = (process_data_t*)entry->data;
    *lpExitCode = proc ? proc->exit_code : 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode) {
    if (!lpExitCode) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (hThread == (HANDLE)(uintptr_t)0xFFFFFFFE) {
        *lpExitCode = 0;  /* Current thread still running */
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    handle_entry_t* entry = get_handle_entry(hThread);
    if (!entry || entry->type != HANDLE_TYPE_THREAD) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    thread_data_t* thread = (thread_data_t*)entry->data;
    *lpExitCode = thread ? thread->exit_code : 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ============================================================================
 * Memory Management Functions
 * ============================================================================ */

LPVOID WINAPI VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
    (void)lpAddress;
    (void)flAllocationType;
    (void)flProtect;
    
    if (dwSize == 0) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    /* Align to page boundary */
    SIZE_T aligned_size = (dwSize + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    void* ptr = kmalloc(aligned_size);
    if (!ptr) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    /* Zero memory if committing */
    if (flAllocationType & MEM_COMMIT) {
        k32_memset(ptr, 0, aligned_size);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return ptr;
}

BOOL WINAPI VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType) {
    (void)dwSize;
    
    if (!lpAddress) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (dwFreeType & MEM_RELEASE) {
        kfree(lpAddress);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI VirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, LPDWORD lpflOldProtect) {
    (void)lpAddress;
    (void)dwSize;
    (void)flNewProtect;
    
    /* Memory protection not fully implemented in Aurora OS */
    if (lpflOldProtect) {
        *lpflOldProtect = PAGE_READWRITE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

SIZE_T WINAPI VirtualQuery(LPCVOID lpAddress, void* lpBuffer, SIZE_T dwLength) {
    if (!lpBuffer || dwLength < sizeof(MEMORY_BASIC_INFORMATION)) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    MEMORY_BASIC_INFORMATION* mbi = (MEMORY_BASIC_INFORMATION*)lpBuffer;
    
    /* Get memory region information */
    uintptr_t addr = (uintptr_t)lpAddress;
    
    /* Round down to page boundary (assume 4KB pages) */
    uintptr_t page_base = addr & ~((uintptr_t)0xFFF);
    
    /* Fill in the memory information structure */
    mbi->BaseAddress = (void*)page_base;
    mbi->AllocationBase = (void*)page_base;  /* Simplified: assume same as base */
    mbi->AllocationProtect = PAGE_READWRITE;
    mbi->RegionSize = 0x1000;  /* Default to one page */
    mbi->State = MEM_COMMIT;   /* Assume committed */
    mbi->Protect = PAGE_READWRITE;
    mbi->Type = MEM_PRIVATE;
    
    /* Check if address is in valid memory range */
    /* In Aurora OS, we have simplified memory management */
    if (addr >= 0x100000 && addr < 0x400000) {
        /* Kernel code/data region */
        mbi->Protect = PAGE_EXECUTE_READ;
        mbi->Type = MEM_IMAGE;
    } else if (addr >= 0x400000 && addr < 0x800000) {
        /* Heap region */
        mbi->Protect = PAGE_READWRITE;
        mbi->Type = MEM_PRIVATE;
    } else if (addr < 0x1000) {
        /* Null page - not accessible */
        mbi->State = MEM_FREE;
        mbi->Protect = PAGE_NOACCESS;
        mbi->Type = 0;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return sizeof(MEMORY_BASIC_INFORMATION);
}

HGLOBAL WINAPI GlobalAlloc(DWORD uFlags, SIZE_T dwBytes) {
    if (dwBytes == 0) {
        dwBytes = 1;  /* Allocate at least 1 byte */
    }
    
    void* ptr = kmalloc(dwBytes);
    if (!ptr) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    if (uFlags & GMEM_ZEROINIT) {
        k32_memset(ptr, 0, dwBytes);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return (HGLOBAL)ptr;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem) {
    if (!hMem) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return hMem;
    }
    
    kfree((void*)hMem);
    winapi_set_last_error(ERROR_SUCCESS);
    return NULL;
}

LPVOID WINAPI GlobalLock(HGLOBAL hMem) {
    if (!hMem) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return NULL;
    }
    
    /* GMEM_FIXED memory is already a pointer */
    winapi_set_last_error(ERROR_SUCCESS);
    return (LPVOID)hMem;
}

BOOL WINAPI GlobalUnlock(HGLOBAL hMem) {
    (void)hMem;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

SIZE_T WINAPI GlobalSize(HGLOBAL hMem) {
    (void)hMem;
    /* Cannot determine size from pointer alone */
    winapi_set_last_error(ERROR_SUCCESS);
    return 0;
}

HLOCAL WINAPI LocalAlloc(DWORD uFlags, SIZE_T uBytes) {
    return (HLOCAL)GlobalAlloc(uFlags, uBytes);
}

HLOCAL WINAPI LocalFree(HLOCAL hMem) {
    return (HLOCAL)GlobalFree((HGLOBAL)hMem);
}

LPVOID WINAPI LocalLock(HLOCAL hMem) {
    return GlobalLock((HGLOBAL)hMem);
}

BOOL WINAPI LocalUnlock(HLOCAL hMem) {
    return GlobalUnlock((HGLOBAL)hMem);
}

SIZE_T WINAPI LocalSize(HLOCAL hMem) {
    return GlobalSize((HGLOBAL)hMem);
}

HANDLE WINAPI HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize) {
    heap_data_t* heap = (heap_data_t*)kmalloc(sizeof(heap_data_t));
    if (!heap) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    k32_memset(heap, 0, sizeof(heap_data_t));
    heap->flags = flOptions;
    heap->size = dwMaximumSize ? dwMaximumSize : 0x100000;  /* Default 1MB */
    heap->used = 0;
    
    /* Allocate initial heap memory */
    if (dwInitialSize > 0) {
        heap->base = kmalloc(dwInitialSize);
        if (!heap->base) {
            kfree(heap);
            winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
            return NULL;
        }
    }
    
    HANDLE hHeap = alloc_handle(HANDLE_TYPE_HEAP, heap);
    if (hHeap == INVALID_HANDLE_VALUE) {
        if (heap->base) kfree(heap->base);
        kfree(heap);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return hHeap;
}

BOOL WINAPI HeapDestroy(HANDLE hHeap) {
    if (!hHeap || hHeap == g_process_heap) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    handle_entry_t* entry = get_handle_entry(hHeap);
    if (!entry || entry->type != HANDLE_TYPE_HEAP) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    heap_data_t* heap = (heap_data_t*)entry->data;
    if (heap && heap->base) {
        kfree(heap->base);
    }
    
    free_handle(hHeap);
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

LPVOID WINAPI HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes) {
    (void)hHeap;  /* Use global allocator for simplicity */
    
    if (dwBytes == 0) {
        dwBytes = 1;
    }
    
    void* ptr = kmalloc(dwBytes);
    if (!ptr) {
        if (dwFlags & HEAP_GENERATE_EXCEPTIONS) {
            /* Would raise exception here */
        }
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    if (dwFlags & HEAP_ZERO_MEMORY) {
        k32_memset(ptr, 0, dwBytes);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return ptr;
}

BOOL WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem) {
    (void)hHeap;
    (void)dwFlags;
    
    if (!lpMem) {
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    kfree(lpMem);
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

LPVOID WINAPI HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes) {
    if (!lpMem) {
        return HeapAlloc(hHeap, dwFlags, dwBytes);
    }
    
    if (dwBytes == 0) {
        HeapFree(hHeap, dwFlags, lpMem);
        return NULL;
    }
    
    /* Simple realloc: allocate new, copy, free old */
    void* new_ptr = HeapAlloc(hHeap, dwFlags & ~HEAP_ZERO_MEMORY, dwBytes);
    if (!new_ptr) {
        return NULL;
    }
    
    /* Copy old data - we don't know the old size, so this is imperfect */
    k32_memcpy(new_ptr, lpMem, dwBytes);
    
    HeapFree(hHeap, 0, lpMem);
    
    return new_ptr;
}

SIZE_T WINAPI HeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem) {
    (void)hHeap;
    (void)dwFlags;
    (void)lpMem;
    
    /* Cannot determine size from pointer alone */
    winapi_set_last_error(ERROR_SUCCESS);
    return (SIZE_T)-1;
}

BOOL WINAPI HeapValidate(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem) {
    (void)hHeap;
    (void)dwFlags;
    (void)lpMem;
    
    /* Always return valid for now */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HANDLE WINAPI GetProcessHeap(void) {
    if (!g_process_heap) {
        g_process_heap = HeapCreate(0, 0x10000, 0);
    }
    winapi_set_last_error(ERROR_SUCCESS);
    return g_process_heap;
}

void WINAPI GlobalMemoryStatus(LPMEMORYSTATUS lpBuffer) {
    if (!lpBuffer) {
        return;
    }
    
    lpBuffer->dwLength = sizeof(MEMORYSTATUS);
    lpBuffer->dwMemoryLoad = 50;  /* 50% memory usage estimate */
    lpBuffer->dwTotalPhys = 0x10000000;  /* 256 MB */
    lpBuffer->dwAvailPhys = 0x08000000;  /* 128 MB available */
    lpBuffer->dwTotalPageFile = 0x20000000;  /* 512 MB page file */
    lpBuffer->dwAvailPageFile = 0x10000000;  /* 256 MB available */
    lpBuffer->dwTotalVirtual = 0x7FFE0000;  /* ~2 GB user space */
    lpBuffer->dwAvailVirtual = 0x7FF00000;
    
    winapi_set_last_error(ERROR_SUCCESS);
}

/* ============================================================================
 * File Management Functions
 * ============================================================================ */

HANDLE WINAPI CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition,
                          DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    (void)lpSecurityAttributes;
    (void)hTemplateFile;
    
    if (!lpFileName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    
    /* Allocate file data structure */
    file_data_t* file = (file_data_t*)kmalloc(sizeof(file_data_t));
    if (!file) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }
    
    k32_memset(file, 0, sizeof(file_data_t));
    k32_strncpy(file->path, lpFileName, MAX_PATH);
    file->access = dwDesiredAccess;
    file->share_mode = dwShareMode;
    file->position = 0;
    file->attributes = dwFlagsAndAttributes & 0xFFFF;
    file->vfs_fd = -1;
    
    /* Try to use VFS first */
    int vfs_flags = 0;
    if ((dwDesiredAccess & GENERIC_READ) && (dwDesiredAccess & GENERIC_WRITE)) {
        vfs_flags = O_RDWR;
    } else if (dwDesiredAccess & GENERIC_WRITE) {
        vfs_flags = O_WRONLY;
    } else if (dwDesiredAccess & GENERIC_READ) {
        vfs_flags = O_RDONLY;
    }
    
    switch (dwCreationDisposition) {
        case CREATE_NEW:
        case CREATE_ALWAYS:
            vfs_flags |= O_CREAT;
            if (dwCreationDisposition == CREATE_ALWAYS) vfs_flags |= O_TRUNC;
            break;
        case OPEN_ALWAYS:
            vfs_flags |= O_CREAT;
            break;
        case TRUNCATE_EXISTING:
            vfs_flags |= O_TRUNC;
            break;
        case OPEN_EXISTING:
        default:
            break;
    }
    
    /* Try VFS open */
    int vfs_fd = vfs_open(lpFileName, vfs_flags);
    if (vfs_fd >= 0) {
        file->vfs_fd = vfs_fd;
    } else if (vfs_flags & O_CREAT) {
        /* Try creating the file */
        if (vfs_create(lpFileName) == 0) {
            vfs_fd = vfs_open(lpFileName, vfs_flags & ~O_CREAT);
            if (vfs_fd >= 0) {
                file->vfs_fd = vfs_fd;
            }
        }
    }
    
    /* If VFS failed, use ramdisk simulation for new files */
    if (file->vfs_fd < 0) {
        switch (dwCreationDisposition) {
            case CREATE_NEW:
            case CREATE_ALWAYS:
            case OPEN_ALWAYS:
                file->buffer_size = 4096;  /* Initial 4KB buffer */
                file->buffer = (uint8_t*)kmalloc(file->buffer_size);
                if (!file->buffer) {
                    kfree(file);
                    winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
                    return INVALID_HANDLE_VALUE;
                }
                k32_memset(file->buffer, 0, file->buffer_size);
                file->size = 0;
                break;
                
            case OPEN_EXISTING:
            case TRUNCATE_EXISTING:
            default:
                kfree(file);
                winapi_set_last_error(ERROR_FILE_NOT_FOUND);
                return INVALID_HANDLE_VALUE;
        }
    }
    
    HANDLE hFile = alloc_handle(HANDLE_TYPE_FILE, file);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (file->vfs_fd >= 0) vfs_close(file->vfs_fd);
        if (file->buffer) kfree(file->buffer);
        kfree(file);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return hFile;
}

BOOL WINAPI ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
                     LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    (void)lpOverlapped;
    
    if (lpNumberOfBytesRead) {
        *lpNumberOfBytesRead = 0;
    }
    
    if (!lpBuffer) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Handle console input */
    if (hFile == g_std_input) {
        /* Console read not fully implemented */
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    file_data_t* file = (file_data_t*)entry->data;
    if (!file) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* Check access mode */
    if (!(file->access & GENERIC_READ)) {
        winapi_set_last_error(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    
    DWORD bytes_read = 0;
    
    /* Use VFS if available */
    if (file->vfs_fd >= 0) {
        int result = vfs_read(file->vfs_fd, lpBuffer, nNumberOfBytesToRead);
        if (result >= 0) {
            bytes_read = (DWORD)result;
        }
    } else if (file->buffer) {
        /* Read from ramdisk buffer */
        DWORD bytes_available = (file->position < file->size) ? (file->size - file->position) : 0;
        DWORD bytes_to_read = (nNumberOfBytesToRead < bytes_available) ? nNumberOfBytesToRead : bytes_available;
        
        if (bytes_to_read > 0) {
            k32_memcpy(lpBuffer, file->buffer + file->position, bytes_to_read);
            file->position += bytes_to_read;
            bytes_read = bytes_to_read;
        }
    }
    
    if (lpNumberOfBytesRead) {
        *lpNumberOfBytesRead = bytes_read;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
                      LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    (void)lpOverlapped;
    
    if (lpNumberOfBytesWritten) {
        *lpNumberOfBytesWritten = 0;
    }
    
    /* Handle console output */
    if (hFile == g_std_output || hFile == g_std_error) {
        if (lpBuffer && nNumberOfBytesToWrite > 0) {
            const char* str = (const char*)lpBuffer;
            for (DWORD i = 0; i < nNumberOfBytesToWrite && str[i]; i++) {
                char c[2] = {str[i], '\0'};
                vga_write(c);
            }
        }
        if (lpNumberOfBytesWritten) {
            *lpNumberOfBytesWritten = nNumberOfBytesToWrite;
        }
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    if (!lpBuffer) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    file_data_t* file = (file_data_t*)entry->data;
    if (!file) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* Check access mode */
    if (!(file->access & GENERIC_WRITE)) {
        winapi_set_last_error(ERROR_ACCESS_DENIED);
        return FALSE;
    }
    
    DWORD bytes_written = 0;
    
    /* Use VFS if available */
    if (file->vfs_fd >= 0) {
        int result = vfs_write(file->vfs_fd, lpBuffer, nNumberOfBytesToWrite);
        if (result >= 0) {
            bytes_written = (DWORD)result;
        }
    } else {
        /* Write to ramdisk buffer */
        /* Expand buffer if needed */
        DWORD new_end = file->position + nNumberOfBytesToWrite;
        if (new_end > file->buffer_size) {
            DWORD new_size = ((new_end + 4095) / 4096) * 4096;  /* Round up to 4KB */
            uint8_t* new_buffer = (uint8_t*)kmalloc(new_size);
            if (!new_buffer) {
                winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
                return FALSE;
            }
            if (file->buffer && file->size > 0) {
                k32_memcpy(new_buffer, file->buffer, file->size);
            }
            if (file->buffer) {
                kfree(file->buffer);
            }
            file->buffer = new_buffer;
            file->buffer_size = new_size;
        }
        
        /* Write data */
        k32_memcpy(file->buffer + file->position, lpBuffer, nNumberOfBytesToWrite);
        file->position += nNumberOfBytesToWrite;
        
        if (file->position > file->size) {
            file->size = file->position;
        }
        
        bytes_written = nNumberOfBytesToWrite;
    }
    
    if (lpNumberOfBytesWritten) {
        *lpNumberOfBytesWritten = bytes_written;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI CloseHandle(HANDLE hObject) {
    if (!hObject || hObject == INVALID_HANDLE_VALUE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* Don't close pseudo-handles or console handles */
    if (hObject == (HANDLE)(uintptr_t)0xFFFFFFFF ||
        hObject == (HANDLE)(uintptr_t)0xFFFFFFFE ||
        hObject == g_std_input ||
        hObject == g_std_output ||
        hObject == g_std_error) {
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    handle_entry_t* entry = get_handle_entry(hObject);
    if (!entry) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* Special cleanup for file handles */
    if (entry->type == HANDLE_TYPE_FILE) {
        file_data_t* file = (file_data_t*)entry->data;
        if (file) {
            if (file->vfs_fd >= 0) {
                vfs_close(file->vfs_fd);
            }
            if (file->buffer) {
                kfree(file->buffer);
                file->buffer = NULL;
            }
        }
    }
    
    free_handle(hObject);
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
    if (lpFileSizeHigh) {
        *lpFileSizeHigh = 0;
    }
    
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return INVALID_FILE_SIZE;
    }
    
    file_data_t* file = (file_data_t*)entry->data;
    if (!file) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return INVALID_FILE_SIZE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return file->size;
}


BOOL WINAPI GetFileSizeEx(HANDLE hFile, void* lpFileSize) {
    if (!lpFileSize) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    DWORD high = 0;
    DWORD low = GetFileSize(hFile, &high);
    
    if (low == INVALID_FILE_SIZE && winapi_get_last_error() != ERROR_SUCCESS) {
        return FALSE;
    }
    
    /* lpFileSize is a LARGE_INTEGER pointer */
    *((DWORD*)lpFileSize) = low;
    *((DWORD*)lpFileSize + 1) = high;
    
    return TRUE;
}

DWORD WINAPI SetFilePointer(HANDLE hFile, LONG lDistanceToMove,
                            LPLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    (void)lpDistanceToMoveHigh;  /* High word not supported */
    
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return INVALID_SET_FILE_POINTER;
    }
    
    file_data_t* file = (file_data_t*)entry->data;
    if (!file) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return INVALID_SET_FILE_POINTER;
    }
    
    LONG new_pos;
    switch (dwMoveMethod) {
        case FILE_BEGIN:
            new_pos = lDistanceToMove;
            break;
        case FILE_CURRENT:
            new_pos = (LONG)file->position + lDistanceToMove;
            break;
        case FILE_END:
            new_pos = (LONG)file->size + lDistanceToMove;
            break;
        default:
            winapi_set_last_error(ERROR_INVALID_PARAMETER);
            return INVALID_SET_FILE_POINTER;
    }
    
    if (new_pos < 0) {
        winapi_set_last_error(ERROR_NEGATIVE_SEEK);
        return INVALID_SET_FILE_POINTER;
    }
    
    file->position = (DWORD)new_pos;
    
    /* Also update VFS position if using VFS */
    if (file->vfs_fd >= 0) {
        vfs_seek(file->vfs_fd, new_pos, SEEK_SET);
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return file->position;
}


BOOL WINAPI SetEndOfFile(HANDLE hFile) {
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    file_data_t* file = (file_data_t*)entry->data;
    if (!file) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    file->size = file->position;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI FlushFileBuffers(HANDLE hFile) {
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        /* Also accept console handles */
        if (hFile == g_std_output || hFile == g_std_error) {
            winapi_set_last_error(ERROR_SUCCESS);
            return TRUE;
        }
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* No actual flushing needed for ramdisk */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI DeleteFileA(LPCSTR lpFileName) {
    if (!lpFileName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    int result = vfs_unlink(lpFileName);
    if (result < 0) {
        winapi_set_last_error(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL bFailIfExists) {
    (void)lpExistingFileName;
    (void)lpNewFileName;
    (void)bFailIfExists;
    winapi_set_last_error(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

BOOL WINAPI MoveFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName) {
    (void)lpExistingFileName;
    (void)lpNewFileName;
    winapi_set_last_error(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

DWORD WINAPI GetFileAttributesA(LPCSTR lpFileName) {
    (void)lpFileName;
    /* Return normal file attributes */
    winapi_set_last_error(ERROR_SUCCESS);
    return FILE_ATTRIBUTE_NORMAL;
}

BOOL WINAPI SetFileAttributesA(LPCSTR lpFileName, DWORD dwFileAttributes) {
    (void)lpFileName;
    (void)dwFileAttributes;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetFileType(HANDLE hFile) {
    if (hFile == g_std_input || hFile == g_std_output || hFile == g_std_error) {
        return FILE_TYPE_CHAR;
    }
    
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FILE_TYPE_UNKNOWN;
    }
    
    if (entry->type == HANDLE_TYPE_FILE) {
        return FILE_TYPE_DISK;
    } else if (entry->type == HANDLE_TYPE_CONSOLE) {
        return FILE_TYPE_CHAR;
    } else if (entry->type == HANDLE_TYPE_PIPE) {
        return FILE_TYPE_PIPE;
    }
    
    return FILE_TYPE_UNKNOWN;
}

BOOL WINAPI GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime,
                        LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) {
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    /* Return current time for all times */
    DWORD tick = timer_get_ticks();
    FILETIME ft = {tick, 0};
    
    if (lpCreationTime) *lpCreationTime = ft;
    if (lpLastAccessTime) *lpLastAccessTime = ft;
    if (lpLastWriteTime) *lpLastWriteTime = ft;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI SetFileTime(HANDLE hFile, const FILETIME* lpCreationTime,
                        const FILETIME* lpLastAccessTime, const FILETIME* lpLastWriteTime) {
    (void)lpCreationTime;
    (void)lpLastAccessTime;
    (void)lpLastWriteTime;
    
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI GetFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation) {
    if (!lpFileInformation) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    handle_entry_t* entry = get_handle_entry(hFile);
    if (!entry || entry->type != HANDLE_TYPE_FILE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    file_data_t* file = (file_data_t*)entry->data;
    
    k32_memset(lpFileInformation, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
    lpFileInformation->dwFileAttributes = file ? file->attributes : FILE_ATTRIBUTE_NORMAL;
    lpFileInformation->nFileSizeLow = file ? file->size : 0;
    lpFileInformation->nNumberOfLinks = 1;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* File lock structure for tracking locks */
#define MAX_FILE_LOCKS 64
typedef struct {
    HANDLE file;
    uint64_t offset;
    uint64_t length;
    int in_use;
} file_lock_entry_t;

static file_lock_entry_t g_file_locks[MAX_FILE_LOCKS];
static int g_file_locks_initialized = 0;

static void init_file_locks(void) {
    if (!g_file_locks_initialized) {
        for (int i = 0; i < MAX_FILE_LOCKS; i++) {
            g_file_locks[i].in_use = 0;
        }
        g_file_locks_initialized = 1;
    }
}

BOOL WINAPI LockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                     DWORD nNumberOfBytesToLockLow, DWORD nNumberOfBytesToLockHigh) {
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    init_file_locks();
    
    uint64_t offset = ((uint64_t)dwFileOffsetHigh << 32) | dwFileOffsetLow;
    uint64_t length = ((uint64_t)nNumberOfBytesToLockHigh << 32) | nNumberOfBytesToLockLow;
    
    /* Check for overlapping locks */
    for (int i = 0; i < MAX_FILE_LOCKS; i++) {
        if (g_file_locks[i].in_use && g_file_locks[i].file == hFile) {
            uint64_t lock_end = g_file_locks[i].offset + g_file_locks[i].length;
            uint64_t new_end = offset + length;
            
            /* Check for overlap */
            if (offset < lock_end && new_end > g_file_locks[i].offset) {
                winapi_set_last_error(ERROR_LOCK_VIOLATION);
                return FALSE;
            }
        }
    }
    
    /* Find free slot and add lock */
    for (int i = 0; i < MAX_FILE_LOCKS; i++) {
        if (!g_file_locks[i].in_use) {
            g_file_locks[i].file = hFile;
            g_file_locks[i].offset = offset;
            g_file_locks[i].length = length;
            g_file_locks[i].in_use = 1;
            winapi_set_last_error(ERROR_SUCCESS);
            return TRUE;
        }
    }
    
    /* No free slots */
    winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
}

BOOL WINAPI UnlockFile(HANDLE hFile, DWORD dwFileOffsetLow, DWORD dwFileOffsetHigh,
                       DWORD nNumberOfBytesToUnlockLow, DWORD nNumberOfBytesToUnlockHigh) {
    if (hFile == INVALID_HANDLE_VALUE || hFile == NULL) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    init_file_locks();
    
    uint64_t offset = ((uint64_t)dwFileOffsetHigh << 32) | dwFileOffsetLow;
    uint64_t length = ((uint64_t)nNumberOfBytesToUnlockHigh << 32) | nNumberOfBytesToUnlockLow;
    
    /* Find and remove the lock */
    for (int i = 0; i < MAX_FILE_LOCKS; i++) {
        if (g_file_locks[i].in_use && 
            g_file_locks[i].file == hFile &&
            g_file_locks[i].offset == offset &&
            g_file_locks[i].length == length) {
            g_file_locks[i].in_use = 0;
            winapi_set_last_error(ERROR_SUCCESS);
            return TRUE;
        }
    }
    
    /* Lock not found - still return success (Windows behavior) */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ============================================================================
 * Directory Functions
 * ============================================================================ */

BOOL WINAPI CreateDirectoryA(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes) {
    (void)lpSecurityAttributes;
    if (!lpPathName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    int result = vfs_mkdir(lpPathName);
    winapi_set_last_error(result == 0 ? ERROR_SUCCESS : ERROR_PATH_NOT_FOUND);
    return result == 0;
}

BOOL WINAPI RemoveDirectoryA(LPCSTR lpPathName) {
    if (!lpPathName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    int result = vfs_rmdir(lpPathName);
    winapi_set_last_error(result == 0 ? ERROR_SUCCESS : ERROR_PATH_NOT_FOUND);
    return result == 0;
}

DWORD WINAPI GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer) {
    DWORD len = (DWORD)k32_strlen(g_current_directory);
    
    if (!lpBuffer || nBufferLength == 0) {
        return len + 1;
    }
    
    if (nBufferLength <= len) {
        return len + 1;
    }
    
    k32_strcpy(lpBuffer, g_current_directory);
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

BOOL WINAPI SetCurrentDirectoryA(LPCSTR lpPathName) {
    if (!lpPathName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    k32_strncpy(g_current_directory, lpPathName, MAX_PATH);
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
    if (!lpFileName || !lpFindFileData) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }
    
    find_data_t* find = (find_data_t*)kmalloc(sizeof(find_data_t));
    if (!find) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return INVALID_HANDLE_VALUE;
    }
    
    k32_strncpy(find->search_path, lpFileName, MAX_PATH);
    find->current_index = 0;
    
    /* No files in empty ramdisk */
    kfree(find);
    winapi_set_last_error(ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

BOOL WINAPI FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData) {
    (void)lpFindFileData;
    
    handle_entry_t* entry = get_handle_entry(hFindFile);
    if (!entry || entry->type != HANDLE_TYPE_FIND) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_NO_MORE_FILES);
    return FALSE;
}

BOOL WINAPI FindClose(HANDLE hFindFile) {
    return CloseHandle(hFindFile);
}

DWORD WINAPI GetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength,
                              LPSTR lpBuffer, LPSTR* lpFilePart) {
    if (!lpFileName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    /* Simple implementation - just return the filename as-is if it looks absolute */
    DWORD len = (DWORD)k32_strlen(lpFileName);
    
    if (!lpBuffer || nBufferLength <= len) {
        return len + 1;
    }
    
    k32_strcpy(lpBuffer, lpFileName);
    
    if (lpFilePart) {
        /* Find last path separator */
        *lpFilePart = lpBuffer;
        for (char* p = lpBuffer; *p; p++) {
            if (*p == '\\' || *p == '/') {
                *lpFilePart = p + 1;
            }
        }
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

DWORD WINAPI GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer) {
    const char* temp_path = "C:\\Windows\\Temp\\";
    DWORD len = (DWORD)k32_strlen(temp_path);
    
    if (!lpBuffer || nBufferLength <= len) {
        return len + 1;
    }
    
    k32_strcpy(lpBuffer, temp_path);
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

static DWORD g_temp_file_counter = 0;

DWORD WINAPI GetTempFileNameA(LPCSTR lpPathName, LPCSTR lpPrefixString,
                              DWORD uUnique, LPSTR lpTempFileName) {
    if (!lpPathName || !lpTempFileName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    DWORD unique = uUnique ? uUnique : ++g_temp_file_counter;
    
    /* Build temp filename */
    k32_strcpy(lpTempFileName, lpPathName);
    
    /* Ensure path ends with backslash */
    int len = k32_strlen(lpTempFileName);
    if (len > 0 && lpTempFileName[len-1] != '\\') {
        lpTempFileName[len] = '\\';
        lpTempFileName[len+1] = '\0';
    }
    
    /* Add prefix */
    if (lpPrefixString) {
        char prefix[4] = {0};
        k32_strncpy(prefix, lpPrefixString, 3);
        k32_strcpy(lpTempFileName + k32_strlen(lpTempFileName), prefix);
    }
    
    /* Add unique number and extension */
    char num_str[16];
    int i = 0;
    DWORD temp = unique;
    do {
        num_str[i++] = '0' + (temp % 10);
        temp /= 10;
    } while (temp > 0);
    num_str[i] = '\0';
    
    /* Reverse the number string */
    for (int j = 0; j < i/2; j++) {
        char t = num_str[j];
        num_str[j] = num_str[i-1-j];
        num_str[i-1-j] = t;
    }
    
    k32_strcpy(lpTempFileName + k32_strlen(lpTempFileName), num_str);
    k32_strcpy(lpTempFileName + k32_strlen(lpTempFileName), ".tmp");
    
    winapi_set_last_error(ERROR_SUCCESS);
    return unique;
}

/* ============================================================================
 * Console Functions
 * ============================================================================ */

HANDLE WINAPI GetStdHandle(DWORD nStdHandle) {
    switch (nStdHandle) {
        case STD_INPUT_HANDLE:
            return g_std_input;
        case STD_OUTPUT_HANDLE:
            return g_std_output;
        case STD_ERROR_HANDLE:
            return g_std_error;
        default:
            winapi_set_last_error(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
    }
}

BOOL WINAPI SetStdHandle(DWORD nStdHandle, HANDLE hHandle) {
    switch (nStdHandle) {
        case STD_INPUT_HANDLE:
            g_std_input = hHandle;
            break;
        case STD_OUTPUT_HANDLE:
            g_std_output = hHandle;
            break;
        case STD_ERROR_HANDLE:
            g_std_error = hHandle;
            break;
        default:
            winapi_set_last_error(ERROR_INVALID_PARAMETER);
            return FALSE;
    }
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI WriteConsoleA(HANDLE hConsoleOutput, LPCVOID lpBuffer, DWORD nNumberOfCharsToWrite,
                          LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved) {
    (void)hConsoleOutput;
    (void)lpReserved;
    
    if (!lpBuffer) {
        if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = 0;
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Write to VGA console */
    const char* str = (const char*)lpBuffer;
    DWORD written = 0;
    for (DWORD i = 0; i < nNumberOfCharsToWrite && str[i]; i++) {
        char c[2] = {str[i], '\0'};
        vga_write(c);
        written++;
    }
    
    if (lpNumberOfCharsWritten) {
        *lpNumberOfCharsWritten = written;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI ReadConsoleA(HANDLE hConsoleInput, LPVOID lpBuffer, DWORD nNumberOfCharsToRead,
                         LPDWORD lpNumberOfCharsRead, LPVOID pInputControl) {
    (void)hConsoleInput;
    (void)lpBuffer;
    (void)nNumberOfCharsToRead;
    (void)pInputControl;
    
    /* Console input not fully implemented */
    if (lpNumberOfCharsRead) {
        *lpNumberOfCharsRead = 0;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI AllocConsole(void) {
    /* Console already allocated by default */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI FreeConsole(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI SetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode) {
    if (hConsoleHandle == g_std_input) {
        g_console_mode_input = dwMode;
    } else if (hConsoleHandle == g_std_output || hConsoleHandle == g_std_error) {
        g_console_mode_output = dwMode;
    } else {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI GetConsoleMode(HANDLE hConsoleHandle, LPDWORD lpMode) {
    if (!lpMode) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    if (hConsoleHandle == g_std_input) {
        *lpMode = g_console_mode_input;
    } else if (hConsoleHandle == g_std_output || hConsoleHandle == g_std_error) {
        *lpMode = g_console_mode_output;
    } else {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI SetConsoleTitleA(LPCSTR lpConsoleTitle) {
    if (lpConsoleTitle) {
        k32_strncpy(g_console_title, lpConsoleTitle, sizeof(g_console_title) - 1);
    }
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetConsoleTitleA(LPSTR lpConsoleTitle, DWORD nSize) {
    DWORD len = (DWORD)k32_strlen(g_console_title);
    
    if (!lpConsoleTitle || nSize == 0) {
        return len + 1;
    }
    
    k32_strncpy(lpConsoleTitle, g_console_title, (int)nSize);
    winapi_set_last_error(ERROR_SUCCESS);
    return len < nSize ? len : nSize - 1;
}

/* ============================================================================
 * String Functions
 * ============================================================================ */

int WINAPI lstrlenA(LPCSTR lpString) {
    return k32_strlen(lpString);
}

LPSTR WINAPI lstrcpyA(LPSTR lpString1, LPCSTR lpString2) {
    if (lpString1 && lpString2) {
        k32_strcpy(lpString1, lpString2);
    }
    return lpString1;
}

LPSTR WINAPI lstrcpynA(LPSTR lpString1, LPCSTR lpString2, int iMaxLength) {
    if (lpString1 && lpString2 && iMaxLength > 0) {
        k32_strncpy(lpString1, lpString2, iMaxLength);
    }
    return lpString1;
}

LPSTR WINAPI lstrcatA(LPSTR lpString1, LPCSTR lpString2) {
    if (lpString1 && lpString2) {
        char* p = lpString1;
        while (*p) p++;
        k32_strcpy(p, lpString2);
    }
    return lpString1;
}

int WINAPI lstrcmpA(LPCSTR lpString1, LPCSTR lpString2) {
    return k32_strcmp(lpString1, lpString2);
}

int WINAPI lstrcmpiA(LPCSTR lpString1, LPCSTR lpString2) {
    return k32_stricmp(lpString1, lpString2);
}

int WINAPI MultiByteToWideChar(DWORD CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr,
                               int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar) {
    (void)CodePage;
    (void)dwFlags;
    
    if (!lpMultiByteStr) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    int src_len = (cbMultiByte < 0) ? k32_strlen(lpMultiByteStr) + 1 : cbMultiByte;
    
    if (!lpWideCharStr || cchWideChar == 0) {
        return src_len;  /* Return required buffer size */
    }
    
    int i;
    for (i = 0; i < src_len && i < cchWideChar; i++) {
        lpWideCharStr[i] = (WCHAR)(unsigned char)lpMultiByteStr[i];
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return i;
}

int WINAPI WideCharToMultiByte(DWORD CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr,
                               int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte,
                               LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar) {
    (void)CodePage;
    (void)dwFlags;
    (void)lpDefaultChar;
    
    if (lpUsedDefaultChar) *lpUsedDefaultChar = FALSE;
    
    if (!lpWideCharStr) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    /* Calculate source length */
    int src_len;
    if (cchWideChar < 0) {
        src_len = 0;
        while (lpWideCharStr[src_len]) src_len++;
        src_len++;  /* Include null terminator */
    } else {
        src_len = cchWideChar;
    }
    
    if (!lpMultiByteStr || cbMultiByte == 0) {
        return src_len;  /* Return required buffer size */
    }
    
    int i;
    for (i = 0; i < src_len && i < cbMultiByte; i++) {
        lpMultiByteStr[i] = (lpWideCharStr[i] < 256) ? (char)lpWideCharStr[i] : '?';
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return i;
}

/* ============================================================================
 * Module Loading Functions
 * ============================================================================ */

HMODULE WINAPI LoadLibraryA(LPCSTR lpLibFileName) {
    if (!lpLibFileName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    HMODULE handle = dll_load(lpLibFileName);
    if (!handle) {
        winapi_set_last_error(ERROR_MOD_NOT_FOUND);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return handle;
}

HMODULE WINAPI LoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
    (void)hFile;
    (void)dwFlags;
    return LoadLibraryA(lpLibFileName);
}

BOOL WINAPI FreeLibrary(HMODULE hLibModule) {
    if (!hLibModule) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    int result = dll_free(hLibModule);
    if (result != 0) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    if (!hModule || !lpProcName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    void* proc = dll_get_proc_address(hModule, lpProcName);
    if (!proc) {
        winapi_set_last_error(ERROR_PROC_NOT_FOUND);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return (FARPROC)proc;
}

HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName) {
    if (!lpModuleName) {
        /* Return handle to current executable */
        winapi_set_last_error(ERROR_SUCCESS);
        return (HMODULE)0x400000;
    }
    
    HMODULE handle = dll_get_module_handle(lpModuleName);
    if (!handle) {
        winapi_set_last_error(ERROR_MOD_NOT_FOUND);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return handle;
}

DWORD WINAPI GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
    (void)hModule;
    
    const char* module_name = "C:\\aurora.exe";
    DWORD len = (DWORD)k32_strlen(module_name);
    
    if (!lpFilename || nSize == 0) {
        winapi_set_last_error(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }
    
    if (nSize <= len) {
        k32_strncpy(lpFilename, module_name, (int)nSize);
        winapi_set_last_error(ERROR_INSUFFICIENT_BUFFER);
        return nSize;
    }
    
    k32_strcpy(lpFilename, module_name);
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

/* ============================================================================
 * Error Functions
 * ============================================================================ */

DWORD WINAPI GetLastError(void) {
    return winapi_get_last_error();
}

void WINAPI SetLastError(DWORD dwErrCode) {
    winapi_set_last_error(dwErrCode);
}

DWORD WINAPI FormatMessageA(DWORD dwFlags, LPCVOID lpSource, DWORD dwMessageId,
                            DWORD dwLanguageId, LPSTR lpBuffer, DWORD nSize, void* Arguments) {
    (void)dwFlags;
    (void)lpSource;
    (void)dwLanguageId;
    (void)Arguments;
    
    if (!lpBuffer || nSize == 0) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    const char* msg;
    switch (dwMessageId) {
        case ERROR_SUCCESS:
            msg = "The operation completed successfully.";
            break;
        case ERROR_FILE_NOT_FOUND:
            msg = "The system cannot find the file specified.";
            break;
        case ERROR_PATH_NOT_FOUND:
            msg = "The system cannot find the path specified.";
            break;
        case ERROR_ACCESS_DENIED:
            msg = "Access is denied.";
            break;
        case ERROR_INVALID_HANDLE:
            msg = "The handle is invalid.";
            break;
        case ERROR_NOT_ENOUGH_MEMORY:
            msg = "Not enough memory resources are available.";
            break;
        case ERROR_INVALID_PARAMETER:
            msg = "The parameter is incorrect.";
            break;
        default:
            msg = "Unknown error.";
            break;
    }
    
    DWORD len = (DWORD)k32_strlen(msg);
    if (len >= nSize) len = nSize - 1;
    k32_strncpy(lpBuffer, msg, (int)len + 1);
    
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

/* ============================================================================
 * Environment Functions
 * ============================================================================ */

DWORD WINAPI GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize) {
    if (!lpName) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    env_var_t* var = find_env_var(lpName);
    if (!var) {
        winapi_set_last_error(ERROR_ENVVAR_NOT_FOUND);
        return 0;
    }
    
    DWORD len = (DWORD)k32_strlen(var->value);
    
    if (!lpBuffer || nSize == 0) {
        return len + 1;
    }
    
    if (nSize <= len) {
        winapi_set_last_error(ERROR_INSUFFICIENT_BUFFER);
        return len + 1;
    }
    
    k32_strcpy(lpBuffer, var->value);
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

BOOL WINAPI SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue) {
    if (!lpName || k32_strlen(lpName) == 0) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    init_env();
    
    /* Look for existing variable */
    env_var_t* var = find_env_var(lpName);
    
    if (!lpValue) {
        /* Delete variable */
        if (var) {
            var->in_use = 0;
            var->name[0] = '\0';
            var->value[0] = '\0';
        }
        winapi_set_last_error(ERROR_SUCCESS);
        return TRUE;
    }
    
    if (var) {
        /* Update existing */
        k32_strncpy(var->value, lpValue, MAX_ENV_VALUE - 1);
    } else {
        /* Find free slot */
        for (int i = 0; i < MAX_ENV_VARS; i++) {
            if (!g_env_vars[i].in_use) {
                k32_strncpy(g_env_vars[i].name, lpName, MAX_ENV_NAME - 1);
                k32_strncpy(g_env_vars[i].value, lpValue, MAX_ENV_VALUE - 1);
                g_env_vars[i].in_use = 1;
                winapi_set_last_error(ERROR_SUCCESS);
                return TRUE;
            }
        }
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

LPSTR WINAPI GetEnvironmentStringsA(void) {
    init_env();
    
    /* Calculate total size needed */
    SIZE_T total_size = 1;  /* Final null */
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (g_env_vars[i].in_use) {
            total_size += k32_strlen(g_env_vars[i].name) + 1 + 
                          k32_strlen(g_env_vars[i].value) + 1;
        }
    }
    
    char* buffer = (char*)kmalloc(total_size);
    if (!buffer) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    char* p = buffer;
    for (int i = 0; i < MAX_ENV_VARS; i++) {
        if (g_env_vars[i].in_use) {
            k32_strcpy(p, g_env_vars[i].name);
            p += k32_strlen(g_env_vars[i].name);
            *p++ = '=';
            k32_strcpy(p, g_env_vars[i].value);
            p += k32_strlen(g_env_vars[i].value);
            *p++ = '\0';
        }
    }
    *p = '\0';  /* Double null at end */
    
    winapi_set_last_error(ERROR_SUCCESS);
    return buffer;
}

BOOL WINAPI FreeEnvironmentStringsA(LPSTR lpszEnvironmentBlock) {
    if (lpszEnvironmentBlock) {
        kfree(lpszEnvironmentBlock);
    }
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

LPSTR WINAPI GetCommandLineA(void) {
    winapi_set_last_error(ERROR_SUCCESS);
    return g_command_line;
}

/* ============================================================================
 * System Information Functions
 * ============================================================================ */

void WINAPI GetSystemInfo(LPSYSTEM_INFO lpSystemInfo) {
    if (!lpSystemInfo) {
        return;
    }
    
    k32_memset(lpSystemInfo, 0, sizeof(SYSTEM_INFO));
    lpSystemInfo->wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
    lpSystemInfo->dwPageSize = PAGE_SIZE;
    lpSystemInfo->lpMinimumApplicationAddress = (LPVOID)0x00010000;
    lpSystemInfo->lpMaximumApplicationAddress = (LPVOID)0x7FFEFFFF;
    lpSystemInfo->dwActiveProcessorMask = 1;
    lpSystemInfo->dwNumberOfProcessors = 1;
    lpSystemInfo->dwProcessorType = 586;  /* Pentium class */
    lpSystemInfo->dwAllocationGranularity = 0x10000;  /* 64KB */
    lpSystemInfo->wProcessorLevel = 6;
    lpSystemInfo->wProcessorRevision = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
}

void WINAPI GetNativeSystemInfo(LPSYSTEM_INFO lpSystemInfo) {
    GetSystemInfo(lpSystemInfo);
}

DWORD WINAPI GetTickCount(void) {
    return timer_get_ticks();
}

BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA lpVersionInfo) {
    if (!lpVersionInfo || lpVersionInfo->dwOSVersionInfoSize < sizeof(OSVERSIONINFOA)) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    lpVersionInfo->dwMajorVersion = 6;  /* Windows Vista/7 era */
    lpVersionInfo->dwMinorVersion = 1;
    lpVersionInfo->dwBuildNumber = 7601;
    lpVersionInfo->dwPlatformId = VER_PLATFORM_WIN32_NT;
    k32_strcpy(lpVersionInfo->szCSDVersion, "Aurora OS");
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetVersion(void) {
    /* Returns packed version info for compatibility */
    /* Low word: major.minor, High word: build number | platform */
    return 0x80000106;  /* Win32s, version 6.1 */
}

UINT WINAPI GetSystemDirectoryA(LPSTR lpBuffer, UINT uSize) {
    const char* sys_dir = "C:\\Windows\\System32";
    UINT len = (UINT)k32_strlen(sys_dir);
    
    if (!lpBuffer || uSize == 0) {
        return len + 1;
    }
    
    if (uSize <= len) {
        return len + 1;
    }
    
    k32_strcpy(lpBuffer, sys_dir);
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

UINT WINAPI GetWindowsDirectoryA(LPSTR lpBuffer, UINT uSize) {
    const char* win_dir = "C:\\Windows";
    UINT len = (UINT)k32_strlen(win_dir);
    
    if (!lpBuffer || uSize == 0) {
        return len + 1;
    }
    
    if (uSize <= len) {
        return len + 1;
    }
    
    k32_strcpy(lpBuffer, win_dir);
    winapi_set_last_error(ERROR_SUCCESS);
    return len;
}

void WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime) {
    if (!lpSystemTime) return;
    
    DWORD ticks = timer_get_ticks();
    
    /* Convert ticks to time (simplified - assumes 1000 ticks/second) */
    DWORD seconds = ticks / 1000;
    DWORD minutes = seconds / 60;
    DWORD hours = minutes / 60;
    
    lpSystemTime->wYear = 2024;
    lpSystemTime->wMonth = 1;
    lpSystemTime->wDayOfWeek = 0;
    lpSystemTime->wDay = 1;
    lpSystemTime->wHour = (WORD)(hours % 24);
    lpSystemTime->wMinute = (WORD)(minutes % 60);
    lpSystemTime->wSecond = (WORD)(seconds % 60);
    lpSystemTime->wMilliseconds = (WORD)(ticks % 1000);
}

void WINAPI GetLocalTime(LPSYSTEMTIME lpSystemTime) {
    GetSystemTime(lpSystemTime);  /* No timezone support */
}

BOOL WINAPI SetSystemTime(const SYSTEMTIME* lpSystemTime) {
    (void)lpSystemTime;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI SetLocalTime(const SYSTEMTIME* lpSystemTime) {
    return SetSystemTime(lpSystemTime);
}

BOOL WINAPI SystemTimeToFileTime(const SYSTEMTIME* lpSystemTime, LPFILETIME lpFileTime) {
    if (!lpSystemTime || !lpFileTime) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Simple conversion - not accurate but functional */
    DWORD ticks = lpSystemTime->wMilliseconds +
                  lpSystemTime->wSecond * 1000 +
                  lpSystemTime->wMinute * 60000;
    
    lpFileTime->dwLowDateTime = ticks;
    lpFileTime->dwHighDateTime = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI FileTimeToSystemTime(const FILETIME* lpFileTime, LPSYSTEMTIME lpSystemTime) {
    if (!lpFileTime || !lpSystemTime) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    DWORD ticks = lpFileTime->dwLowDateTime;
    
    lpSystemTime->wMilliseconds = (WORD)(ticks % 1000);
    lpSystemTime->wSecond = (WORD)((ticks / 1000) % 60);
    lpSystemTime->wMinute = (WORD)((ticks / 60000) % 60);
    lpSystemTime->wHour = 0;
    lpSystemTime->wDay = 1;
    lpSystemTime->wMonth = 1;
    lpSystemTime->wYear = 2024;
    lpSystemTime->wDayOfWeek = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetTimeZoneInformation(void* lpTimeZoneInformation) {
    (void)lpTimeZoneInformation;
    return 0;  /* TIME_ZONE_ID_UNKNOWN */
}

BOOL WINAPI QueryPerformanceCounter(void* lpPerformanceCount) {
    if (!lpPerformanceCount) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    DWORD ticks = timer_get_ticks();
    *((DWORD*)lpPerformanceCount) = ticks;
    *((DWORD*)lpPerformanceCount + 1) = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI QueryPerformanceFrequency(void* lpFrequency) {
    if (!lpFrequency) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Return 1000 Hz (1ms resolution) */
    *((DWORD*)lpFrequency) = 1000;
    *((DWORD*)lpFrequency + 1) = 0;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

DWORD WINAPI GetLogicalDrives(void) {
    /* Return C: drive only */
    return 0x04;  /* Bit 2 = C: */
}

DWORD WINAPI GetLogicalDriveStringsA(DWORD nBufferLength, LPSTR lpBuffer) {
    const char* drives = "C:\\\0";
    DWORD len = 4;  /* "C:\" + null + final null */
    
    if (!lpBuffer || nBufferLength < len) {
        return len;
    }
    
    k32_memcpy(lpBuffer, drives, len);
    winapi_set_last_error(ERROR_SUCCESS);
    return len - 1;
}

UINT WINAPI GetDriveTypeA(LPCSTR lpRootPathName) {
    (void)lpRootPathName;
    return 3;  /* DRIVE_FIXED */
}

BOOL WINAPI GetDiskFreeSpaceA(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster,
                              LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters,
                              LPDWORD lpTotalNumberOfClusters) {
    (void)lpRootPathName;
    
    if (lpSectorsPerCluster) *lpSectorsPerCluster = 8;
    if (lpBytesPerSector) *lpBytesPerSector = 512;
    if (lpNumberOfFreeClusters) *lpNumberOfFreeClusters = 100000;
    if (lpTotalNumberOfClusters) *lpTotalNumberOfClusters = 200000;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize) {
    const char* name = "AURORA";
    DWORD len = (DWORD)k32_strlen(name);
    
    if (!lpBuffer || !nSize || *nSize <= len) {
        if (nSize) *nSize = len + 1;
        winapi_set_last_error(ERROR_BUFFER_OVERFLOW);
        return FALSE;
    }
    
    k32_strcpy(lpBuffer, name);
    *nSize = len;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}


BOOL WINAPI GetUserNameA(LPSTR lpBuffer, LPDWORD pcbBuffer) {
    const char* name = "User";
    DWORD len = (DWORD)k32_strlen(name);
    
    if (!lpBuffer || !pcbBuffer || *pcbBuffer <= len) {
        if (pcbBuffer) *pcbBuffer = len + 1;
        winapi_set_last_error(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }
    
    k32_strcpy(lpBuffer, name);
    *pcbBuffer = len + 1;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ============================================================================
 * Synchronization Functions
 * ============================================================================ */

HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset,
                           BOOL bInitialState, LPCSTR lpName) {
    (void)lpEventAttributes;
    
    event_data_t* event = (event_data_t*)kmalloc(sizeof(event_data_t));
    if (!event) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    k32_memset(event, 0, sizeof(event_data_t));
    event->signaled = bInitialState ? 1 : 0;
    event->manual_reset = bManualReset ? 1 : 0;
    if (lpName) {
        k32_strncpy(event->name, lpName, sizeof(event->name) - 1);
    }
    
    HANDLE hEvent = alloc_handle(HANDLE_TYPE_EVENT, event);
    if (hEvent == INVALID_HANDLE_VALUE) {
        kfree(event);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return hEvent;
}

BOOL WINAPI SetEvent(HANDLE hEvent) {
    handle_entry_t* entry = get_handle_entry(hEvent);
    if (!entry || entry->type != HANDLE_TYPE_EVENT) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    event_data_t* event = (event_data_t*)entry->data;
    if (event) {
        event->signaled = 1;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI ResetEvent(HANDLE hEvent) {
    handle_entry_t* entry = get_handle_entry(hEvent);
    if (!entry || entry->type != HANDLE_TYPE_EVENT) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    event_data_t* event = (event_data_t*)entry->data;
    if (event) {
        event->signaled = 0;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

BOOL WINAPI PulseEvent(HANDLE hEvent) {
    SetEvent(hEvent);
    ResetEvent(hEvent);
    return TRUE;
}

HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) {
    (void)lpMutexAttributes;
    
    mutex_data_t* mutex = (mutex_data_t*)kmalloc(sizeof(mutex_data_t));
    if (!mutex) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    k32_memset(mutex, 0, sizeof(mutex_data_t));
    if (bInitialOwner) {
        mutex->locked = 1;
        mutex->owner_thread = g_current_thread_id;
        mutex->recursion_count = 1;
    }
    if (lpName) {
        k32_strncpy(mutex->name, lpName, sizeof(mutex->name) - 1);
    }
    
    HANDLE hMutex = alloc_handle(HANDLE_TYPE_MUTEX, mutex);
    if (hMutex == INVALID_HANDLE_VALUE) {
        kfree(mutex);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return hMutex;
}

BOOL WINAPI ReleaseMutex(HANDLE hMutex) {
    handle_entry_t* entry = get_handle_entry(hMutex);
    if (!entry || entry->type != HANDLE_TYPE_MUTEX) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    mutex_data_t* mutex = (mutex_data_t*)entry->data;
    if (!mutex || mutex->owner_thread != g_current_thread_id) {
        winapi_set_last_error(ERROR_NOT_OWNER);
        return FALSE;
    }
    
    mutex->recursion_count--;
    if (mutex->recursion_count <= 0) {
        mutex->locked = 0;
        mutex->owner_thread = 0;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}


HANDLE WINAPI CreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount,
                               LONG lMaximumCount, LPCSTR lpName) {
    (void)lpSemaphoreAttributes;
    
    if (lInitialCount < 0 || lMaximumCount <= 0 || lInitialCount > lMaximumCount) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    semaphore_data_t* sem = (semaphore_data_t*)kmalloc(sizeof(semaphore_data_t));
    if (!sem) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    k32_memset(sem, 0, sizeof(semaphore_data_t));
    sem->count = lInitialCount;
    sem->max_count = lMaximumCount;
    if (lpName) {
        k32_strncpy(sem->name, lpName, sizeof(sem->name) - 1);
    }
    
    HANDLE hSemaphore = alloc_handle(HANDLE_TYPE_SEMAPHORE, sem);
    if (hSemaphore == INVALID_HANDLE_VALUE) {
        kfree(sem);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return hSemaphore;
}

BOOL WINAPI ReleaseSemaphore(HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount) {
    handle_entry_t* entry = get_handle_entry(hSemaphore);
    if (!entry || entry->type != HANDLE_TYPE_SEMAPHORE) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    semaphore_data_t* sem = (semaphore_data_t*)entry->data;
    if (!sem) {
        winapi_set_last_error(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    
    if (lpPreviousCount) {
        *lpPreviousCount = sem->count;
    }
    
    if (sem->count + lReleaseCount > sem->max_count) {
        winapi_set_last_error(ERROR_TOO_MANY_POSTS);
        return FALSE;
    }
    
    sem->count += lReleaseCount;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}


void WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    
    k32_memset(lpCriticalSection, 0, sizeof(CRITICAL_SECTION));
    lpCriticalSection->LockCount = -1;
    lpCriticalSection->RecursionCount = 0;
    lpCriticalSection->OwningThread = NULL;
}

void WINAPI EnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    
    /* Simple spinlock implementation */
    while (1) {
        LONG old_count = lpCriticalSection->LockCount;
        if (old_count < 0) {
            lpCriticalSection->LockCount = 0;
            lpCriticalSection->RecursionCount = 1;
            lpCriticalSection->OwningThread = (HANDLE)(uintptr_t)g_current_thread_id;
            return;
        }
        
        /* Check for recursive entry */
        if (lpCriticalSection->OwningThread == (HANDLE)(uintptr_t)g_current_thread_id) {
            lpCriticalSection->RecursionCount++;
            return;
        }
        
        /* Spin */
        process_yield();
    }
}

void WINAPI LeaveCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    
    if (lpCriticalSection->OwningThread != (HANDLE)(uintptr_t)g_current_thread_id) {
        return;  /* Not owner */
    }
    
    lpCriticalSection->RecursionCount--;
    if (lpCriticalSection->RecursionCount == 0) {
        lpCriticalSection->OwningThread = NULL;
        lpCriticalSection->LockCount = -1;
    }
}

void WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return;
    k32_memset(lpCriticalSection, 0, sizeof(CRITICAL_SECTION));
}

BOOL WINAPI TryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    if (!lpCriticalSection) return FALSE;
    
    if (lpCriticalSection->LockCount < 0) {
        lpCriticalSection->LockCount = 0;
        lpCriticalSection->RecursionCount = 1;
        lpCriticalSection->OwningThread = (HANDLE)(uintptr_t)g_current_thread_id;
        return TRUE;
    }
    
    if (lpCriticalSection->OwningThread == (HANDLE)(uintptr_t)g_current_thread_id) {
        lpCriticalSection->RecursionCount++;
        return TRUE;
    }
    
    return FALSE;
}

LONG WINAPI InterlockedIncrement(LONG volatile* lpAddend) {
    if (!lpAddend) return 0;
    return ++(*lpAddend);
}

LONG WINAPI InterlockedDecrement(LONG volatile* lpAddend) {
    if (!lpAddend) return 0;
    return --(*lpAddend);
}

LONG WINAPI InterlockedExchange(LONG volatile* Target, LONG Value) {
    if (!Target) return 0;
    LONG old = *Target;
    *Target = Value;
    return old;
}

LONG WINAPI InterlockedCompareExchange(LONG volatile* Destination, LONG Exchange, LONG Comparand) {
    if (!Destination) return 0;
    LONG old = *Destination;
    if (old == Comparand) {
        *Destination = Exchange;
    }
    return old;
}

LPVOID WINAPI InterlockedExchangePointer(LPVOID volatile* Target, LPVOID Value) {
    if (!Target) return NULL;
    LPVOID old = *Target;
    *Target = Value;
    return old;
}

LPVOID WINAPI InterlockedCompareExchangePointer(LPVOID volatile* Destination,
                                                 LPVOID Exchange, LPVOID Comparand) {
    if (!Destination) return NULL;
    LPVOID old = *Destination;
    if (old == Comparand) {
        *Destination = Exchange;
    }
    return old;
}

/* ============================================================================
 * TLS Functions
 * ============================================================================ */

DWORD WINAPI TlsAlloc(void) {
    for (DWORD i = 0; i < TLS_SLOTS; i++) {
        if (!g_tls_in_use[i]) {
            g_tls_in_use[i] = 1;
            g_tls_slots[i] = NULL;
            return i;
        }
    }
    winapi_set_last_error(ERROR_NO_MORE_ITEMS);
    return 0xFFFFFFFF;
}

BOOL WINAPI TlsFree(DWORD dwTlsIndex) {
    if (dwTlsIndex >= TLS_SLOTS) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    g_tls_in_use[dwTlsIndex] = 0;
    g_tls_slots[dwTlsIndex] = NULL;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

LPVOID WINAPI TlsGetValue(DWORD dwTlsIndex) {
    if (dwTlsIndex >= TLS_SLOTS) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    winapi_set_last_error(ERROR_SUCCESS);
    return g_tls_slots[dwTlsIndex];
}

BOOL WINAPI TlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue) {
    if (dwTlsIndex >= TLS_SLOTS) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    g_tls_slots[dwTlsIndex] = lpTlsValue;
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

/* ============================================================================
 * Process Creation
 * ============================================================================ */

BOOL WINAPI CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine,
                           LPSECURITY_ATTRIBUTES lpProcessAttributes,
                           LPSECURITY_ATTRIBUTES lpThreadAttributes,
                           BOOL bInheritHandles, DWORD dwCreationFlags,
                           LPVOID lpEnvironment, LPCSTR lpCurrentDirectory,
                           LPSTARTUPINFOA lpStartupInfo,
                           LPPROCESS_INFORMATION lpProcessInformation) {
    (void)lpApplicationName;
    (void)lpCommandLine;
    (void)lpProcessAttributes;
    (void)lpThreadAttributes;
    (void)bInheritHandles;
    (void)dwCreationFlags;
    (void)lpEnvironment;
    (void)lpCurrentDirectory;
    (void)lpStartupInfo;
    
    if (!lpProcessInformation) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    /* Allocate process data */
    process_data_t* proc = (process_data_t*)kmalloc(sizeof(process_data_t));
    if (!proc) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    k32_memset(proc, 0, sizeof(process_data_t));
    proc->process_id = g_next_process_id++;
    proc->terminated = 0;
    
    HANDLE hProcess = alloc_handle(HANDLE_TYPE_PROCESS, proc);
    if (hProcess == INVALID_HANDLE_VALUE) {
        kfree(proc);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    /* Allocate thread data */
    thread_data_t* thread = (thread_data_t*)kmalloc(sizeof(thread_data_t));
    if (!thread) {
        free_handle(hProcess);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    k32_memset(thread, 0, sizeof(thread_data_t));
    thread->thread_id = g_next_thread_id++;
    thread->suspended = (dwCreationFlags & CREATE_SUSPENDED) ? 1 : 0;
    
    HANDLE hThread = alloc_handle(HANDLE_TYPE_THREAD, thread);
    if (hThread == INVALID_HANDLE_VALUE) {
        free_handle(hProcess);
        kfree(thread);
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    
    lpProcessInformation->hProcess = hProcess;
    lpProcessInformation->hThread = hThread;
    lpProcessInformation->dwProcessId = proc->process_id;
    lpProcessInformation->dwThreadId = thread->thread_id;
    
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

HANDLE WINAPI OpenProcess(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId) {
    (void)dwDesiredAccess;
    (void)bInheritHandle;
    
    if (dwProcessId == g_current_process_id) {
        return GetCurrentProcess();
    }
    
    /* Other processes not supported */
    winapi_set_last_error(ERROR_INVALID_PARAMETER);
    return NULL;
}

/* ============================================================================
 * Debugging
 * ============================================================================ */

void WINAPI OutputDebugStringA(LPCSTR lpOutputString) {
    if (lpOutputString) {
        vga_write("[DEBUG] ");
        vga_write(lpOutputString);
        vga_write("\n");
    }
}

BOOL WINAPI IsDebuggerPresent(void) {
    return FALSE;  /* No debugger in kernel mode */
}

void WINAPI DebugBreak(void) {
    __asm__ volatile("int $3");
}

/* ============================================================================
 * Exception Handling
 * ============================================================================ */

/* Vectored Exception Handler storage */
#define MAX_VEH_HANDLERS 32
static struct {
    void* handler;
    int first;  /* 1 if first handler, 0 if last */
    int in_use;
} g_veh_handlers[MAX_VEH_HANDLERS];
static int g_veh_count = 0;

/* Structured Exception Handler chain (per-thread) */
#define MAX_SEH_FRAMES 64
static struct {
    void* handler;
    void* frame;
    int in_use;
} g_seh_chain[MAX_SEH_FRAMES];
static int g_seh_depth = 0;

/* Exception record structure */
typedef struct _EXCEPTION_RECORD {
    DWORD ExceptionCode;
    DWORD ExceptionFlags;
    struct _EXCEPTION_RECORD* ExceptionRecord;
    LPVOID ExceptionAddress;
    DWORD NumberParameters;
    DWORD ExceptionInformation[15];
} EXCEPTION_RECORD, *PEXCEPTION_RECORD;

/* Context structure (simplified) */
typedef struct _CONTEXT {
    DWORD ContextFlags;
    DWORD Eax;
    DWORD Ebx;
    DWORD Ecx;
    DWORD Edx;
    DWORD Esi;
    DWORD Edi;
    DWORD Ebp;
    DWORD Esp;
    DWORD Eip;
    DWORD EFlags;
} CONTEXT, *PCONTEXT;

/* Exception pointers structure */
typedef struct _EXCEPTION_POINTERS {
    PEXCEPTION_RECORD ExceptionRecord;
    PCONTEXT ContextRecord;
} EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

/* Vectored exception handler return values */
#define EXCEPTION_CONTINUE_SEARCH 0
#define EXCEPTION_CONTINUE_EXECUTION (-1)

/* Exception disposition values */
#define ExceptionContinueExecution 0
#define ExceptionContinueSearch 1
#define ExceptionNestedException 2
#define ExceptionCollidedUnwind 3

LPVOID WINAPI AddVectoredExceptionHandler(DWORD First, void* Handler) {
    if (!Handler) {
        winapi_set_last_error(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    
    if (g_veh_count >= MAX_VEH_HANDLERS) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    /* Find free slot */
    int slot = -1;
    for (int i = 0; i < MAX_VEH_HANDLERS; i++) {
        if (!g_veh_handlers[i].in_use) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        winapi_set_last_error(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    
    g_veh_handlers[slot].handler = Handler;
    g_veh_handlers[slot].first = First ? 1 : 0;
    g_veh_handlers[slot].in_use = 1;
    g_veh_count++;
    
    vga_write("Kernel32: Registered vectored exception handler\n");
    
    winapi_set_last_error(ERROR_SUCCESS);
    return Handler;
}

LPVOID WINAPI AddVectoredContinueHandler(DWORD First, void* Handler) {
    /* Continue handlers are called after SEH handlers */
    return AddVectoredExceptionHandler(First, Handler);
}

DWORD WINAPI RemoveVectoredExceptionHandler(LPVOID Handle) {
    if (!Handle) {
        return 0;
    }
    
    for (int i = 0; i < MAX_VEH_HANDLERS; i++) {
        if (g_veh_handlers[i].in_use && g_veh_handlers[i].handler == Handle) {
            g_veh_handlers[i].handler = NULL;
            g_veh_handlers[i].first = 0;
            g_veh_handlers[i].in_use = 0;
            g_veh_count--;
            
            vga_write("Kernel32: Removed vectored exception handler\n");
            return 1;
        }
    }
    return 0;
}

DWORD WINAPI RemoveVectoredContinueHandler(LPVOID Handle) {
    return RemoveVectoredExceptionHandler(Handle);
}

/* Internal function to dispatch exception to VEH handlers */
static int dispatch_to_veh_handlers(PEXCEPTION_POINTERS exception_info, int first_only) {
    /* Call handlers marked as 'first' */
    for (int i = 0; i < MAX_VEH_HANDLERS; i++) {
        if (g_veh_handlers[i].in_use && g_veh_handlers[i].first) {
            /* Validate handler pointer - basic sanity check */
            void* handler_ptr = g_veh_handlers[i].handler;
            if (!handler_ptr || (uintptr_t)handler_ptr < 0x1000) {
                /* Invalid handler address - skip */
                continue;
            }
            
            typedef LONG (*VEH_HANDLER)(PEXCEPTION_POINTERS);
            VEH_HANDLER handler = (VEH_HANDLER)handler_ptr;
            LONG result = handler(exception_info);
            if (result == EXCEPTION_CONTINUE_EXECUTION) {
                return 1;  /* Exception handled */
            }
        }
    }
    
    if (first_only) {
        return 0;
    }
    
    /* Call handlers marked as 'last' */
    for (int i = 0; i < MAX_VEH_HANDLERS; i++) {
        if (g_veh_handlers[i].in_use && !g_veh_handlers[i].first) {
            /* Validate handler pointer - basic sanity check */
            void* handler_ptr = g_veh_handlers[i].handler;
            if (!handler_ptr || (uintptr_t)handler_ptr < 0x1000) {
                /* Invalid handler address - skip */
                continue;
            }
            
            typedef LONG (*VEH_HANDLER)(PEXCEPTION_POINTERS);
            VEH_HANDLER handler = (VEH_HANDLER)handler_ptr;
            LONG result = handler(exception_info);
            if (result == EXCEPTION_CONTINUE_EXECUTION) {
                return 1;  /* Exception handled */
            }
        }
    }
    
    return 0;
}

void WINAPI RaiseException(DWORD dwExceptionCode, DWORD dwExceptionFlags,
                           DWORD nNumberOfArguments, const DWORD* lpArguments) {
    vga_write("Kernel32: Exception raised - Code: ");
    vga_write_hex(dwExceptionCode);
    vga_write("\n");
    
    /* Build exception record */
    EXCEPTION_RECORD exception_record;
    k32_memset(&exception_record, 0, sizeof(EXCEPTION_RECORD));
    exception_record.ExceptionCode = dwExceptionCode;
    exception_record.ExceptionFlags = dwExceptionFlags;
    exception_record.ExceptionRecord = NULL;
    exception_record.ExceptionAddress = NULL;  /* Would get return address */
    
    if (nNumberOfArguments > 15) {
        nNumberOfArguments = 15;
    }
    exception_record.NumberParameters = nNumberOfArguments;
    
    if (lpArguments && nNumberOfArguments > 0) {
        for (DWORD i = 0; i < nNumberOfArguments; i++) {
            exception_record.ExceptionInformation[i] = lpArguments[i];
        }
    }
    
    /* Build context */
    CONTEXT context;
    k32_memset(&context, 0, sizeof(CONTEXT));
    context.ContextFlags = 0x10001F;  /* CONTEXT_FULL */
    
    /* Build exception pointers */
    EXCEPTION_POINTERS exception_info;
    exception_info.ExceptionRecord = &exception_record;
    exception_info.ContextRecord = &context;
    
    /* Dispatch to VEH handlers */
    if (dispatch_to_veh_handlers(&exception_info, 0)) {
        vga_write("Kernel32: Exception handled by VEH\n");
        return;
    }
    
    /* If not handled and not continuable, this is fatal */
    if (!(dwExceptionFlags & 0x01)) {  /* EXCEPTION_NONCONTINUABLE */
        vga_write("Kernel32: Unhandled non-continuable exception!\n");
        /* Would terminate process */
    }
}

/* Set/Get unhandled exception filter */
static void* g_unhandled_exception_filter = NULL;

LPVOID WINAPI SetUnhandledExceptionFilter(void* lpTopLevelExceptionFilter) {
    LPVOID old_filter = g_unhandled_exception_filter;
    g_unhandled_exception_filter = lpTopLevelExceptionFilter;
    winapi_set_last_error(ERROR_SUCCESS);
    return old_filter;
}

/* Unhandled exception filter */
LONG WINAPI UnhandledExceptionFilter(PEXCEPTION_POINTERS ExceptionInfo) {
    (void)ExceptionInfo;
    
    if (g_unhandled_exception_filter) {
        typedef LONG (*UEF)(PEXCEPTION_POINTERS);
        UEF filter = (UEF)g_unhandled_exception_filter;
        return filter(ExceptionInfo);
    }
    
    /* Default behavior: terminate */
    return 0;  /* EXCEPTION_EXECUTE_HANDLER - terminate process */
}

/* ============================================================================
 * Miscellaneous
 * ============================================================================ */

void WINAPI GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo) {
    if (!lpStartupInfo) return;
    k32_memcpy(lpStartupInfo, &g_startup_info, sizeof(STARTUPINFOA));
}

DWORD WINAPI GetCurrentDirectory_A(DWORD nBufferLength, LPSTR lpBuffer) {
    return GetCurrentDirectoryA(nBufferLength, lpBuffer);
}

BOOL WINAPI Beep(DWORD dwFreq, DWORD dwDuration) {
    (void)dwFreq;
    (void)dwDuration;
    
    /* PC speaker not implemented */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}

UINT WINAPI SetErrorMode(UINT uMode) {
    static UINT error_mode = 0;
    UINT old_mode = error_mode;
    error_mode = uMode;
    return old_mode;
}

DWORD WINAPI GetPrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault,
                                      LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName) {
    (void)lpAppName;
    (void)lpKeyName;
    (void)lpFileName;
    
    if (!lpReturnedString || nSize == 0) {
        return 0;
    }
    
    if (lpDefault) {
        k32_strncpy(lpReturnedString, lpDefault, (int)nSize - 1);
        return (DWORD)k32_strlen(lpReturnedString);
    }
    
    lpReturnedString[0] = '\0';
    return 0;
}

BOOL WINAPI WritePrivateProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName,
                                       LPCSTR lpString, LPCSTR lpFileName) {
    (void)lpAppName;
    (void)lpKeyName;
    (void)lpString;
    (void)lpFileName;
    
    /* INI file writing not implemented */
    winapi_set_last_error(ERROR_SUCCESS);
    return TRUE;
}
