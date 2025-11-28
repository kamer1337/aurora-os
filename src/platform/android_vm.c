/**
 * @file android_vm.c
 * @brief Android Kernel Virtual Machine Implementation
 */

#include "../../include/platform/android_vm.h"
#include "../../include/platform/dalvik_art.h"
#include "../../include/platform/binder_ipc.h"
#include "../../include/platform/surfaceflinger.h"
#include "../../include/platform/platform_util.h"

/* Global Android VM state */
static bool g_android_vm_initialized = false;
static uint32_t g_android_vm_count = 0;
static android_property_t g_android_properties[64];
static uint32_t g_property_count = 0;

/* Android VM version */
#define ANDROID_VM_VERSION "1.0.0-aurora-aosp"

/* Console output buffer for write syscall */
#define ANDROID_CONSOLE_BUFFER_SIZE 4096
static char g_android_console_buffer[ANDROID_CONSOLE_BUFFER_SIZE];
static uint32_t g_android_console_pos = 0;

/* File descriptor table for Android */
#define ANDROID_MAX_FDS 256
typedef struct {
    bool in_use;
    uint32_t type;      /* 0=stdin, 1=stdout, 2=stderr, 3=file, 4=socket, 5=pipe */
    uint32_t flags;     /* Open flags */
    uint32_t position;  /* Current position in file */
    uint32_t size;      /* File size (for regular files) */
    char path[128];     /* File path */
} android_fd_entry_t;

static android_fd_entry_t g_android_fd_table[ANDROID_MAX_FDS];
static int g_android_next_fd = 3; /* Next available fd after stdin/stdout/stderr */

/* Memory management for Android */
#define ANDROID_HEAP_BASE   0x10000000   /* Heap starts at 256MB */
#define ANDROID_HEAP_MAX    0x30000000   /* Max heap at 768MB */
static uint32_t g_android_current_brk = ANDROID_HEAP_BASE;

/* Process/Thread tracking */
#define ANDROID_MAX_THREADS 64
typedef struct {
    bool active;
    uint32_t tid;
    uint32_t pid;
    uint32_t parent_tid;
    uint32_t stack_ptr;
} android_thread_t;

static android_thread_t g_android_threads[ANDROID_MAX_THREADS];
static uint32_t g_android_next_tid = 1;
static uint32_t g_android_current_pid = 1;

/* Futex table for synchronization */
#define ANDROID_MAX_FUTEXES 32
typedef struct {
    bool in_use;
    uint32_t addr;
    uint32_t waiters;
} android_futex_t;

static android_futex_t g_android_futexes[ANDROID_MAX_FUTEXES];

/* Initialize Android file descriptor table */
static void android_init_fd_table(void) {
    for (int i = 0; i < ANDROID_MAX_FDS; i++) {
        g_android_fd_table[i].in_use = false;
        g_android_fd_table[i].type = 0;
        g_android_fd_table[i].flags = 0;
        g_android_fd_table[i].position = 0;
        g_android_fd_table[i].size = 0;
        g_android_fd_table[i].path[0] = '\0';
    }
    /* Initialize standard file descriptors */
    g_android_fd_table[0].in_use = true;
    g_android_fd_table[0].type = 0; /* stdin */
    g_android_fd_table[1].in_use = true;
    g_android_fd_table[1].type = 1; /* stdout */
    g_android_fd_table[2].in_use = true;
    g_android_fd_table[2].type = 2; /* stderr */
    g_android_next_fd = 3;
}

/* Initialize Android thread table */
static void android_init_thread_table(void) {
    for (int i = 0; i < ANDROID_MAX_THREADS; i++) {
        g_android_threads[i].active = false;
        g_android_threads[i].tid = 0;
        g_android_threads[i].pid = 0;
        g_android_threads[i].parent_tid = 0;
        g_android_threads[i].stack_ptr = 0;
    }
    /* Create initial thread (init process) */
    g_android_threads[0].active = true;
    g_android_threads[0].tid = 1;
    g_android_threads[0].pid = 1;
    g_android_threads[0].parent_tid = 0;
    g_android_next_tid = 2;
    g_android_current_pid = 1;
}

/* Initialize Android futex table */
static void android_init_futex_table(void) {
    for (int i = 0; i < ANDROID_MAX_FUTEXES; i++) {
        g_android_futexes[i].in_use = false;
        g_android_futexes[i].addr = 0;
        g_android_futexes[i].waiters = 0;
    }
}

/* Architecture names */
static const char* arch_names[] = {
    "ARM32",
    "ARM64",
    "x86",
    "x86_64"
};

int android_vm_init(void) {
    if (g_android_vm_initialized) {
        return 0;
    }
    
    /* Initialize Android VM subsystem */
    g_android_vm_count = 0;
    g_property_count = 0;
    platform_memset(g_android_properties, 0, sizeof(g_android_properties));
    
    /* Initialize console buffer */
    g_android_console_pos = 0;
    platform_memset(g_android_console_buffer, 0, ANDROID_CONSOLE_BUFFER_SIZE);
    
    /* Initialize file descriptor table */
    android_init_fd_table();
    
    /* Initialize thread table */
    android_init_thread_table();
    
    /* Initialize futex table */
    android_init_futex_table();
    
    /* Initialize heap */
    g_android_current_brk = ANDROID_HEAP_BASE;
    
    g_android_vm_initialized = true;
    
    return 0;
}

AndroidVM* android_vm_create(android_arch_t arch) {
    if (!g_android_vm_initialized) {
        android_vm_init();
    }
    
    /* Allocate Android VM structure */
    AndroidVM* vm = (AndroidVM*)platform_malloc(sizeof(AndroidVM));
    if (!vm) {
        return NULL;
    }
    
    /* Initialize fields */
    platform_memset(vm, 0, sizeof(AndroidVM));
    
    /* Create underlying Aurora VM */
    vm->aurora_vm = aurora_vm_create();
    if (!vm->aurora_vm) {
        platform_free(vm);
        return NULL;
    }
    
    vm->state = ANDROID_VM_STATE_INITIALIZED;
    vm->arch = arch;
    vm->kernel_image = NULL;
    vm->kernel_size = 0;
    vm->kernel_entry = ANDROID_VM_KERNEL_BASE;
    vm->has_ramdisk = false;
    vm->ramdisk_addr = ANDROID_VM_RAMDISK_BASE;
    vm->ramdisk_size = 0;
    vm->system_image = NULL;
    vm->system_size = 0;
    vm->data_image = NULL;
    vm->data_size = 0;
    vm->android_version = 1300; /* Android 13.0 default */
    vm->dalvik_enabled = true;
    vm->dalvik_vm = NULL;
    vm->binder_process = NULL;
    vm->surfaceflinger = NULL;
    
    /* Set default kernel command line */
    platform_strncpy(vm->kernel_cmdline, 
                   "console=ttyAMA0 androidboot.hardware=aurora androidboot.selinux=permissive", 
                   sizeof(vm->kernel_cmdline));
    
    g_android_vm_count++;
    
    return vm;
}

