/**
 * @file android_vm.h
 * @brief Android Kernel Virtual Machine Support for Aurora OS
 * 
 * Provides Android kernel compatibility layer and VM support
 * allowing Aurora OS to run Android workloads in dedicated workspaces
 */

#ifndef ANDROID_VM_H
#define ANDROID_VM_H

#include <stdint.h>
#include <stdbool.h>
#include "aurora_vm.h"

/* Forward declarations */
typedef struct dalvik_vm dalvik_vm_t;
typedef struct binder_process binder_process_t;
typedef struct surfaceflinger surfaceflinger_t;

/* Android VM configuration */
#define ANDROID_VM_MEMORY_SIZE      (256 * 1024 * 1024)   /* 256MB for Android */
#define ANDROID_VM_KERNEL_BASE      0x80000               /* ARM kernel load address */
#define ANDROID_VM_RAMDISK_BASE     0x01000000            /* Ramdisk load address */
#define ANDROID_VM_MAX_PROCESSES    128                   /* Max Android processes */
#define ANDROID_VM_SYSTEM_SIZE      (512 * 1024 * 1024)   /* 512MB system partition */
#define ANDROID_VM_DATA_SIZE        (1024 * 1024 * 1024)  /* 1GB data partition */

/* Android VM state */
typedef enum {
    ANDROID_VM_STATE_UNINITIALIZED = 0,
    ANDROID_VM_STATE_INITIALIZED,
    ANDROID_VM_STATE_BOOTING,
    ANDROID_VM_STATE_RUNNING,
    ANDROID_VM_STATE_PAUSED,
    ANDROID_VM_STATE_STOPPED,
    ANDROID_VM_STATE_ERROR
} android_vm_state_t;

/* Android architecture types */
typedef enum {
    ANDROID_ARCH_ARM32 = 0,
    ANDROID_ARCH_ARM64,
    ANDROID_ARCH_X86,
    ANDROID_ARCH_X86_64
} android_arch_t;

/* Android VM instance */
typedef struct {
    AuroraVM* aurora_vm;              /* Underlying Aurora VM */
    android_vm_state_t state;         /* Current state */
    android_arch_t arch;              /* Target architecture */
    uint8_t* kernel_image;            /* Loaded kernel image */
    uint32_t kernel_size;             /* Kernel image size */
    uint32_t kernel_entry;            /* Kernel entry point */
    char kernel_cmdline[512];         /* Kernel command line */
    bool has_ramdisk;                 /* Has initial ramdisk */
    uint32_t ramdisk_addr;            /* Ramdisk load address */
    uint32_t ramdisk_size;            /* Ramdisk size */
    uint8_t* system_image;            /* System partition image */
    uint32_t system_size;             /* System partition size */
    uint8_t* data_image;              /* Data partition image */
    uint32_t data_size;               /* Data partition size */
    uint32_t android_version;         /* Android version (e.g., 1300 for 13.0) */
    bool dalvik_enabled;              /* Dalvik/ART VM enabled */
    dalvik_vm_t* dalvik_vm;           /* Dalvik/ART VM instance */
    binder_process_t* binder_process; /* Binder IPC process state */
    void* surfaceflinger;             /* SurfaceFlinger instance */
} AndroidVM;