void android_vm_destroy(AndroidVM* vm) {
    if (!vm) {
        return;
    }
    
    /* Stop VM if running */
    if (vm->state == ANDROID_VM_STATE_RUNNING || vm->state == ANDROID_VM_STATE_BOOTING) {
        android_vm_stop(vm);
    }
    
    /* Destroy Dalvik/ART VM */
    if (vm->dalvik_vm) {
        /* Note: dalvik_destroy would be called here */
        platform_free(vm->dalvik_vm);
    }
    
    /* Destroy Binder process */
    if (vm->binder_process) {
        /* Note: binder_destroy_process would be called here */
        platform_free(vm->binder_process);
    }
    
    /* Free kernel image */
    if (vm->kernel_image) {
        platform_free(vm->kernel_image);
    }
    
    /* Free system image */
    if (vm->system_image) {
        platform_free(vm->system_image);
    }
    
    /* Free data image */
    if (vm->data_image) {
        platform_free(vm->data_image);
    }
    
    /* Destroy Aurora VM */
    if (vm->aurora_vm) {
        aurora_vm_destroy(vm->aurora_vm);
    }
    
    platform_free(vm);
    
    if (g_android_vm_count > 0) {
        g_android_vm_count--;
    }
}

int android_vm_load_kernel(AndroidVM* vm, const uint8_t* kernel_data, uint32_t size) {
    if (!vm || !kernel_data || size == 0) {
        return -1;
    }
    
    /* Free existing kernel image if any */
    if (vm->kernel_image) {
        platform_free(vm->kernel_image);
    }
    
    /* Allocate memory for kernel image */
    vm->kernel_image = (uint8_t*)platform_malloc(size);
    if (!vm->kernel_image) {
        return -1;
    }
    
    /* Copy kernel data */
    platform_memcpy(vm->kernel_image, kernel_data, size);
    vm->kernel_size = size;
    
    /* TODO: Parse Android boot image header to find kernel entry point */
    /* Android boot images have specific header format with kernel, ramdisk, etc. */
    vm->kernel_entry = ANDROID_VM_KERNEL_BASE;
    
    return 0;
}

int android_vm_load_ramdisk(AndroidVM* vm, const uint8_t* ramdisk_data, uint32_t size) {
    if (!vm || !ramdisk_data || size == 0) {
        return -1;
    }
    
    /* Set ramdisk parameters */
    vm->has_ramdisk = true;
    vm->ramdisk_size = size;
    
    /* TODO: Load ramdisk into VM memory at ramdisk_addr */
    /* This would copy the ramdisk data to the VM's memory space */
    
    return 0;
}

int android_vm_load_system(AndroidVM* vm, const uint8_t* system_data, uint32_t size) {
    if (!vm || !system_data || size == 0 || size > ANDROID_VM_SYSTEM_SIZE) {
        return -1;
    }
    
    /* Free existing system image if any */
    if (vm->system_image) {
        platform_free(vm->system_image);
    }
    
    /* Allocate memory for system image */
    vm->system_image = (uint8_t*)platform_malloc(size);
    if (!vm->system_image) {
        return -1;
    }
    
    /* Copy system data */
    platform_memcpy(vm->system_image, system_data, size);
    vm->system_size = size;
    
    return 0;
}

int android_vm_load_data(AndroidVM* vm, const uint8_t* data_data, uint32_t size) {
    if (!vm || !data_data || size == 0 || size > ANDROID_VM_DATA_SIZE) {
        return -1;
    }
    
    /* Free existing data image if any */
    if (vm->data_image) {
        platform_free(vm->data_image);
    }
    
    /* Allocate memory for data image */
    vm->data_image = (uint8_t*)platform_malloc(size);
    if (!vm->data_image) {
        return -1;
    }
    
    /* Copy data */
    platform_memcpy(vm->data_image, data_data, size);
    vm->data_size = size;
    
    return 0;
}

int android_vm_set_cmdline(AndroidVM* vm, const char* cmdline) {
    if (!vm || !cmdline) {
        return -1;
    }
    
    platform_strncpy(vm->kernel_cmdline, cmdline, sizeof(vm->kernel_cmdline));
    
    return 0;
}

int android_vm_set_version(AndroidVM* vm, uint32_t major, uint32_t minor) {
    if (!vm) {
        return -1;
    }
    
    /* Encode version as major * 100 + minor (e.g., 13.0 -> 1300) */
    vm->android_version = major * 100 + minor;
    
    return 0;
}

int android_vm_start(AndroidVM* vm) {
    if (!vm) {
        return -1;
    }
    
    if (vm->state == ANDROID_VM_STATE_RUNNING) {
        return 0; /* Already running */
    }
    
    if (!vm->kernel_image) {
        return -1; /* No kernel loaded */
    }
    
    vm->state = ANDROID_VM_STATE_BOOTING;
    
    /* Initialize Binder IPC subsystem */
    if (!vm->binder_process) {
        /* Note: In real implementation would call:
         * binder_init();
         * vm->binder_process = binder_create_process(vm->pid);
         * service_manager_init();
         */
    }
    
    /* Initialize SurfaceFlinger */
    if (!vm->surfaceflinger) {
        /* Note: In real implementation would call:
         * surfaceflinger_init();
         * surfaceflinger_set_display(width, height, framebuffer, pitch);
         */
    }
    
    /* Initialize Dalvik/ART VM if enabled */
    if (vm->dalvik_enabled && !vm->dalvik_vm) {
        /* Note: In real implementation would call:
         * dalvik_init(VM_MODE_ART);
         * vm->dalvik_vm = dalvik_create(VM_MODE_ART, 64 * 1024 * 1024);
         * 
         * If DEX files are available:
         * dalvik_load_dex(vm->dalvik_vm, dex_data, dex_size);
         * dalvik_start(vm->dalvik_vm, "Landroid/app/ActivityThread;", "main");
         */
    }
    
    /* Set up Android boot protocol */
    /* This would involve:
     * 1. Setting up Android boot image format (boot.img)
     * 2. Loading kernel into VM memory at KERNEL_BASE
     * 3. Loading ramdisk at RAMDISK_BASE
     * 4. Setting up device tree blob (DTB)
     * 5. Setting up ATAGS or device tree for ARM
     * 6. Setting up initial register state for Android kernel
     * 7. Starting VM execution at kernel entry point
     * 8. Initializing Android init process
     * 9. Mounting system and data partitions
     * 10. Starting system services via Binder IPC
     * 11. Starting Zygote process for app spawning
     * 12. Initializing SurfaceFlinger for graphics
     */
    
    vm->state = ANDROID_VM_STATE_RUNNING;
    
    return 0;
}

int android_vm_pause(AndroidVM* vm) {
    if (!vm) {
        return -1;
    }
    
    if (vm->state != ANDROID_VM_STATE_RUNNING) {
        return -1;
    }
    
    vm->state = ANDROID_VM_STATE_PAUSED;
    
    return 0;
}

int android_vm_resume(AndroidVM* vm) {
    if (!vm) {
        return -1;
    }
    
    if (vm->state != ANDROID_VM_STATE_PAUSED) {
        return -1;
    }
    
    vm->state = ANDROID_VM_STATE_RUNNING;
    
    return 0;
}

int android_vm_stop(AndroidVM* vm) {
    if (!vm) {
        return -1;
    }
    
    vm->state = ANDROID_VM_STATE_STOPPED;
    
    return 0;
}

android_vm_state_t android_vm_get_state(AndroidVM* vm) {
    if (!vm) {
        return ANDROID_VM_STATE_ERROR;
    }
    
    return vm->state;
}

/* Extended Android syscall numbers (ARM/ARM64 compatible) */
#define ANDROID_EXT_SYSCALL_LSEEK       8
#define ANDROID_EXT_SYSCALL_GETPPID     64
#define ANDROID_EXT_SYSCALL_GETTID      178
#define ANDROID_EXT_SYSCALL_GETEUID     49
#define ANDROID_EXT_SYSCALL_GETEGID     50
#define ANDROID_EXT_SYSCALL_SETUID      23
#define ANDROID_EXT_SYSCALL_SETGID      46
#define ANDROID_EXT_SYSCALL_UMASK       60
#define ANDROID_EXT_SYSCALL_CHDIR       12
#define ANDROID_EXT_SYSCALL_FCHDIR      133
#define ANDROID_EXT_SYSCALL_GETCWD      183
#define ANDROID_EXT_SYSCALL_DUP         32
#define ANDROID_EXT_SYSCALL_DUP2        63
#define ANDROID_EXT_SYSCALL_DUP3        292
#define ANDROID_EXT_SYSCALL_PIPE        42
#define ANDROID_EXT_SYSCALL_PIPE2       293
#define ANDROID_EXT_SYSCALL_FCNTL       25
#define ANDROID_EXT_SYSCALL_FSTAT       28
#define ANDROID_EXT_SYSCALL_STAT        106
#define ANDROID_EXT_SYSCALL_LSTAT       107
#define ANDROID_EXT_SYSCALL_MKDIR       39
#define ANDROID_EXT_SYSCALL_MKDIRAT     296
#define ANDROID_EXT_SYSCALL_RMDIR       40
#define ANDROID_EXT_SYSCALL_UNLINK      10
#define ANDROID_EXT_SYSCALL_UNLINKAT    301
#define ANDROID_EXT_SYSCALL_RENAME      38
#define ANDROID_EXT_SYSCALL_RENAMEAT    302
#define ANDROID_EXT_SYSCALL_READLINK    85
#define ANDROID_EXT_SYSCALL_READLINKAT  305
#define ANDROID_EXT_SYSCALL_SYMLINK     83
#define ANDROID_EXT_SYSCALL_SYMLINKAT   304
#define ANDROID_EXT_SYSCALL_LINK        9
#define ANDROID_EXT_SYSCALL_LINKAT      303
#define ANDROID_EXT_SYSCALL_CHMOD       15
#define ANDROID_EXT_SYSCALL_FCHMOD      94
#define ANDROID_EXT_SYSCALL_FCHMODAT    306
#define ANDROID_EXT_SYSCALL_CHOWN       16
#define ANDROID_EXT_SYSCALL_FCHOWN      95
#define ANDROID_EXT_SYSCALL_FCHOWNAT    298
#define ANDROID_EXT_SYSCALL_UTIME       30
#define ANDROID_EXT_SYSCALL_UTIMES      269
#define ANDROID_EXT_SYSCALL_NANOSLEEP   162
#define ANDROID_EXT_SYSCALL_CLOCK_GETTIME 263
#define ANDROID_EXT_SYSCALL_GETTIMEOFDAY 78
#define ANDROID_EXT_SYSCALL_GETRLIMIT   76
#define ANDROID_EXT_SYSCALL_SETRLIMIT   75
#define ANDROID_EXT_SYSCALL_GETRUSAGE   77
#define ANDROID_EXT_SYSCALL_SYSINFO     116
#define ANDROID_EXT_SYSCALL_UNAME       63
#define ANDROID_EXT_SYSCALL_KILL        37
#define ANDROID_EXT_SYSCALL_TGKILL      270
#define ANDROID_EXT_SYSCALL_RT_SIGACTION 134
#define ANDROID_EXT_SYSCALL_RT_SIGPROCMASK 135
#define ANDROID_EXT_SYSCALL_SIGALTSTACK 132
#define ANDROID_EXT_SYSCALL_SOCKET      281
#define ANDROID_EXT_SYSCALL_SOCKETPAIR  288
#define ANDROID_EXT_SYSCALL_BIND        282
#define ANDROID_EXT_SYSCALL_LISTEN      284
#define ANDROID_EXT_SYSCALL_ACCEPT      285
#define ANDROID_EXT_SYSCALL_ACCEPT4     366
#define ANDROID_EXT_SYSCALL_CONNECT     283
#define ANDROID_EXT_SYSCALL_SEND        289
#define ANDROID_EXT_SYSCALL_SENDTO      290
#define ANDROID_EXT_SYSCALL_RECV        291
#define ANDROID_EXT_SYSCALL_RECVFROM    292
#define ANDROID_EXT_SYSCALL_SHUTDOWN    293
#define ANDROID_EXT_SYSCALL_SETSOCKOPT  294
#define ANDROID_EXT_SYSCALL_GETSOCKOPT  295
#define ANDROID_EXT_SYSCALL_EPOLL_CREATE 250
#define ANDROID_EXT_SYSCALL_EPOLL_CREATE1 329
#define ANDROID_EXT_SYSCALL_EPOLL_CTL   251
#define ANDROID_EXT_SYSCALL_EPOLL_WAIT  252
#define ANDROID_EXT_SYSCALL_EPOLL_PWAIT 313
#define ANDROID_EXT_SYSCALL_EVENTFD     323
#define ANDROID_EXT_SYSCALL_EVENTFD2    356
#define ANDROID_EXT_SYSCALL_TIMERFD_CREATE 350
#define ANDROID_EXT_SYSCALL_TIMERFD_SETTIME 353
#define ANDROID_EXT_SYSCALL_TIMERFD_GETTIME 354
#define ANDROID_EXT_SYSCALL_SIGNALFD    282
#define ANDROID_EXT_SYSCALL_SIGNALFD4   355
#define ANDROID_EXT_SYSCALL_GETRANDOM   384
#define ANDROID_EXT_SYSCALL_MEMFD_CREATE 385
#define ANDROID_EXT_SYSCALL_MADVISE     220
#define ANDROID_EXT_SYSCALL_MPROTECT    125
#define ANDROID_EXT_SYSCALL_MSYNC       227
#define ANDROID_EXT_SYSCALL_MLOCK       150
#define ANDROID_EXT_SYSCALL_MUNLOCK     151
#define ANDROID_EXT_SYSCALL_SET_TID_ADDRESS 256
#define ANDROID_EXT_SYSCALL_SET_ROBUST_LIST 311
#define ANDROID_EXT_SYSCALL_GET_ROBUST_LIST 312
#define ANDROID_EXT_SYSCALL_SCHED_YIELD 124
#define ANDROID_EXT_SYSCALL_SCHED_GETAFFINITY 242
#define ANDROID_EXT_SYSCALL_SCHED_SETAFFINITY 241
#define ANDROID_EXT_SYSCALL_ARCH_PRCTL  158
#define ANDROID_EXT_SYSCALL_SECCOMP     383

/* Current directory for file operations */
static char g_android_cwd[256] = "/";
static uint32_t g_android_umask = 0022;

/* Socket tracking */
#define ANDROID_MAX_SOCKETS 32
typedef struct {
    bool in_use;
    int domain;
    int type;
    int protocol;
    bool connected;
    bool listening;
} android_socket_t;

static android_socket_t g_android_sockets[ANDROID_MAX_SOCKETS];
static int g_android_next_sock_fd = 100; /* Start socket fds at 100 */