/* Android syscall emulation (Bionic libc compatibility) */
typedef enum {
    ANDROID_SYSCALL_EXIT = 1,
    ANDROID_SYSCALL_FORK = 2,
    ANDROID_SYSCALL_READ = 3,
    ANDROID_SYSCALL_WRITE = 4,
    ANDROID_SYSCALL_OPEN = 5,
    ANDROID_SYSCALL_CLOSE = 6,
    ANDROID_SYSCALL_WAITPID = 7,
    ANDROID_SYSCALL_EXECVE = 11,
    ANDROID_SYSCALL_GETPID = 20,
    ANDROID_SYSCALL_GETUID = 24,
    ANDROID_SYSCALL_IOCTL = 54,
    ANDROID_SYSCALL_BRK = 45,
    ANDROID_SYSCALL_MMAP = 90,
    ANDROID_SYSCALL_MUNMAP = 91,
    ANDROID_SYSCALL_CLONE = 120,
    ANDROID_SYSCALL_PRCTL = 172,
    ANDROID_SYSCALL_FUTEX = 240,
    /* Bionic-specific syscalls */
    ANDROID_SYSCALL_OPENAT = 295,
    ANDROID_SYSCALL_FACCESSAT = 307,
    /* Extended syscalls - file operations */
    ANDROID_SYSCALL_LSEEK = 8,
    ANDROID_SYSCALL_STAT = 106,
    ANDROID_SYSCALL_FSTAT = 28,
    ANDROID_SYSCALL_LSTAT = 107,
    ANDROID_SYSCALL_DUP = 32,
    ANDROID_SYSCALL_DUP2 = 63,
    ANDROID_SYSCALL_DUP3 = 292,
    ANDROID_SYSCALL_PIPE = 42,
    ANDROID_SYSCALL_PIPE2 = 293,
    ANDROID_SYSCALL_FCNTL = 25,
    /* Extended syscalls - directory operations */
    ANDROID_SYSCALL_MKDIR = 39,
    ANDROID_SYSCALL_MKDIRAT = 296,
    ANDROID_SYSCALL_RMDIR = 40,
    ANDROID_SYSCALL_UNLINK = 10,
    ANDROID_SYSCALL_UNLINKAT = 301,
    ANDROID_SYSCALL_RENAME = 38,
    ANDROID_SYSCALL_RENAMEAT = 302,
    ANDROID_SYSCALL_CHDIR = 12,
    ANDROID_SYSCALL_FCHDIR = 133,
    ANDROID_SYSCALL_GETCWD = 183,
    /* Extended syscalls - process/thread */
    ANDROID_SYSCALL_GETPPID = 64,
    ANDROID_SYSCALL_GETTID = 178,
    ANDROID_SYSCALL_GETEUID = 49,
    ANDROID_SYSCALL_GETEGID = 50,
    ANDROID_SYSCALL_SETUID = 23,
    ANDROID_SYSCALL_SETGID = 46,
    ANDROID_SYSCALL_SET_TID_ADDRESS = 256,
    /* Extended syscalls - time */
    ANDROID_SYSCALL_NANOSLEEP = 162,
    ANDROID_SYSCALL_CLOCK_GETTIME = 263,
    ANDROID_SYSCALL_GETTIMEOFDAY = 78,
    /* Extended syscalls - signals */
    ANDROID_SYSCALL_KILL = 37,
    ANDROID_SYSCALL_TGKILL = 270,
    ANDROID_SYSCALL_RT_SIGACTION = 134,
    ANDROID_SYSCALL_RT_SIGPROCMASK = 135,
    /* Extended syscalls - socket */
    ANDROID_SYSCALL_SOCKET = 281,
    ANDROID_SYSCALL_BIND = 282,
    ANDROID_SYSCALL_CONNECT = 283,
    ANDROID_SYSCALL_LISTEN = 284,
    ANDROID_SYSCALL_ACCEPT = 285,
    /* Extended syscalls - epoll */
    ANDROID_SYSCALL_EPOLL_CREATE = 250,
    ANDROID_SYSCALL_EPOLL_CREATE1 = 329,
    ANDROID_SYSCALL_EPOLL_CTL = 251,
    ANDROID_SYSCALL_EPOLL_WAIT = 252,
    /* Extended syscalls - memory */
    ANDROID_SYSCALL_MPROTECT = 125,
    ANDROID_SYSCALL_MADVISE = 220,
    ANDROID_SYSCALL_MSYNC = 227,
    /* Extended syscalls - misc */
    ANDROID_SYSCALL_UMASK = 60,
    ANDROID_SYSCALL_UNAME = 63,
    ANDROID_SYSCALL_GETRANDOM = 384,
    ANDROID_SYSCALL_MEMFD_CREATE = 385,
    ANDROID_SYSCALL_SECCOMP = 383,
    ANDROID_SYSCALL_SCHED_YIELD = 124,
} android_syscall_t;

/* Total implemented syscalls count */
#define ANDROID_SYSCALL_COUNT 75

/* Android property system */
typedef struct {
    char name[128];
    char value[128];
} android_property_t;

/**
 * Initialize Android VM subsystem
 * @return 0 on success, -1 on failure
 */
int android_vm_init(void);

/**
 * Create a new Android VM instance
 * @param arch Target Android architecture
 * @return Pointer to Android VM or NULL on failure
 */
AndroidVM* android_vm_create(android_arch_t arch);

/**
 * Destroy Android VM instance
 * @param vm Android VM instance
 */
void android_vm_destroy(AndroidVM* vm);

/**
 * Load Android kernel image
 * @param vm Android VM instance
 * @param kernel_data Kernel image data
 * @param size Kernel image size
 * @return 0 on success, -1 on failure
 */