/* Epoll tracking */
#define ANDROID_MAX_EPOLL 16
typedef struct {
    bool in_use;
    int fd;
    int events;
} android_epoll_entry_t;

typedef struct {
    bool in_use;
    android_epoll_entry_t entries[32];
    uint32_t entry_count;
} android_epoll_t;

static android_epoll_t g_android_epolls[ANDROID_MAX_EPOLL];
static int g_android_next_epoll_fd = 200; /* Start epoll fds at 200 */

/**
 * Extended Android syscall handler for additional syscalls
 */
static int32_t android_vm_handle_extended_syscall(AndroidVM* vm, uint32_t syscall_num, uint32_t* args) {
    (void)vm; /* May not be needed for all syscalls */
    
    switch (syscall_num) {
        /* File position operations */
        case ANDROID_EXT_SYSCALL_LSEEK: {
            /* args[0] = fd, args[1] = offset, args[2] = whence */
            uint32_t fd = args[0];
            int32_t offset = (int32_t)args[1];
            uint32_t whence = args[2];
            
            if (fd >= ANDROID_MAX_FDS || !g_android_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            uint32_t new_pos;
            switch (whence) {
                case 0: /* SEEK_SET */
                    new_pos = (uint32_t)offset;
                    break;
                case 1: /* SEEK_CUR */
                    new_pos = g_android_fd_table[fd].position + (uint32_t)offset;
                    break;
                case 2: /* SEEK_END */
                    new_pos = g_android_fd_table[fd].size + (uint32_t)offset;
                    break;
                default:
                    return -22; /* -EINVAL */
            }
            
            g_android_fd_table[fd].position = new_pos;
            return (int32_t)new_pos;
        }
        
        /* Process ID operations */
        case ANDROID_EXT_SYSCALL_GETPPID:
            return (g_android_current_pid > 1) ? 1 : 0; /* Parent is init (1) */
            
        case ANDROID_EXT_SYSCALL_GETTID:
            return (int32_t)g_android_next_tid - 1; /* Current thread ID */
            
        case ANDROID_EXT_SYSCALL_GETEUID:
        case ANDROID_EXT_SYSCALL_GETEGID:
            return 0; /* Root for Android init */
            
        case ANDROID_EXT_SYSCALL_SETUID:
        case ANDROID_EXT_SYSCALL_SETGID:
            return 0; /* Success */
            
        /* File mask and directory operations */
        case ANDROID_EXT_SYSCALL_UMASK: {
            uint32_t old_umask = g_android_umask;
            g_android_umask = args[0] & 0777;
            return (int32_t)old_umask;
        }
        
        case ANDROID_EXT_SYSCALL_CHDIR:
        case ANDROID_EXT_SYSCALL_FCHDIR:
            return 0; /* Success */
            
        case ANDROID_EXT_SYSCALL_GETCWD: {
            /* args[0] = buf, args[1] = size */
            uint32_t size = args[1];
            uint32_t len = platform_strlen(g_android_cwd);
            if (size <= len) {
                return -34; /* -ERANGE */
            }
            return (int32_t)(len + 1);
        }
        
        /* File descriptor operations */
        case ANDROID_EXT_SYSCALL_DUP: {
            uint32_t oldfd = args[0];
            if (oldfd >= ANDROID_MAX_FDS || !g_android_fd_table[oldfd].in_use) {
                return -9; /* -EBADF */
            }
            if (g_android_next_fd >= ANDROID_MAX_FDS) {
                return -24; /* -EMFILE */
            }
            int newfd = g_android_next_fd++;
            g_android_fd_table[newfd] = g_android_fd_table[oldfd];
            return newfd;
        }
        
        case ANDROID_EXT_SYSCALL_DUP2:
        case ANDROID_EXT_SYSCALL_DUP3: {
            uint32_t oldfd = args[0];
            uint32_t newfd = args[1];
            if (oldfd >= ANDROID_MAX_FDS || !g_android_fd_table[oldfd].in_use) {
                return -9; /* -EBADF */
            }
            if (newfd >= ANDROID_MAX_FDS) {
                return -9; /* -EBADF */
            }
            if (oldfd == newfd) {
                return (int32_t)newfd;
            }
            g_android_fd_table[newfd] = g_android_fd_table[oldfd];
            return (int32_t)newfd;
        }
        
        case ANDROID_EXT_SYSCALL_PIPE:
        case ANDROID_EXT_SYSCALL_PIPE2: {
            /* Create pipe - allocate two file descriptors */
            if (g_android_next_fd + 1 >= ANDROID_MAX_FDS) {
                return -24; /* -EMFILE */
            }
            int read_fd = g_android_next_fd++;
            int write_fd = g_android_next_fd++;
            g_android_fd_table[read_fd].in_use = true;
            g_android_fd_table[read_fd].type = 5; /* Pipe */
            g_android_fd_table[write_fd].in_use = true;
            g_android_fd_table[write_fd].type = 5; /* Pipe */
            return 0;
        }
        
        case ANDROID_EXT_SYSCALL_FCNTL: {
            /* args[0] = fd, args[1] = cmd, args[2] = arg */
            uint32_t fd = args[0];
            uint32_t cmd = args[1];
            
            if (fd >= ANDROID_MAX_FDS || !g_android_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            /* Handle common fcntl commands */
            switch (cmd) {
                case 0: /* F_DUPFD */
                    if (g_android_next_fd >= ANDROID_MAX_FDS) return -24;
                    g_android_fd_table[g_android_next_fd] = g_android_fd_table[fd];
                    return g_android_next_fd++;
                case 1: /* F_GETFD */
                    return 0;
                case 2: /* F_SETFD */
                    return 0;
                case 3: /* F_GETFL */
                    return (int32_t)g_android_fd_table[fd].flags;
                case 4: /* F_SETFL */
                    g_android_fd_table[fd].flags = args[2];
                    return 0;
                default:
                    return 0;
            }
        }
        
        /* File stat operations */
        case ANDROID_EXT_SYSCALL_FSTAT:
        case ANDROID_EXT_SYSCALL_STAT:
        case ANDROID_EXT_SYSCALL_LSTAT:
            return 0; /* Success - would fill stat buffer */
        
        /* Directory operations */
        case ANDROID_EXT_SYSCALL_MKDIR:
        case ANDROID_EXT_SYSCALL_MKDIRAT:
        case ANDROID_EXT_SYSCALL_RMDIR:
            return 0;
            
        /* File operations */
        case ANDROID_EXT_SYSCALL_UNLINK:
        case ANDROID_EXT_SYSCALL_UNLINKAT:
        case ANDROID_EXT_SYSCALL_RENAME:
        case ANDROID_EXT_SYSCALL_RENAMEAT:
        case ANDROID_EXT_SYSCALL_READLINK:
        case ANDROID_EXT_SYSCALL_READLINKAT:
        case ANDROID_EXT_SYSCALL_SYMLINK:
        case ANDROID_EXT_SYSCALL_SYMLINKAT:
        case ANDROID_EXT_SYSCALL_LINK:
        case ANDROID_EXT_SYSCALL_LINKAT:
        case ANDROID_EXT_SYSCALL_CHMOD:
        case ANDROID_EXT_SYSCALL_FCHMOD:
        case ANDROID_EXT_SYSCALL_FCHMODAT:
        case ANDROID_EXT_SYSCALL_CHOWN:
        case ANDROID_EXT_SYSCALL_FCHOWN:
        case ANDROID_EXT_SYSCALL_FCHOWNAT:
        case ANDROID_EXT_SYSCALL_UTIME:
        case ANDROID_EXT_SYSCALL_UTIMES:
            return 0;
        
        /* Time operations */
        case ANDROID_EXT_SYSCALL_NANOSLEEP:
            return 0; /* Sleep completed */
            
        case ANDROID_EXT_SYSCALL_CLOCK_GETTIME:
        case ANDROID_EXT_SYSCALL_GETTIMEOFDAY:
            return 0; /* Would fill time structure */
        
        /* Resource operations */
        case ANDROID_EXT_SYSCALL_GETRLIMIT:
        case ANDROID_EXT_SYSCALL_SETRLIMIT:
        case ANDROID_EXT_SYSCALL_GETRUSAGE:
        case ANDROID_EXT_SYSCALL_SYSINFO:
            return 0;
        
        case ANDROID_EXT_SYSCALL_UNAME:
            return 0; /* Would fill utsname structure */
        
        /* Signal operations */
        case ANDROID_EXT_SYSCALL_KILL:
        case ANDROID_EXT_SYSCALL_TGKILL:
            return 0;
            
        case ANDROID_EXT_SYSCALL_RT_SIGACTION:
        case ANDROID_EXT_SYSCALL_RT_SIGPROCMASK:
        case ANDROID_EXT_SYSCALL_SIGALTSTACK:
            return 0;
        
        /* Socket operations */
        case ANDROID_EXT_SYSCALL_SOCKET: {
            /* args[0] = domain, args[1] = type, args[2] = protocol */
            if (g_android_next_sock_fd >= 100 + ANDROID_MAX_SOCKETS) {
                return -24; /* -EMFILE */
            }
            
            int sock_idx = g_android_next_sock_fd - 100;
            g_android_sockets[sock_idx].in_use = true;
            g_android_sockets[sock_idx].domain = (int)args[0];
            g_android_sockets[sock_idx].type = (int)args[1];
            g_android_sockets[sock_idx].protocol = (int)args[2];
            g_android_sockets[sock_idx].connected = false;
            g_android_sockets[sock_idx].listening = false;
            
            return g_android_next_sock_fd++;
        }
        
        case ANDROID_EXT_SYSCALL_SOCKETPAIR: {
            if (g_android_next_sock_fd + 1 >= 100 + ANDROID_MAX_SOCKETS) {
                return -24; /* -EMFILE */
            }
            int sock_idx1 = g_android_next_sock_fd - 100;
            int sock_idx2 = sock_idx1 + 1;
            g_android_sockets[sock_idx1].in_use = true;
            g_android_sockets[sock_idx2].in_use = true;
            g_android_next_sock_fd += 2;
            return 0;
        }
        
        case ANDROID_EXT_SYSCALL_BIND:
        case ANDROID_EXT_SYSCALL_LISTEN:
        case ANDROID_EXT_SYSCALL_ACCEPT:
        case ANDROID_EXT_SYSCALL_ACCEPT4:
        case ANDROID_EXT_SYSCALL_CONNECT:
        case ANDROID_EXT_SYSCALL_SEND:
        case ANDROID_EXT_SYSCALL_SENDTO:
        case ANDROID_EXT_SYSCALL_RECV:
        case ANDROID_EXT_SYSCALL_RECVFROM:
        case ANDROID_EXT_SYSCALL_SHUTDOWN:
        case ANDROID_EXT_SYSCALL_SETSOCKOPT:
        case ANDROID_EXT_SYSCALL_GETSOCKOPT:
            return 0;
        
        /* Epoll operations */
        case ANDROID_EXT_SYSCALL_EPOLL_CREATE:
        case ANDROID_EXT_SYSCALL_EPOLL_CREATE1: {
            if (g_android_next_epoll_fd >= 200 + ANDROID_MAX_EPOLL) {
                return -24; /* -EMFILE */
            }
            int epoll_idx = g_android_next_epoll_fd - 200;
            g_android_epolls[epoll_idx].in_use = true;
            g_android_epolls[epoll_idx].entry_count = 0;
            return g_android_next_epoll_fd++;
        }
        
        case ANDROID_EXT_SYSCALL_EPOLL_CTL:
            return 0;
            
        case ANDROID_EXT_SYSCALL_EPOLL_WAIT:
        case ANDROID_EXT_SYSCALL_EPOLL_PWAIT:
            return 0; /* No events ready */
        
        /* Event operations */
        case ANDROID_EXT_SYSCALL_EVENTFD:
        case ANDROID_EXT_SYSCALL_EVENTFD2: {
            if (g_android_next_fd >= ANDROID_MAX_FDS) return -24;
            int fd = g_android_next_fd++;
            g_android_fd_table[fd].in_use = true;
            g_android_fd_table[fd].type = 6; /* eventfd */
            return fd;
        }
        
        case ANDROID_EXT_SYSCALL_TIMERFD_CREATE: {
            if (g_android_next_fd >= ANDROID_MAX_FDS) return -24;
            int fd = g_android_next_fd++;
            g_android_fd_table[fd].in_use = true;
            g_android_fd_table[fd].type = 7; /* timerfd */
            return fd;
        }
        
        case ANDROID_EXT_SYSCALL_TIMERFD_SETTIME:
        case ANDROID_EXT_SYSCALL_TIMERFD_GETTIME:
            return 0;
        
        case ANDROID_EXT_SYSCALL_SIGNALFD:
        case ANDROID_EXT_SYSCALL_SIGNALFD4: {
            if (g_android_next_fd >= ANDROID_MAX_FDS) return -24;
            int fd = g_android_next_fd++;
            g_android_fd_table[fd].in_use = true;
            g_android_fd_table[fd].type = 8; /* signalfd */
            return fd;
        }
        
        /* Random and memory operations */
        case ANDROID_EXT_SYSCALL_GETRANDOM:
            return (int32_t)args[1]; /* Return requested bytes */
            
        case ANDROID_EXT_SYSCALL_MEMFD_CREATE: {
            if (g_android_next_fd >= ANDROID_MAX_FDS) return -24;
            int fd = g_android_next_fd++;
            g_android_fd_table[fd].in_use = true;
            g_android_fd_table[fd].type = 9; /* memfd */
            return fd;
        }
        
        case ANDROID_EXT_SYSCALL_MADVISE:
        case ANDROID_EXT_SYSCALL_MPROTECT:
        case ANDROID_EXT_SYSCALL_MSYNC:
        case ANDROID_EXT_SYSCALL_MLOCK:
        case ANDROID_EXT_SYSCALL_MUNLOCK:
            return 0;
        
        /* Thread operations */
        case ANDROID_EXT_SYSCALL_SET_TID_ADDRESS:
            return (int32_t)g_android_next_tid - 1;
            
        case ANDROID_EXT_SYSCALL_SET_ROBUST_LIST:
        case ANDROID_EXT_SYSCALL_GET_ROBUST_LIST:
            return 0;
        
        /* Scheduler operations */
        case ANDROID_EXT_SYSCALL_SCHED_YIELD:
            return 0;
            
        case ANDROID_EXT_SYSCALL_SCHED_GETAFFINITY:
        case ANDROID_EXT_SYSCALL_SCHED_SETAFFINITY:
            return 0;
        
        /* Architecture-specific operations */
        case ANDROID_EXT_SYSCALL_ARCH_PRCTL:
            return 0;
            
        case ANDROID_EXT_SYSCALL_SECCOMP:
            return 0;
        
        default:
            /* Unknown syscall - return -ENOSYS */
            return -38;
    }
}

int32_t android_vm_handle_syscall(AndroidVM* vm, uint32_t syscall_num, uint32_t* args) {
    if (!vm || !args) {
        return -1;
    }
    
    /* Handle common Android syscalls (Bionic libc) */
    switch (syscall_num) {
        case ANDROID_SYSCALL_EXIT:
            /* Exit process - stop VM execution */
            vm->state = ANDROID_VM_STATE_STOPPED;
            return 0;
            
        case ANDROID_SYSCALL_FORK: {
            /* Fork - create child process */
            /* In a VM context, fork creates a new process entry */
            /* Find free thread slot for child process */
            int slot = -1;
            for (int i = 0; i < ANDROID_MAX_THREADS; i++) {
                if (!g_android_threads[i].active) {
                    slot = i;
                    break;
                }
            }
            
            if (slot < 0) {
                return -11; /* -EAGAIN - no resources */
            }
            
            /* Create child process with new PID */
            uint32_t child_pid = g_android_next_tid++;
            g_android_threads[slot].active = true;
            g_android_threads[slot].tid = child_pid;
            g_android_threads[slot].pid = child_pid;
            g_android_threads[slot].parent_tid = g_android_current_pid;
            g_android_threads[slot].stack_ptr = 0;
            
            /* Return child PID to parent, would return 0 to child */
            return (int32_t)child_pid;
        }
            
        case ANDROID_SYSCALL_WRITE: {
            /* Write to file descriptor */
            /* args[0] = fd, args[1] = buf ptr, args[2] = count */
            uint32_t fd = args[0];
            uint32_t buf_ptr = args[1];
            uint32_t count = args[2];
            
            /* Validate file descriptor */
            if (fd >= ANDROID_MAX_FDS || !g_android_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            /* Handle stdout and stderr - write to console buffer */
            if (fd == 1 || fd == 2) {
                /* Write to console buffer with overflow protection */
                uint32_t bytes_to_write = count;
                
                /* Check if buffer is already full */
                if (g_android_console_pos >= ANDROID_CONSOLE_BUFFER_SIZE - 1) {
                    bytes_to_write = 0;
                } else {
                    /* Calculate available space safely */
                    uint32_t available = ANDROID_CONSOLE_BUFFER_SIZE - g_android_console_pos - 1;
                    if (bytes_to_write > available) {
                        bytes_to_write = available;
                    }
                }
                
                /* Copy data from VM memory to console buffer */
                if (vm->aurora_vm && bytes_to_write > 0) {
                    for (uint32_t i = 0; i < bytes_to_write; i++) {
                        if (buf_ptr + i < AURORA_VM_MEMORY_SIZE) {
                            g_android_console_buffer[g_android_console_pos++] = 
                                vm->aurora_vm->memory[buf_ptr + i];
                        }
                    }
                    g_android_console_buffer[g_android_console_pos] = '\0';
                }
                
                return (int32_t)count;
            }
            
            /* For regular files, update position and return count */
            g_android_fd_table[fd].position += count;
            return (int32_t)count;
        }
            
        case ANDROID_SYSCALL_READ: {
            /* Read from file descriptor */
            /* args[0] = fd, args[1] = buf ptr, args[2] = count */
            uint32_t fd = args[0];
            uint32_t buf_ptr = args[1];
            uint32_t count = args[2];
            
            /* Validate file descriptor */
            if (fd >= ANDROID_MAX_FDS || !g_android_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            /* Handle stdin - return 0 (EOF) for now */
            if (fd == 0) {
                return 0;
            }
            
            /* For regular files, calculate bytes available to read */
            uint32_t available = 0;
            if (g_android_fd_table[fd].size > g_android_fd_table[fd].position) {
                available = g_android_fd_table[fd].size - g_android_fd_table[fd].position;
            }
            
            if (count > available) {
                count = available;
            }
            
            /* Update file position */
            g_android_fd_table[fd].position += count;
            (void)buf_ptr;
            return (int32_t)count;
        }
            
        case ANDROID_SYSCALL_OPEN: {
            /* Open file (legacy syscall) */
            /* args[0] = pathname ptr, args[1] = flags, args[2] = mode */
            uint32_t flags = args[1];
            
            /* Find free fd slot */
            if (g_android_next_fd >= ANDROID_MAX_FDS) {
                return -24; /* -EMFILE */
            }
            
            int new_fd = g_android_next_fd++;
            g_android_fd_table[new_fd].in_use = true;
            g_android_fd_table[new_fd].type = 3; /* Regular file */
            g_android_fd_table[new_fd].flags = flags;
            g_android_fd_table[new_fd].position = 0;
            g_android_fd_table[new_fd].size = 0;
            
            return new_fd;
        }
            
        case ANDROID_SYSCALL_CLOSE: {
            /* Close file descriptor */
            /* args[0] = fd */
            uint32_t fd = args[0];
            
            /* Validate file descriptor */
            if (fd >= ANDROID_MAX_FDS || !g_android_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            /* Don't allow closing stdin, stdout, stderr */
            if (fd < 3) {
                return -9; /* -EBADF */
            }
            
            /* Mark fd as available */
            g_android_fd_table[fd].in_use = false;
            g_android_fd_table[fd].type = 0;
            g_android_fd_table[fd].flags = 0;
            g_android_fd_table[fd].position = 0;
            g_android_fd_table[fd].size = 0;
            g_android_fd_table[fd].path[0] = '\0';
            
            return 0;
        }
        
        case ANDROID_SYSCALL_WAITPID: {
            /* Wait for child process */
            /* args[0] = pid, args[1] = status ptr, args[2] = options */
            int32_t pid = (int32_t)args[0];
            
            /* Find matching child process */
            for (int i = 0; i < ANDROID_MAX_THREADS; i++) {
                if (g_android_threads[i].active && 
                    g_android_threads[i].parent_tid == g_android_current_pid) {
                    if (pid == -1 || (int32_t)g_android_threads[i].pid == pid) {
                        /* Return the child's PID */
                        uint32_t child_pid = g_android_threads[i].pid;
                        /* Mark thread as inactive (reaped) */
                        g_android_threads[i].active = false;
                        return (int32_t)child_pid;
                    }
                }
            }
            
            /* No child found */
            return -10; /* -ECHILD */
        }
        
        case ANDROID_SYSCALL_EXECVE:
            /* Execute program - not fully supported in VM context */
            /* Return success but don't actually exec */
            return 0;
            
        case ANDROID_SYSCALL_GETPID:
            /* Get process ID */
            return (int32_t)g_android_current_pid;
            
        case ANDROID_SYSCALL_GETUID:
            /* Get user ID - return root (0) for Android init */
            return 0;
        
        case ANDROID_SYSCALL_IOCTL: {
            /* Device I/O control */
            /* args[0] = fd, args[1] = request, args[2] = argp */
            uint32_t fd = args[0];
            uint32_t request = args[1];
            
            /* Validate file descriptor */
            if (fd >= ANDROID_MAX_FDS || !g_android_fd_table[fd].in_use) {
                return -9; /* -EBADF */
            }
            
            /* Handle common ioctl requests */
            /* TCGETS (terminal attributes) */
            if (request == 0x5401) {
                /* Not a terminal - return error */
                if (g_android_fd_table[fd].type != 0 && 
                    g_android_fd_table[fd].type != 1 && 
                    g_android_fd_table[fd].type != 2) {
                    return -25; /* -ENOTTY */
                }
                return 0; /* Success for terminal */
            }
            
            /* TIOCGWINSZ (get window size) */
            if (request == 0x5413) {
                return 0; /* Success - would fill in window size */
            }
            
            /* FIONREAD (bytes available) */
            if (request == 0x541B) {
                return 0;
            }
            
            /* Unknown request - return success by default for compatibility */
            return 0;
        }
            
        case ANDROID_SYSCALL_BRK: {
            /* Change data segment size */
            /* args[0] = new break address, 0 to query current */
            uint32_t new_brk = args[0];
            
            if (new_brk == 0) {
                /* Query current break */
                return (int32_t)g_android_current_brk;
            }
            
            /* Validate new break address */
            if (new_brk < ANDROID_HEAP_BASE || new_brk >= ANDROID_HEAP_MAX) {
                return -12; /* -ENOMEM */
            }
            
            /* Set new break address */
            g_android_current_brk = new_brk;
            return (int32_t)g_android_current_brk;
        }
            
        case ANDROID_SYSCALL_MMAP: {
            /* Memory mapping */
            /* args[0] = addr, args[1] = length, args[2] = prot, args[3] = flags */
            uint32_t addr = args[0];
            uint32_t length = args[1];
            /* uint32_t prot = args[2]; */
            /* uint32_t flags = args[3]; */
            
            /* Simple implementation: allocate from current break */
            if (addr == 0) {
                /* Check for potential overflow in alignment calculation */
                if (g_android_current_brk > (0xFFFFFFFF - 0xFFF)) {
                    return -12; /* -ENOMEM */
                }
                
                /* Allocate at current break with page alignment */
                uint32_t aligned_brk = (g_android_current_brk + 0xFFF) & ~0xFFF;
                
                /* Check for overflow in size calculation */
                if (length > ANDROID_HEAP_MAX - aligned_brk) {
                    return -12; /* -ENOMEM */
                }
                
                g_android_current_brk = aligned_brk + length;
                return (int32_t)aligned_brk;
            }
            
            /* Fixed mapping at specified address */
            return (int32_t)addr;
        }
        
        case ANDROID_SYSCALL_MUNMAP: {
            /* Memory unmap */
            /* args[0] = addr, args[1] = length */
            uint32_t addr = args[0];
            uint32_t length = args[1];
            
            /* Validate address alignment */
            if (addr & 0xFFF) {
                return -22; /* -EINVAL */
            }
            
            /* In a full implementation, we would:
             * 1. Find the mapping at addr
             * 2. Remove it from the mapping table
             * 3. Return the memory to the allocator
             * For now, just validate and return success
             */
            (void)length;
            return 0;
        }
            
        case ANDROID_SYSCALL_CLONE: {
            /* Create child process/thread */
            /* args[0] = flags, args[1] = stack, args[2] = parent_tid ptr, args[3] = child_tid ptr */
            uint32_t flags = args[0];
            uint32_t stack = args[1];
            (void)flags; /* Clone flags determine behavior */
            (void)stack; /* New stack for child */
            
            /* Find free thread slot */
            int slot = -1;
            for (int i = 0; i < ANDROID_MAX_THREADS; i++) {
                if (!g_android_threads[i].active) {
                    slot = i;
                    break;
                }
            }
            
            if (slot < 0) {
                return -11; /* -EAGAIN */
            }
            
            /* Create new thread */
            uint32_t new_tid = g_android_next_tid++;
            g_android_threads[slot].active = true;
            g_android_threads[slot].tid = new_tid;
            g_android_threads[slot].pid = g_android_current_pid;
            g_android_threads[slot].parent_tid = 1;
            g_android_threads[slot].stack_ptr = stack;
            
            return (int32_t)new_tid;
        }
            
        case ANDROID_SYSCALL_PRCTL: {
            /* Process control operations */
            /* args[0] = option, args[1-4] = arguments */
            uint32_t option = args[0];
            
            /* Handle common prctl options */
            switch (option) {
                case 15: /* PR_SET_NAME - set thread name */
                    /* Accept the request but don't store the name */
                    return 0;
                case 16: /* PR_GET_NAME - get thread name */
                    /* Return empty string */
                    return 0;
                case 38: /* PR_SET_NO_NEW_PRIVS */
                    return 0;
                case 22: /* PR_SET_SECCOMP */
                    return 0;
                case 28: /* PR_CAPBSET_READ */
                    return 1; /* Capability is in bounding set */
                case 25: /* PR_CAPBSET_DROP */
                    return 0;
                default:
                    return 0; /* Success for unhandled options */
            }
        }
            
        case ANDROID_SYSCALL_FUTEX: {
            /* Fast userspace mutex */
            /* args[0] = uaddr, args[1] = op, args[2] = val, args[3] = timeout, args[4] = uaddr2 */
            uint32_t uaddr = args[0];
            uint32_t op = args[1];
            uint32_t val = args[2];
            
            /* Futex operations */
            #define FUTEX_WAIT 0
            #define FUTEX_WAKE 1
            #define FUTEX_REQUEUE 3
            #define FUTEX_CMP_REQUEUE 4
            #define FUTEX_WAKE_OP 5
            #define FUTEX_WAIT_BITSET 9
            #define FUTEX_WAKE_BITSET 10
            
            switch (op & 0x7F) {
                case FUTEX_WAIT:
                case FUTEX_WAIT_BITSET:
                    /* Wait on futex - for simplicity, return immediately */
                    (void)uaddr;
                    (void)val;
                    return 0;
                case FUTEX_WAKE:
                case FUTEX_WAKE_BITSET:
                    /* Wake waiters - return number of waiters woken */
                    return 1;
                case FUTEX_REQUEUE:
                case FUTEX_CMP_REQUEUE:
                    /* Requeue waiters */
                    return 0;
                case FUTEX_WAKE_OP:
                    /* Wake with operation */
                    return 1;
                default:
                    return 0;
            }
        }
            
        case ANDROID_SYSCALL_OPENAT: {
            /* Open file relative to directory fd */
            /* args[0] = dirfd, args[1] = pathname ptr, args[2] = flags, args[3] = mode */
            /* int32_t dirfd = (int32_t)args[0]; */
            /* uint32_t pathname_ptr = args[1]; */
            uint32_t flags = args[2];
            /* uint32_t mode = args[3]; */
            
            /* Find free fd slot */
            if (g_android_next_fd >= ANDROID_MAX_FDS) {
                return -24; /* -EMFILE */
            }
            
            int new_fd = g_android_next_fd++;
            g_android_fd_table[new_fd].in_use = true;
            g_android_fd_table[new_fd].type = 3; /* Regular file */
            g_android_fd_table[new_fd].flags = flags;
            g_android_fd_table[new_fd].position = 0;
            g_android_fd_table[new_fd].size = 0;
            
            return new_fd;
        }
        
        case ANDROID_SYSCALL_FACCESSAT: {
            /* Check file accessibility */
            /* args[0] = dirfd, args[1] = pathname ptr, args[2] = mode, args[3] = flags */
            /* For now, return success (file exists and is accessible) */
            return 0;
        }
            
        default:
            /* Check for additional syscalls in extended range */
            return android_vm_handle_extended_syscall(vm, syscall_num, args);
    }
}

int android_vm_set_property(AndroidVM* vm, const char* name, const char* value) {
    if (!vm || !name || !value) {
        return -1;
    }
    
    /* Check if property already exists */
    for (uint32_t i = 0; i < g_property_count; i++) {
        if (platform_strcmp(g_android_properties[i].name, name) == 0) {
            /* Update existing property */
            platform_strncpy(g_android_properties[i].value, value, sizeof(g_android_properties[i].value));
            return 0;
        }
    }
    
    /* Add new property */
    if (g_property_count >= 64) {
        return -1; /* Property table full */
    }
    
    platform_strncpy(g_android_properties[g_property_count].name, name, sizeof(g_android_properties[g_property_count].name));
    platform_strncpy(g_android_properties[g_property_count].value, value, sizeof(g_android_properties[g_property_count].value));
    g_property_count++;
    
    return 0;
}

int android_vm_get_property(AndroidVM* vm, const char* name, char* value, uint32_t size) {
    if (!vm || !name || !value || size == 0) {
        return -1;
    }
    
    /* Search for property */
    for (uint32_t i = 0; i < g_property_count; i++) {
        if (platform_strcmp(g_android_properties[i].name, name) == 0) {
            platform_strncpy(value, g_android_properties[i].value, size);
            return 0;
        }
    }
    
    return -1; /* Property not found */
}

int android_vm_enable_dalvik(AndroidVM* vm, bool enable) {
    if (!vm) {
        return -1;
    }
    
    vm->dalvik_enabled = enable;
    
    return 0;
}

bool android_vm_is_available(void) {
    return g_android_vm_initialized;
}

const char* android_vm_get_version(void) {
    return ANDROID_VM_VERSION;
}

const char* android_vm_get_arch_name(android_arch_t arch) {
    if (arch >= 0 && arch < 4) {
        return arch_names[arch];
    }
    return "Unknown";
}

uint32_t android_vm_get_syscall_count(void) {
    return ANDROID_SYSCALL_COUNT;
}

bool android_vm_is_syscall_implemented(uint32_t syscall_num) {
    /* Check if syscall is in the main handler */
    switch (syscall_num) {
        case ANDROID_SYSCALL_EXIT:
        case ANDROID_SYSCALL_FORK:
        case ANDROID_SYSCALL_READ:
        case ANDROID_SYSCALL_WRITE:
        case ANDROID_SYSCALL_OPEN:
        case ANDROID_SYSCALL_CLOSE:
        case ANDROID_SYSCALL_WAITPID:
        case ANDROID_SYSCALL_EXECVE:
        case ANDROID_SYSCALL_GETPID:
        case ANDROID_SYSCALL_GETUID:
        case ANDROID_SYSCALL_IOCTL:
        case ANDROID_SYSCALL_BRK:
        case ANDROID_SYSCALL_MMAP:
        case ANDROID_SYSCALL_MUNMAP:
        case ANDROID_SYSCALL_CLONE:
        case ANDROID_SYSCALL_PRCTL:
        case ANDROID_SYSCALL_FUTEX:
        case ANDROID_SYSCALL_OPENAT:
        case ANDROID_SYSCALL_FACCESSAT:
            return true;
        default:
            break;
    }
    
    /* Check extended syscalls */
    switch (syscall_num) {
        case ANDROID_EXT_SYSCALL_LSEEK:
        case ANDROID_EXT_SYSCALL_GETPPID:
        case ANDROID_EXT_SYSCALL_GETTID:
        case ANDROID_EXT_SYSCALL_GETEUID:
        case ANDROID_EXT_SYSCALL_GETEGID:
        case ANDROID_EXT_SYSCALL_SETUID:
        case ANDROID_EXT_SYSCALL_SETGID:
        case ANDROID_EXT_SYSCALL_UMASK:
        case ANDROID_EXT_SYSCALL_CHDIR:
        case ANDROID_EXT_SYSCALL_FCHDIR:
        case ANDROID_EXT_SYSCALL_GETCWD:
        case ANDROID_EXT_SYSCALL_DUP:
        case ANDROID_EXT_SYSCALL_DUP2:
        case ANDROID_EXT_SYSCALL_DUP3:
        case ANDROID_EXT_SYSCALL_PIPE:
        case ANDROID_EXT_SYSCALL_PIPE2:
        case ANDROID_EXT_SYSCALL_FCNTL:
        case ANDROID_EXT_SYSCALL_FSTAT:
        case ANDROID_EXT_SYSCALL_STAT:
        case ANDROID_EXT_SYSCALL_LSTAT:
        case ANDROID_EXT_SYSCALL_MKDIR:
        case ANDROID_EXT_SYSCALL_MKDIRAT:
        case ANDROID_EXT_SYSCALL_RMDIR:
        case ANDROID_EXT_SYSCALL_UNLINK:
        case ANDROID_EXT_SYSCALL_UNLINKAT:
        case ANDROID_EXT_SYSCALL_RENAME:
        case ANDROID_EXT_SYSCALL_RENAMEAT:
        case ANDROID_EXT_SYSCALL_NANOSLEEP:
        case ANDROID_EXT_SYSCALL_CLOCK_GETTIME:
        case ANDROID_EXT_SYSCALL_GETTIMEOFDAY:
        case ANDROID_EXT_SYSCALL_KILL:
        case ANDROID_EXT_SYSCALL_TGKILL:
        case ANDROID_EXT_SYSCALL_RT_SIGACTION:
        case ANDROID_EXT_SYSCALL_RT_SIGPROCMASK:
        case ANDROID_EXT_SYSCALL_SOCKET:
        case ANDROID_EXT_SYSCALL_SOCKETPAIR:
        case ANDROID_EXT_SYSCALL_BIND:
        case ANDROID_EXT_SYSCALL_LISTEN:
        case ANDROID_EXT_SYSCALL_ACCEPT:
        case ANDROID_EXT_SYSCALL_CONNECT:
        case ANDROID_EXT_SYSCALL_EPOLL_CREATE:
        case ANDROID_EXT_SYSCALL_EPOLL_CREATE1:
        case ANDROID_EXT_SYSCALL_EPOLL_CTL:
        case ANDROID_EXT_SYSCALL_EPOLL_WAIT:
        case ANDROID_EXT_SYSCALL_EVENTFD:
        case ANDROID_EXT_SYSCALL_EVENTFD2:
        case ANDROID_EXT_SYSCALL_TIMERFD_CREATE:
        case ANDROID_EXT_SYSCALL_GETRANDOM:
        case ANDROID_EXT_SYSCALL_MEMFD_CREATE:
        case ANDROID_EXT_SYSCALL_MADVISE:
        case ANDROID_EXT_SYSCALL_MPROTECT:
        case ANDROID_EXT_SYSCALL_SET_TID_ADDRESS:
        case ANDROID_EXT_SYSCALL_SCHED_YIELD:
        case ANDROID_EXT_SYSCALL_SECCOMP:
            return true;
        default:
            return false;
    }
}

const char* android_vm_get_console_output(void) {
    return g_android_console_buffer;
}

void android_vm_clear_console(void) {
    g_android_console_pos = 0;
    g_android_console_buffer[0] = '\0';
}