int android_vm_load_kernel(AndroidVM* vm, const uint8_t* kernel_data, uint32_t size);

/**
 * Load ramdisk (initial ramdisk)
 * @param vm Android VM instance
 * @param ramdisk_data Ramdisk data
 * @param size Ramdisk size
 * @return 0 on success, -1 on failure
 */
int android_vm_load_ramdisk(AndroidVM* vm, const uint8_t* ramdisk_data, uint32_t size);

/**
 * Load system partition image
 * @param vm Android VM instance
 * @param system_data System image data
 * @param size System image size
 * @return 0 on success, -1 on failure
 */
int android_vm_load_system(AndroidVM* vm, const uint8_t* system_data, uint32_t size);

/**
 * Load data partition image
 * @param vm Android VM instance
 * @param data_data Data image data
 * @param size Data image size
 * @return 0 on success, -1 on failure
 */
int android_vm_load_data(AndroidVM* vm, const uint8_t* data_data, uint32_t size);

/**
 * Set kernel command line
 * @param vm Android VM instance
 * @param cmdline Command line string
 * @return 0 on success, -1 on failure
 */
int android_vm_set_cmdline(AndroidVM* vm, const char* cmdline);

/**
 * Set Android version
 * @param vm Android VM instance
 * @param major Major version (e.g., 13)
 * @param minor Minor version (e.g., 0)
 * @return 0 on success, -1 on failure
 */
int android_vm_set_version(AndroidVM* vm, uint32_t major, uint32_t minor);

/**
 * Start Android VM execution
 * @param vm Android VM instance
 * @return 0 on success, -1 on failure
 */
int android_vm_start(AndroidVM* vm);

/**
 * Pause Android VM execution
 * @param vm Android VM instance
 * @return 0 on success, -1 on failure
 */
int android_vm_pause(AndroidVM* vm);

/**
 * Resume Android VM execution
 * @param vm Android VM instance
 * @return 0 on success, -1 on failure
 */
int android_vm_resume(AndroidVM* vm);

/**
 * Stop Android VM execution
 * @param vm Android VM instance
 * @return 0 on success, -1 on failure
 */
int android_vm_stop(AndroidVM* vm);

/**
 * Get Android VM state
 * @param vm Android VM instance
 * @return Current state
 */
android_vm_state_t android_vm_get_state(AndroidVM* vm);

/**
 * Handle Android syscall (Bionic libc compatibility)
 * @param vm Android VM instance
 * @param syscall_num Syscall number
 * @param args Syscall arguments
 * @return Syscall return value
 */
int32_t android_vm_handle_syscall(AndroidVM* vm, uint32_t syscall_num, uint32_t* args);

/**
 * Set Android property
 * @param vm Android VM instance
 * @param name Property name
 * @param value Property value
 * @return 0 on success, -1 on failure
 */
int android_vm_set_property(AndroidVM* vm, const char* name, const char* value);

/**
 * Get Android property
 * @param vm Android VM instance
 * @param name Property name
 * @param value Buffer to store value
 * @param size Buffer size
 * @return 0 on success, -1 on failure
 */
int android_vm_get_property(AndroidVM* vm, const char* name, char* value, uint32_t size);

/**
 * Enable Dalvik/ART VM
 * @param vm Android VM instance
 * @param enable Enable or disable
 * @return 0 on success, -1 on failure
 */
int android_vm_enable_dalvik(AndroidVM* vm, bool enable);

/**
 * Check if Android VM is available
 * @return true if Android VM support is enabled
 */
bool android_vm_is_available(void);

/**
 * Get Android VM version string
 * @return Version string
 */
const char* android_vm_get_version(void);

/**
 * Get architecture name string
 * @param arch Architecture type
 * @return Architecture name
 */
const char* android_vm_get_arch_name(android_arch_t arch);

/**
 * Get implemented syscall count
 * @return Number of implemented syscalls
 */
uint32_t android_vm_get_syscall_count(void);

/**
 * Check if syscall is implemented
 * @param syscall_num Syscall number
 * @return true if implemented, false otherwise
 */
bool android_vm_is_syscall_implemented(uint32_t syscall_num);

/**
 * Get console output buffer
 * @return Pointer to console buffer
 */
const char* android_vm_get_console_output(void);

/**
 * Clear console output buffer
 */
void android_vm_clear_console(void);

#endif /* ANDROID_VM_H */
