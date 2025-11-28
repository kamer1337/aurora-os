/**
 * @file syscall_table.c
 * @brief Extended Syscall Table Implementation (200+ syscalls)
 *
 * Comprehensive Linux/Android syscall emulation for broad compatibility
 */

#include "../../include/platform/android_vm.h"
#include "../../include/platform/linux_vm.h"
#include "../../include/platform/platform_util.h"

/* ============================================================================
 * EXTENDED SYSCALL DEFINITIONS (Linux x86-64 compatible)
 * ============================================================================ */

/* Syscall numbers for Linux x86-64 ABI */
typedef enum {
    /* File operations */
    SYS_READ            = 0,
    SYS_WRITE           = 1,
    SYS_OPEN            = 2,
    SYS_CLOSE           = 3,
    SYS_STAT            = 4,
    SYS_FSTAT           = 5,
    SYS_LSTAT           = 6,
    SYS_POLL            = 7,
    SYS_LSEEK           = 8,
    SYS_MMAP            = 9,
    SYS_MPROTECT        = 10,
    SYS_MUNMAP          = 11,
    SYS_BRK             = 12,
    SYS_RT_SIGACTION    = 13,
    SYS_RT_SIGPROCMASK  = 14,
    SYS_RT_SIGRETURN    = 15,
    SYS_IOCTL           = 16,
    SYS_PREAD64         = 17,
    SYS_PWRITE64        = 18,
    SYS_READV           = 19,
    SYS_WRITEV          = 20,
    SYS_ACCESS          = 21,
    SYS_PIPE            = 22,
    SYS_SELECT          = 23,
    SYS_SCHED_YIELD     = 24,
    SYS_MREMAP          = 25,
    SYS_MSYNC           = 26,
    SYS_MINCORE         = 27,
    SYS_MADVISE         = 28,
    SYS_SHMGET          = 29,
    SYS_SHMAT           = 30,
    SYS_SHMCTL          = 31,
    SYS_DUP             = 32,
    SYS_DUP2            = 33,
    SYS_PAUSE           = 34,
    SYS_NANOSLEEP       = 35,
    SYS_GETITIMER       = 36,
    SYS_ALARM           = 37,
    SYS_SETITIMER       = 38,
    SYS_GETPID          = 39,
    SYS_SENDFILE        = 40,
    SYS_SOCKET          = 41,
    SYS_CONNECT         = 42,
    SYS_ACCEPT          = 43,
    SYS_SENDTO          = 44,
    SYS_RECVFROM        = 45,
    SYS_SENDMSG         = 46,
    SYS_RECVMSG         = 47,
    SYS_SHUTDOWN        = 48,
    SYS_BIND            = 49,
    SYS_LISTEN          = 50,
    SYS_GETSOCKNAME     = 51,
    SYS_GETPEERNAME     = 52,
    SYS_SOCKETPAIR      = 53,
    SYS_SETSOCKOPT      = 54,
    SYS_GETSOCKOPT      = 55,
    SYS_CLONE           = 56,
    SYS_FORK            = 57,
    SYS_VFORK           = 58,
    SYS_EXECVE          = 59,
    SYS_EXIT            = 60,
    SYS_WAIT4           = 61,
    SYS_KILL            = 62,
    SYS_UNAME           = 63,
    SYS_SEMGET          = 64,
    SYS_SEMOP           = 65,
    SYS_SEMCTL          = 66,
    SYS_SHMDT           = 67,
    SYS_MSGGET          = 68,
    SYS_MSGSND          = 69,
    SYS_MSGRCV          = 70,
    SYS_MSGCTL          = 71,
    SYS_FCNTL           = 72,
    SYS_FLOCK           = 73,
    SYS_FSYNC           = 74,
    SYS_FDATASYNC       = 75,
    SYS_TRUNCATE        = 76,
    SYS_FTRUNCATE       = 77,
    SYS_GETDENTS        = 78,
    SYS_GETCWD          = 79,
    SYS_CHDIR           = 80,
    SYS_FCHDIR          = 81,
    SYS_RENAME          = 82,
    SYS_MKDIR           = 83,
    SYS_RMDIR           = 84,
    SYS_CREAT           = 85,
    SYS_LINK            = 86,
    SYS_UNLINK          = 87,
    SYS_SYMLINK         = 88,
    SYS_READLINK        = 89,
    SYS_CHMOD           = 90,
    SYS_FCHMOD          = 91,
    SYS_CHOWN           = 92,
    SYS_FCHOWN          = 93,
    SYS_LCHOWN          = 94,
    SYS_UMASK           = 95,
    SYS_GETTIMEOFDAY    = 96,
    SYS_GETRLIMIT       = 97,
    SYS_GETRUSAGE       = 98,
    SYS_SYSINFO         = 99,
    SYS_TIMES           = 100,
    SYS_PTRACE          = 101,
    SYS_GETUID          = 102,
    SYS_SYSLOG          = 103,
    SYS_GETGID          = 104,
    SYS_SETUID          = 105,
    SYS_SETGID          = 106,
    SYS_GETEUID         = 107,
    SYS_GETEGID         = 108,
    SYS_SETPGID         = 109,
    SYS_GETPPID         = 110,
    SYS_GETPGRP         = 111,
    SYS_SETSID          = 112,
    SYS_SETREUID        = 113,
    SYS_SETREGID        = 114,
    SYS_GETGROUPS       = 115,
    SYS_SETGROUPS       = 116,
    SYS_SETRESUID       = 117,
    SYS_GETRESUID       = 118,
    SYS_SETRESGID       = 119,
    SYS_GETRESGID       = 120,
    SYS_GETPGID         = 121,
    SYS_SETFSUID        = 122,
    SYS_SETFSGID        = 123,
    SYS_GETSID          = 124,
    SYS_CAPGET          = 125,
    SYS_CAPSET          = 126,
    SYS_RT_SIGPENDING   = 127,
    SYS_RT_SIGTIMEDWAIT = 128,
    SYS_RT_SIGQUEUEINFO = 129,
    SYS_RT_SIGSUSPEND   = 130,
    SYS_SIGALTSTACK     = 131,
    SYS_UTIME           = 132,
    SYS_MKNOD           = 133,
    SYS_USELIB          = 134,
    SYS_PERSONALITY     = 135,
    SYS_USTAT           = 136,
    SYS_STATFS          = 137,
    SYS_FSTATFS         = 138,
    SYS_SYSFS           = 139,
    SYS_GETPRIORITY     = 140,
    SYS_SETPRIORITY     = 141,
    SYS_SCHED_SETPARAM  = 142,
    SYS_SCHED_GETPARAM  = 143,
    SYS_SCHED_SETSCHEDULER = 144,
    SYS_SCHED_GETSCHEDULER = 145,
    SYS_SCHED_GET_PRIORITY_MAX = 146,
    SYS_SCHED_GET_PRIORITY_MIN = 147,
    SYS_SCHED_RR_GET_INTERVAL = 148,
    SYS_MLOCK           = 149,
    SYS_MUNLOCK         = 150,
    SYS_MLOCKALL        = 151,
    SYS_MUNLOCKALL      = 152,
    SYS_VHANGUP         = 153,
    SYS_MODIFY_LDT      = 154,
    SYS_PIVOT_ROOT      = 155,
    SYS_SYSCTL          = 156,
    SYS_PRCTL           = 157,
    SYS_ARCH_PRCTL      = 158,
    SYS_ADJTIMEX        = 159,
    SYS_SETRLIMIT       = 160,
    SYS_CHROOT          = 161,
    SYS_SYNC            = 162,
    SYS_ACCT            = 163,
    SYS_SETTIMEOFDAY    = 164,
    SYS_MOUNT           = 165,
    SYS_UMOUNT2         = 166,
    SYS_SWAPON          = 167,
    SYS_SWAPOFF         = 168,
    SYS_REBOOT          = 169,
    SYS_SETHOSTNAME     = 170,
    SYS_SETDOMAINNAME   = 171,
    SYS_IOPL            = 172,
    SYS_IOPERM          = 173,
    SYS_CREATE_MODULE   = 174,
    SYS_INIT_MODULE     = 175,
    SYS_DELETE_MODULE   = 176,
    SYS_GET_KERNEL_SYMS = 177,
    SYS_QUERY_MODULE    = 178,
    SYS_QUOTACTL        = 179,
    SYS_NFSSERVCTL      = 180,
    SYS_GETPMSG         = 181,
    SYS_PUTPMSG         = 182,
    SYS_AFS_SYSCALL     = 183,
    SYS_TUXCALL         = 184,
    SYS_SECURITY        = 185,
    SYS_GETTID          = 186,
    SYS_READAHEAD       = 187,
    SYS_SETXATTR        = 188,
    SYS_LSETXATTR       = 189,
    SYS_FSETXATTR       = 190,
    SYS_GETXATTR        = 191,
    SYS_LGETXATTR       = 192,
    SYS_FGETXATTR       = 193,
    SYS_LISTXATTR       = 194,
    SYS_LLISTXATTR      = 195,
    SYS_FLISTXATTR      = 196,
    SYS_REMOVEXATTR     = 197,
    SYS_LREMOVEXATTR    = 198,
    SYS_FREMOVEXATTR    = 199,
    SYS_TKILL           = 200,
    SYS_TIME            = 201,
    SYS_FUTEX           = 202,
    SYS_SCHED_SETAFFINITY = 203,
    SYS_SCHED_GETAFFINITY = 204,
    SYS_SET_THREAD_AREA = 205,
    SYS_IO_SETUP        = 206,
    SYS_IO_DESTROY      = 207,
    SYS_IO_GETEVENTS    = 208,
    SYS_IO_SUBMIT       = 209,
    SYS_IO_CANCEL       = 210,
    SYS_GET_THREAD_AREA = 211,
    SYS_LOOKUP_DCOOKIE  = 212,
    SYS_EPOLL_CREATE    = 213,
    SYS_EPOLL_CTL_OLD   = 214,
    SYS_EPOLL_WAIT_OLD  = 215,
    SYS_REMAP_FILE_PAGES = 216,
    SYS_GETDENTS64      = 217,
    SYS_SET_TID_ADDRESS = 218,
    SYS_RESTART_SYSCALL = 219,
    SYS_SEMTIMEDOP      = 220,
    SYS_FADVISE64       = 221,
    SYS_TIMER_CREATE    = 222,
    SYS_TIMER_SETTIME   = 223,
    SYS_TIMER_GETTIME   = 224,
    SYS_TIMER_GETOVERRUN = 225,
    SYS_TIMER_DELETE    = 226,
    SYS_CLOCK_SETTIME   = 227,
    SYS_CLOCK_GETTIME   = 228,
    SYS_CLOCK_GETRES    = 229,
    SYS_CLOCK_NANOSLEEP = 230,
    SYS_EXIT_GROUP      = 231,
    SYS_EPOLL_WAIT      = 232,
    SYS_EPOLL_CTL       = 233,
    SYS_TGKILL          = 234,
    SYS_UTIMES          = 235,
    SYS_VSERVER         = 236,
    SYS_MBIND           = 237,
    SYS_SET_MEMPOLICY   = 238,
    SYS_GET_MEMPOLICY   = 239,
    SYS_MQ_OPEN         = 240,
    SYS_MQ_UNLINK       = 241,
    SYS_MQ_TIMEDSEND    = 242,
    SYS_MQ_TIMEDRECEIVE = 243,
    SYS_MQ_NOTIFY       = 244,
    SYS_MQ_GETSETATTR   = 245,
    SYS_KEXEC_LOAD      = 246,
    SYS_WAITID          = 247,
    SYS_ADD_KEY         = 248,
    SYS_REQUEST_KEY     = 249,
    SYS_KEYCTL          = 250,
    SYS_IOPRIO_SET      = 251,
    SYS_IOPRIO_GET      = 252,
    SYS_INOTIFY_INIT    = 253,
    SYS_INOTIFY_ADD_WATCH = 254,
    SYS_INOTIFY_RM_WATCH = 255,
    SYS_MIGRATE_PAGES   = 256,
    SYS_OPENAT          = 257,
    SYS_MKDIRAT         = 258,
    SYS_MKNODAT         = 259,
    SYS_FCHOWNAT        = 260,
    SYS_FUTIMESAT       = 261,
    SYS_NEWFSTATAT      = 262,
    SYS_UNLINKAT        = 263,
    SYS_RENAMEAT        = 264,
    SYS_LINKAT          = 265,
    SYS_SYMLINKAT       = 266,
    SYS_READLINKAT      = 267,
    SYS_FCHMODAT        = 268,
    SYS_FACCESSAT       = 269,
    SYS_PSELECT6        = 270,
    SYS_PPOLL           = 271,
    SYS_UNSHARE         = 272,
    SYS_SET_ROBUST_LIST = 273,
    SYS_GET_ROBUST_LIST = 274,
    SYS_SPLICE          = 275,
    SYS_TEE             = 276,
    SYS_SYNC_FILE_RANGE = 277,
    SYS_VMSPLICE        = 278,
    SYS_MOVE_PAGES      = 279,
    SYS_UTIMENSAT       = 280,
    SYS_EPOLL_PWAIT     = 281,
    SYS_SIGNALFD        = 282,
    SYS_TIMERFD_CREATE  = 283,
    SYS_EVENTFD         = 284,
    SYS_FALLOCATE       = 285,
    SYS_TIMERFD_SETTIME = 286,
    SYS_TIMERFD_GETTIME = 287,
    SYS_ACCEPT4         = 288,
    SYS_SIGNALFD4       = 289,
    SYS_EVENTFD2        = 290,
    SYS_EPOLL_CREATE1   = 291,
    SYS_DUP3            = 292,
    SYS_PIPE2           = 293,
    SYS_INOTIFY_INIT1   = 294,
    SYS_PREADV          = 295,
    SYS_PWRITEV         = 296,
    SYS_RT_TGSIGQUEUEINFO = 297,
    SYS_PERF_EVENT_OPEN = 298,
    SYS_RECVMMSG        = 299,
    SYS_FANOTIFY_INIT   = 300,
    SYS_FANOTIFY_MARK   = 301,
    SYS_PRLIMIT64       = 302,
    SYS_NAME_TO_HANDLE_AT = 303,
    SYS_OPEN_BY_HANDLE_AT = 304,
    SYS_CLOCK_ADJTIME   = 305,
    SYS_SYNCFS          = 306,
    SYS_SENDMMSG        = 307,
    SYS_SETNS           = 308,
    SYS_GETCPU          = 309,
    SYS_PROCESS_VM_READV = 310,
    SYS_PROCESS_VM_WRITEV = 311,
    SYS_KCMP            = 312,
    SYS_FINIT_MODULE    = 313,
    SYS_SCHED_SETATTR   = 314,
    SYS_SCHED_GETATTR   = 315,
    SYS_RENAMEAT2       = 316,
    SYS_SECCOMP         = 317,
    SYS_GETRANDOM       = 318,
    SYS_MEMFD_CREATE    = 319,
    SYS_KEXEC_FILE_LOAD = 320,
    SYS_BPF             = 321,
    SYS_EXECVEAT        = 322,
    SYS_USERFAULTFD     = 323,
    SYS_MEMBARRIER      = 324,
    SYS_MLOCK2          = 325,
    SYS_COPY_FILE_RANGE = 326,
    SYS_PREADV2         = 327,
    SYS_PWRITEV2        = 328,
    SYS_PKEY_MPROTECT   = 329,
    SYS_PKEY_ALLOC      = 330,
    SYS_PKEY_FREE       = 331,
    SYS_STATX           = 332,
    SYS_IO_PGETEVENTS   = 333,
    SYS_RSEQ            = 334,
    /* Syscall count */
    SYS_COUNT           = 335
} syscall_number_t;

/* ============================================================================
 * SYSCALL HANDLER TYPE
 * ============================================================================ */

typedef int32_t (*syscall_handler_t)(void* vm, uint32_t* args);

/* ============================================================================
 * GENERIC SYSCALL IMPLEMENTATIONS
 * ============================================================================ */

/* Process state tracking */
static uint32_t g_current_pid = 1;
static uint32_t g_current_tid = 1;
static uint32_t g_current_uid = 0;
static uint32_t g_current_gid = 0;
static uint32_t g_umask_value = 0022;
static char g_current_dir[256] = "/";
static char g_hostname[64] = "aurora";
static char g_domainname[64] = "localdomain";

/* File descriptor tracking */
#define MAX_FDS 256
#define MAX_PIPES 32
#define MAX_SOCKETS 32

typedef struct {
    bool open;
    uint32_t flags;
    uint32_t offset;
    uint32_t size;
    uint8_t type;  /* 0=file, 1=pipe_read, 2=pipe_write, 3=socket */
} fd_entry_t;

typedef struct {
    bool used;
    int32_t read_fd;
    int32_t write_fd;
    uint8_t buffer[4096];
    uint32_t read_pos;
    uint32_t write_pos;
} pipe_entry_t;

typedef struct {
    bool used;
    int32_t fd;
    uint16_t family;
    uint16_t type;
    uint16_t protocol;
    bool connected;
    bool listening;
} socket_entry_t;

static fd_entry_t g_fds[MAX_FDS] = {{0}};
static pipe_entry_t g_pipes[MAX_PIPES] = {{0}};
static socket_entry_t g_sockets[MAX_SOCKETS] = {{0}};
static uint32_t g_next_fd = 3;  /* Start after stdin, stdout, stderr */
static uint64_t g_brk_addr = 0x10000000;  /* Initial break address */
static uint64_t g_timer_ticks = 0;

/* Memory mapping tracking */
#define MAX_MMAPS 64
typedef struct {
    bool used;
    uint64_t addr;
    uint64_t size;
    uint32_t prot;
    uint32_t flags;
} mmap_entry_t;

static mmap_entry_t g_mmaps[MAX_MMAPS] = {{0}};
static uint64_t g_mmap_base = 0x40000000;

/* Signal handling */
#define MAX_SIGNALS 64
typedef struct {
    uint64_t handler;
    uint64_t flags;
    uint64_t mask;
} sigaction_entry_t;

static sigaction_entry_t g_sigactions[MAX_SIGNALS] = {{0}};
static uint64_t g_sigmask = 0;

/* Helper functions */
static int32_t alloc_fd(void) {
    for (uint32_t i = g_next_fd; i < MAX_FDS; i++) {
        if (!g_fds[i].open) {
            g_fds[i].open = true;
            g_fds[i].flags = 0;
            g_fds[i].offset = 0;
            g_fds[i].size = 0;
            g_fds[i].type = 0;
            return (int32_t)i;
        }
    }
    return -24;  /* EMFILE */
}

static void free_fd(int32_t fd) {
    if (fd >= 0 && fd < MAX_FDS) {
        g_fds[fd].open = false;
    }
}

/* Implementation functions */
static int32_t sys_exit_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t status = (int32_t)args[0];
    return status;
}

static int32_t sys_getpid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_pid;
}

static int32_t sys_getppid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (g_current_pid > 1) ? 1 : 0;
}

static int32_t sys_gettid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_tid;
}

static int32_t sys_getuid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_uid;
}

static int32_t sys_geteuid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_uid;
}

static int32_t sys_getgid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_gid;
}

static int32_t sys_getegid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_gid;
}

static int32_t sys_setuid_impl(void* vm, uint32_t* args) {
    (void)vm;
    g_current_uid = args[0];
    return 0;
}

static int32_t sys_setgid_impl(void* vm, uint32_t* args) {
    (void)vm;
    g_current_gid = args[0];
    return 0;
}

static int32_t sys_umask_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t old_umask = g_umask_value;
    g_umask_value = args[0] & 0777;
    return (int32_t)old_umask;
}

static int32_t sys_getcwd_impl(void* vm, uint32_t* args) {
    (void)vm;
    char* buf = (char*)(uintptr_t)args[0];
    uint32_t size = args[1];
    if (!buf || size == 0) return -14;  /* EFAULT */
    uint32_t len = platform_strlen(g_current_dir);
    if (len + 1 > size) return -34;  /* ERANGE */
    platform_strncpy(buf, g_current_dir, size);
    return (int32_t)len;
}

static int32_t sys_chdir_impl(void* vm, uint32_t* args) {
    (void)vm;
    const char* path = (const char*)(uintptr_t)args[0];
    if (!path) return -14;  /* EFAULT */
    if (path[0] == '/') {
        platform_strncpy(g_current_dir, path, sizeof(g_current_dir));
    }
    return 0;
}

static int32_t sys_fchdir_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    if (fd < 0 || fd >= MAX_FDS || !g_fds[fd].open) return -9;  /* EBADF */
    return 0;
}

static int32_t sys_uname_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
    } *buf = (void*)(uintptr_t)args[0];
    if (!buf) return -14;  /* EFAULT */
    platform_strncpy(buf->sysname, "Aurora", 65);
    platform_strncpy(buf->nodename, g_hostname, 65);
    platform_strncpy(buf->release, "6.6.0-aurora", 65);
    platform_strncpy(buf->version, "#1 SMP Aurora OS", 65);
    platform_strncpy(buf->machine, "x86_64", 65);
    return 0;
}

static int32_t sys_sethostname_impl(void* vm, uint32_t* args) {
    (void)vm;
    const char* name = (const char*)(uintptr_t)args[0];
    uint32_t len = args[1];
    if (!name) return -14;  /* EFAULT */
    if (len >= sizeof(g_hostname)) return -36;  /* ENAMETOOLONG */
    platform_strncpy(g_hostname, name, sizeof(g_hostname));
    return 0;
}

static int32_t sys_setdomainname_impl(void* vm, uint32_t* args) {
    (void)vm;
    const char* name = (const char*)(uintptr_t)args[0];
    uint32_t len = args[1];
    if (!name) return -14;  /* EFAULT */
    if (len >= sizeof(g_domainname)) return -36;  /* ENAMETOOLONG */
    platform_strncpy(g_domainname, name, sizeof(g_domainname));
    return 0;
}

static int32_t sys_gettimeofday_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t tv_sec;
        uint64_t tv_usec;
    } *tv = (void*)(uintptr_t)args[0];
    if (tv) {
        tv->tv_sec = g_timer_ticks / 1000000;
        tv->tv_usec = g_timer_ticks % 1000000;
    }
    return 0;
}

static int32_t sys_settimeofday_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t tv_sec;
        uint64_t tv_usec;
    } *tv = (void*)(uintptr_t)args[0];
    if (tv) {
        g_timer_ticks = tv->tv_sec * 1000000 + tv->tv_usec;
    }
    return 0;
}

static int32_t sys_clock_gettime_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t tv_sec;
        uint64_t tv_nsec;
    } *tp = (void*)(uintptr_t)args[1];
    if (tp) {
        tp->tv_sec = g_timer_ticks / 1000000;
        tp->tv_nsec = (g_timer_ticks % 1000000) * 1000;
    }
    return 0;
}

static int32_t sys_clock_settime_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t tv_sec;
        uint64_t tv_nsec;
    } *tp = (void*)(uintptr_t)args[1];
    if (tp) {
        g_timer_ticks = tp->tv_sec * 1000000 + tp->tv_nsec / 1000;
    }
    return 0;
}

static int32_t sys_clock_getres_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t tv_sec;
        uint64_t tv_nsec;
    } *res = (void*)(uintptr_t)args[1];
    if (res) {
        res->tv_sec = 0;
        res->tv_nsec = 1000;  /* 1 microsecond resolution */
    }
    return 0;
}

static int32_t sys_nanosleep_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t tv_sec;
        uint64_t tv_nsec;
    } *req = (void*)(uintptr_t)args[0];
    if (req) {
        g_timer_ticks += req->tv_sec * 1000000 + req->tv_nsec / 1000;
    }
    return 0;
}

static int32_t sys_sched_yield_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_rt_sigaction_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t signum = (int32_t)args[0];
    if (signum < 1 || signum >= MAX_SIGNALS) return -22;  /* EINVAL */
    return 0;
}

static int32_t sys_rt_sigprocmask_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t how = (int32_t)args[0];
    uint64_t* set = (uint64_t*)(uintptr_t)args[1];
    uint64_t* oldset = (uint64_t*)(uintptr_t)args[2];
    if (oldset) *oldset = g_sigmask;
    if (set) {
        switch (how) {
            case 0: g_sigmask |= *set; break;  /* SIG_BLOCK */
            case 1: g_sigmask &= ~(*set); break;  /* SIG_UNBLOCK */
            case 2: g_sigmask = *set; break;  /* SIG_SETMASK */
        }
    }
    return 0;
}

static int32_t sys_rt_sigpending_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint64_t* set = (uint64_t*)(uintptr_t)args[0];
    if (set) *set = 0;  /* No pending signals */
    return 0;
}

static int32_t sys_sigaltstack_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* Accept but ignore */
}

static int32_t sys_kill_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t pid = (int32_t)args[0];
    int32_t sig = (int32_t)args[1];
    if (sig < 0 || sig >= MAX_SIGNALS) return -22;  /* EINVAL */
    if (pid <= 0 || (uint32_t)pid == g_current_pid) return 0;
    return -3;  /* ESRCH - process not found */
}

static int32_t sys_tgkill_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t tgid = (int32_t)args[0];
    int32_t tid = (int32_t)args[1];
    int32_t sig = (int32_t)args[2];
    if (sig < 0 || sig >= MAX_SIGNALS) return -22;  /* EINVAL */
    if ((uint32_t)tgid != g_current_pid || (uint32_t)tid != g_current_tid) return -3;  /* ESRCH */
    return 0;
}

static int32_t sys_tkill_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t tid = (int32_t)args[0];
    int32_t sig = (int32_t)args[1];
    if (sig < 0 || sig >= MAX_SIGNALS) return -22;  /* EINVAL */
    if ((uint32_t)tid != g_current_tid) return -3;  /* ESRCH */
    return 0;
}

static int32_t sys_prctl_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t option = args[0];
    switch (option) {
        case 15: case 16: case 38: return 0;
        default: return 0;
    }
}

static int32_t sys_arch_prctl_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_seccomp_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_getrandom_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint8_t* buf = (uint8_t*)(uintptr_t)args[0];
    uint32_t count = args[1];
    /* Simple pseudo-random number generator */
    static uint32_t seed = 0x12345678;
    for (uint32_t i = 0; i < count && buf; i++) {
        seed = seed * 1103515245 + 12345;
        buf[i] = (uint8_t)(seed >> 16);
    }
    return (int32_t)count;
}

static int32_t sys_memfd_create_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    int32_t fd = alloc_fd();
    if (fd >= 0) {
        g_fds[fd].type = 0;
        g_fds[fd].size = 0;
    }
    return fd;
}

/* File operations */
static int32_t sys_read_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    uint8_t* buf = (uint8_t*)(uintptr_t)args[1];
    uint32_t count = args[2];
    
    if (fd < 0 || fd >= MAX_FDS) return -9;  /* EBADF */
    if (!buf) return -14;  /* EFAULT */
    
    /* Handle stdin */
    if (fd == 0) {
        return 0;  /* EOF for now */
    }
    
    /* Handle pipe read */
    if (fd < MAX_FDS && g_fds[fd].type == 1) {
        return 0;  /* No data available */
    }
    
    if (!g_fds[fd].open) return -9;  /* EBADF */
    
    /* Simulate read */
    uint32_t avail = g_fds[fd].size - g_fds[fd].offset;
    uint32_t to_read = (count < avail) ? count : avail;
    g_fds[fd].offset += to_read;
    return (int32_t)to_read;
}

static int32_t sys_write_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    const uint8_t* buf = (const uint8_t*)(uintptr_t)args[1];
    uint32_t count = args[2];
    
    if (fd < 0 || fd >= MAX_FDS) return -9;  /* EBADF */
    if (!buf && count > 0) return -14;  /* EFAULT */
    
    /* Handle stdout/stderr */
    if (fd == 1 || fd == 2) {
        return (int32_t)count;  /* Silently consume */
    }
    
    if (!g_fds[fd].open) return -9;  /* EBADF */
    
    g_fds[fd].offset += count;
    if (g_fds[fd].offset > g_fds[fd].size) {
        g_fds[fd].size = g_fds[fd].offset;
    }
    return (int32_t)count;
}

static int32_t sys_open_impl(void* vm, uint32_t* args) {
    (void)vm;
    const char* pathname = (const char*)(uintptr_t)args[0];
    uint32_t flags = args[1];
    
    if (!pathname) return -14;  /* EFAULT */
    
    int32_t fd = alloc_fd();
    if (fd < 0) return fd;
    
    g_fds[fd].flags = flags;
    g_fds[fd].type = 0;
    return fd;
}

static int32_t sys_close_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    
    if (fd < 0 || fd >= MAX_FDS) return -9;  /* EBADF */
    if (fd <= 2) return 0;  /* Don't close stdin/stdout/stderr */
    if (!g_fds[fd].open) return -9;  /* EBADF */
    
    free_fd(fd);
    return 0;
}

static int32_t sys_lseek_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    int32_t offset = (int32_t)args[1];
    int32_t whence = (int32_t)args[2];
    
    if (fd < 0 || fd >= MAX_FDS || !g_fds[fd].open) return -9;  /* EBADF */
    
    switch (whence) {
        case 0:  /* SEEK_SET */
            if (offset < 0) return -22;  /* EINVAL - negative offset for SEEK_SET */
            g_fds[fd].offset = (uint32_t)offset;
            break;
        case 1:  /* SEEK_CUR */
            {
                int64_t new_offset = (int64_t)g_fds[fd].offset + offset;
                if (new_offset < 0) return -22;  /* EINVAL - would result in negative position */
                g_fds[fd].offset = (uint32_t)new_offset;
            }
            break;
        case 2:  /* SEEK_END */
            {
                int64_t new_offset = (int64_t)g_fds[fd].size + offset;
                if (new_offset < 0) return -22;  /* EINVAL - would result in negative position */
                g_fds[fd].offset = (uint32_t)new_offset;
            }
            break;
        default:
            return -22;  /* EINVAL */
    }
    return (int32_t)g_fds[fd].offset;
}

static int32_t sys_dup_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t oldfd = (int32_t)args[0];
    
    if (oldfd < 0 || oldfd >= MAX_FDS) return -9;  /* EBADF */
    if (oldfd > 2 && !g_fds[oldfd].open) return -9;  /* EBADF */
    
    int32_t newfd = alloc_fd();
    if (newfd < 0) return newfd;
    
    if (oldfd <= 2) {
        g_fds[newfd].type = (uint8_t)oldfd;  /* Reference stdin/stdout/stderr */
    } else {
        g_fds[newfd] = g_fds[oldfd];
    }
    return newfd;
}

static int32_t sys_dup2_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t oldfd = (int32_t)args[0];
    int32_t newfd = (int32_t)args[1];
    
    if (oldfd < 0 || oldfd >= MAX_FDS) return -9;  /* EBADF */
    if (newfd < 0 || newfd >= MAX_FDS) return -9;  /* EBADF */
    if (oldfd > 2 && !g_fds[oldfd].open) return -9;  /* EBADF */
    if (oldfd == newfd) return newfd;
    
    if (g_fds[newfd].open) {
        free_fd(newfd);
    }
    
    g_fds[newfd] = g_fds[oldfd];
    g_fds[newfd].open = true;
    return newfd;
}

static int32_t sys_dup3_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t oldfd = (int32_t)args[0];
    int32_t newfd = (int32_t)args[1];
    uint32_t flags = args[2];
    
    if (oldfd == newfd) return -22;  /* EINVAL */
    
    int32_t result = sys_dup2_impl(vm, args);
    if (result >= 0 && (flags & 0x80000)) {  /* O_CLOEXEC */
        g_fds[result].flags |= 0x80000;
    }
    return result;
}

static int32_t sys_pipe_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t* pipefd = (int32_t*)(uintptr_t)args[0];
    
    if (!pipefd) return -14;  /* EFAULT */
    
    /* Find free pipe */
    int pipe_idx = -1;
    for (int i = 0; i < MAX_PIPES; i++) {
        if (!g_pipes[i].used) {
            pipe_idx = i;
            break;
        }
    }
    if (pipe_idx < 0) return -24;  /* EMFILE */
    
    int32_t read_fd = alloc_fd();
    if (read_fd < 0) return read_fd;
    
    int32_t write_fd = alloc_fd();
    if (write_fd < 0) {
        free_fd(read_fd);
        return write_fd;
    }
    
    g_pipes[pipe_idx].used = true;
    g_pipes[pipe_idx].read_fd = read_fd;
    g_pipes[pipe_idx].write_fd = write_fd;
    g_pipes[pipe_idx].read_pos = 0;
    g_pipes[pipe_idx].write_pos = 0;
    
    g_fds[read_fd].type = 1;   /* pipe read */
    g_fds[write_fd].type = 2;  /* pipe write */
    
    pipefd[0] = read_fd;
    pipefd[1] = write_fd;
    return 0;
}

static int32_t sys_pipe2_impl(void* vm, uint32_t* args) {
    int32_t result = sys_pipe_impl(vm, args);
    if (result == 0 && (args[1] & 0x80000)) {  /* O_CLOEXEC */
        int32_t* pipefd = (int32_t*)(uintptr_t)args[0];
        if (pipefd) {
            g_fds[pipefd[0]].flags |= 0x80000;
            g_fds[pipefd[1]].flags |= 0x80000;
        }
    }
    return result;
}

static int32_t sys_stat_impl(void* vm, uint32_t* args) {
    (void)vm;
    const char* pathname = (const char*)(uintptr_t)args[0];
    void* statbuf = (void*)(uintptr_t)args[1];
    
    if (!pathname || !statbuf) return -14;  /* EFAULT */
    
    /* Fill with reasonable defaults */
    platform_memset(statbuf, 0, 144);  /* sizeof(struct stat) on x86_64 */
    return 0;
}

static int32_t sys_fstat_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    void* statbuf = (void*)(uintptr_t)args[1];
    
    if (fd < 0 || fd >= MAX_FDS) return -9;  /* EBADF */
    if (!statbuf) return -14;  /* EFAULT */
    if (fd > 2 && !g_fds[fd].open) return -9;  /* EBADF */
    
    platform_memset(statbuf, 0, 144);
    return 0;
}

static int32_t sys_lstat_impl(void* vm, uint32_t* args) {
    return sys_stat_impl(vm, args);
}

static int32_t sys_access_impl(void* vm, uint32_t* args) {
    (void)vm;
    const char* pathname = (const char*)(uintptr_t)args[0];
    if (!pathname) return -14;  /* EFAULT */
    return 0;  /* Always succeed */
}

static int32_t sys_faccessat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* Always succeed */
}

/* Memory operations */
static int32_t sys_brk_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint64_t addr = args[0];
    if (addr == 0) {
        return (int32_t)g_brk_addr;
    }
    if (addr >= 0x10000000 && addr <= 0x40000000) {
        g_brk_addr = addr;
    }
    return (int32_t)g_brk_addr;
}

static int32_t sys_mmap_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint64_t addr = args[0];
    uint64_t length = args[1];
    uint32_t prot = args[2];
    uint32_t flags = args[3];
    
    if (length == 0) return -22;  /* EINVAL */
    
    /* Find free mmap slot */
    int idx = -1;
    for (int i = 0; i < MAX_MMAPS; i++) {
        if (!g_mmaps[i].used) {
            idx = i;
            break;
        }
    }
    if (idx < 0) return -12;  /* ENOMEM */
    
    /* Allocate address if not specified */
    if (addr == 0) {
        addr = g_mmap_base;
        g_mmap_base += (length + 4095) & ~4095UL;
    }
    
    g_mmaps[idx].used = true;
    g_mmaps[idx].addr = addr;
    g_mmaps[idx].size = length;
    g_mmaps[idx].prot = prot;
    g_mmaps[idx].flags = flags;
    
    return (int32_t)addr;
}

static int32_t sys_munmap_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint64_t addr = args[0];
    
    for (int i = 0; i < MAX_MMAPS; i++) {
        if (g_mmaps[i].used && g_mmaps[i].addr == addr) {
            g_mmaps[i].used = false;
            return 0;
        }
    }
    return 0;  /* Ignore unmapping unknown regions */
}

static int32_t sys_mprotect_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint64_t addr = args[0];
    uint32_t prot = args[2];
    
    for (int i = 0; i < MAX_MMAPS; i++) {
        if (g_mmaps[i].used && g_mmaps[i].addr == addr) {
            g_mmaps[i].prot = prot;
            return 0;
        }
    }
    return 0;
}

static int32_t sys_mremap_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint64_t old_addr = args[0];
    uint64_t new_size = args[2];
    
    for (int i = 0; i < MAX_MMAPS; i++) {
        if (g_mmaps[i].used && g_mmaps[i].addr == old_addr) {
            g_mmaps[i].size = new_size;
            return (int32_t)old_addr;
        }
    }
    return -14;  /* EFAULT */
}

static int32_t sys_madvise_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* Ignore advice */
}

static int32_t sys_mlock_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* Pretend to lock */
}

static int32_t sys_munlock_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_mlockall_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_munlockall_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

/* Socket operations */
static int32_t sys_socket_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint16_t family = (uint16_t)args[0];
    uint16_t type = (uint16_t)args[1];
    uint16_t protocol = (uint16_t)args[2];
    
    /* Find free socket */
    int sock_idx = -1;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (!g_sockets[i].used) {
            sock_idx = i;
            break;
        }
    }
    if (sock_idx < 0) return -24;  /* EMFILE */
    
    int32_t fd = alloc_fd();
    if (fd < 0) return fd;
    
    g_sockets[sock_idx].used = true;
    g_sockets[sock_idx].fd = fd;
    g_sockets[sock_idx].family = family;
    g_sockets[sock_idx].type = type;
    g_sockets[sock_idx].protocol = protocol;
    g_sockets[sock_idx].connected = false;
    g_sockets[sock_idx].listening = false;
    
    g_fds[fd].type = 3;  /* socket */
    
    return fd;
}

static int32_t sys_bind_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    return 0;
}

static int32_t sys_listen_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (g_sockets[i].used && g_sockets[i].fd == sockfd) {
            g_sockets[i].listening = true;
            return 0;
        }
    }
    return -9;
}

static int32_t sys_accept_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    /* Would normally block - return EAGAIN in non-blocking mode */
    return -11;  /* EAGAIN */
}

static int32_t sys_accept4_impl(void* vm, uint32_t* args) {
    return sys_accept_impl(vm, args);
}

static int32_t sys_connect_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    for (int i = 0; i < MAX_SOCKETS; i++) {
        if (g_sockets[i].used && g_sockets[i].fd == sockfd) {
            g_sockets[i].connected = true;
            return 0;
        }
    }
    return -9;
}

static int32_t sys_getsockname_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    return 0;
}

static int32_t sys_getpeername_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    return -107;  /* ENOTCONN - not connected */
}

static int32_t sys_setsockopt_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_getsockopt_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_socketpair_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t* sv = (int32_t*)(uintptr_t)args[3];
    
    if (!sv) return -14;  /* EFAULT */
    
    /* Create two connected sockets */
    uint32_t temp_args[3] = {args[0], args[1], args[2]};
    int32_t fd1 = sys_socket_impl(vm, temp_args);
    if (fd1 < 0) return fd1;
    
    int32_t fd2 = sys_socket_impl(vm, temp_args);
    if (fd2 < 0) {
        free_fd(fd1);
        return fd2;
    }
    
    sv[0] = fd1;
    sv[1] = fd2;
    return 0;
}

static int32_t sys_sendto_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    uint32_t len = args[2];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    return (int32_t)len;  /* Pretend to send */
}

static int32_t sys_recvfrom_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    return -11;  /* EAGAIN - no data available */
}

static int32_t sys_shutdown_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t sockfd = (int32_t)args[0];
    
    if (sockfd < 0 || sockfd >= MAX_FDS || !g_fds[sockfd].open) return -9;  /* EBADF */
    if (g_fds[sockfd].type != 3) return -88;  /* ENOTSOCK */
    
    return 0;
}

/* Process operations */
static int32_t sys_fork_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -38;  /* ENOSYS - not implemented in single-process VM */
}

static int32_t sys_vfork_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -38;  /* ENOSYS */
}

static int32_t sys_clone_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -38;  /* ENOSYS */
}

static int32_t sys_execve_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -38;  /* ENOSYS */
}

static int32_t sys_wait4_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -10;  /* ECHILD - no child processes */
}

static int32_t sys_waitid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -10;  /* ECHILD */
}

/* Scheduling */
static int32_t sys_getpriority_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* Normal priority */
}

static int32_t sys_setpriority_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_sched_setscheduler_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_sched_getscheduler_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* SCHED_OTHER */
}

static int32_t sys_sched_setparam_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_sched_getparam_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_sched_get_priority_max_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 99;
}

static int32_t sys_sched_get_priority_min_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 1;
}

static int32_t sys_sched_setaffinity_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_sched_getaffinity_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* mask = (void*)(uintptr_t)args[2];
    uint32_t len = args[1];
    if (mask && len >= 8) {
        platform_memset(mask, 0, len);
        *((uint64_t*)mask) = 1;  /* CPU 0 */
    }
    return 0;
}

/* Resource limits */
static int32_t sys_getrlimit_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t rlim_cur;
        uint64_t rlim_max;
    } *rlim = (void*)(uintptr_t)args[1];
    if (rlim) {
        rlim->rlim_cur = 0xFFFFFFFFFFFFFFFFULL;  /* RLIM_INFINITY */
        rlim->rlim_max = 0xFFFFFFFFFFFFFFFFULL;
    }
    return 0;
}

static int32_t sys_setrlimit_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_prlimit64_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t rlim_cur;
        uint64_t rlim_max;
    } *old_rlim = (void*)(uintptr_t)args[3];
    if (old_rlim) {
        old_rlim->rlim_cur = 0xFFFFFFFFFFFFFFFFULL;
        old_rlim->rlim_max = 0xFFFFFFFFFFFFFFFFULL;
    }
    return 0;
}

static int32_t sys_getrusage_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* usage = (void*)(uintptr_t)args[1];
    if (usage) {
        platform_memset(usage, 0, 144);
    }
    return 0;
}

/* System info */
static int32_t sys_sysinfo_impl(void* vm, uint32_t* args) {
    (void)vm;
    struct {
        uint64_t uptime;
        uint64_t loads[3];
        uint64_t totalram;
        uint64_t freeram;
        uint64_t sharedram;
        uint64_t bufferram;
        uint64_t totalswap;
        uint64_t freeswap;
        uint16_t procs;
        uint16_t pad;
        uint64_t totalhigh;
        uint64_t freehigh;
        uint32_t mem_unit;
    } *info = (void*)(uintptr_t)args[0];
    
    if (info) {
        platform_memset(info, 0, sizeof(*info));
        info->uptime = g_timer_ticks / 1000000;
        info->totalram = 128 * 1024 * 1024;
        info->freeram = 64 * 1024 * 1024;
        info->procs = 1;
        info->mem_unit = 1;
    }
    return 0;
}

static int32_t sys_getcpu_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t* cpu = (uint32_t*)(uintptr_t)args[0];
    uint32_t* node = (uint32_t*)(uintptr_t)args[1];
    if (cpu) *cpu = 0;
    if (node) *node = 0;
    return 0;
}

/* File system operations */
static int32_t sys_truncate_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_ftruncate_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    uint32_t length = args[1];
    
    if (fd < 0 || fd >= MAX_FDS || !g_fds[fd].open) return -9;  /* EBADF */
    g_fds[fd].size = length;
    return 0;
}

static int32_t sys_fsync_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    if (fd < 0 || fd >= MAX_FDS) return -9;  /* EBADF */
    if (fd > 2 && !g_fds[fd].open) return -9;
    return 0;
}

static int32_t sys_fdatasync_impl(void* vm, uint32_t* args) {
    return sys_fsync_impl(vm, args);
}

static int32_t sys_sync_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_syncfs_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_mkdir_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_rmdir_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_rename_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_link_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_unlink_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_symlink_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_readlink_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -22;  /* EINVAL - not a symlink */
}

static int32_t sys_chmod_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_fchmod_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_chown_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_fchown_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_lchown_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

/* I/O control */
static int32_t sys_ioctl_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    uint32_t request = args[1];
    
    if (fd < 0 || fd >= MAX_FDS) return -9;  /* EBADF */
    if (fd > 2 && !g_fds[fd].open) return -9;
    
    /* Handle TIOCGWINSZ - get terminal window size */
    if (request == 0x5413) {
        struct {
            uint16_t ws_row;
            uint16_t ws_col;
            uint16_t ws_xpixel;
            uint16_t ws_ypixel;
        } *ws = (void*)(uintptr_t)args[2];
        if (ws) {
            ws->ws_row = 24;
            ws->ws_col = 80;
            ws->ws_xpixel = 640;
            ws->ws_ypixel = 480;
        }
        return 0;
    }
    
    /* Handle FIONREAD - bytes available for reading */
    if (request == 0x541B) {
        int32_t* bytes = (int32_t*)(uintptr_t)args[2];
        if (bytes) *bytes = 0;
        return 0;
    }
    
    return 0;  /* Success for unknown ioctls */
}

static int32_t sys_fcntl_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t fd = (int32_t)args[0];
    int32_t cmd = (int32_t)args[1];
    
    if (fd < 0 || fd >= MAX_FDS) return -9;  /* EBADF */
    if (fd > 2 && !g_fds[fd].open) return -9;
    
    switch (cmd) {
        case 0:  /* F_DUPFD */
            return sys_dup_impl(vm, args);
        case 1:  /* F_GETFD */
            return (g_fds[fd].flags & 0x80000) ? 1 : 0;
        case 2:  /* F_SETFD */
            if (args[2] & 1) g_fds[fd].flags |= 0x80000;
            else g_fds[fd].flags &= ~0x80000;
            return 0;
        case 3:  /* F_GETFL */
            return (int32_t)g_fds[fd].flags;
        case 4:  /* F_SETFL */
            g_fds[fd].flags = args[2];
            return 0;
        default:
            return 0;
    }
}

static int32_t sys_flock_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* Pretend to lock */
}

/* Poll/Select */
static int32_t sys_poll_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* No events */
}

static int32_t sys_select_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* No events */
}

static int32_t sys_pselect6_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_ppoll_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

/* Epoll */
static int32_t sys_epoll_create_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return alloc_fd();
}

static int32_t sys_epoll_create1_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return alloc_fd();
}

static int32_t sys_epoll_ctl_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_epoll_wait_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* No events */
}

static int32_t sys_epoll_pwait_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

/* Futex */
static int32_t sys_futex_impl(void* vm, uint32_t* args) {
    (void)vm;
    int32_t op = (int32_t)args[1] & 0x7F;
    
    switch (op) {
        case 0:  /* FUTEX_WAIT */
            return -11;  /* EAGAIN - would block */
        case 1:  /* FUTEX_WAKE */
            return 0;  /* No threads woken */
        default:
            return 0;
    }
}

/* eventfd/timerfd/signalfd */
static int32_t sys_eventfd_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return alloc_fd();
}

static int32_t sys_eventfd2_impl(void* vm, uint32_t* args) {
    return sys_eventfd_impl(vm, args);
}

static int32_t sys_timerfd_create_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return alloc_fd();
}

static int32_t sys_timerfd_settime_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_timerfd_gettime_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_signalfd_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return alloc_fd();
}

static int32_t sys_signalfd4_impl(void* vm, uint32_t* args) {
    return sys_signalfd_impl(vm, args);
}

/* inotify */
static int32_t sys_inotify_init_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return alloc_fd();
}

static int32_t sys_inotify_init1_impl(void* vm, uint32_t* args) {
    return sys_inotify_init_impl(vm, args);
}

static int32_t sys_inotify_add_watch_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 1;  /* Watch descriptor */
}

static int32_t sys_inotify_rm_watch_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

/* Directory operations */
static int32_t sys_getdents_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* Empty directory */
}

static int32_t sys_getdents64_impl(void* vm, uint32_t* args) {
    return sys_getdents_impl(vm, args);
}

/* Misc */
static int32_t sys_pause_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -4;  /* EINTR */
}

static int32_t sys_alarm_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;  /* No previous alarm */
}

static int32_t sys_getitimer_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* value = (void*)(uintptr_t)args[1];
    if (value) platform_memset(value, 0, 32);
    return 0;
}

static int32_t sys_setitimer_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_times_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* buf = (void*)(uintptr_t)args[0];
    if (buf) platform_memset(buf, 0, 32);
    return (int32_t)(g_timer_ticks / 10000);  /* ticks */
}

static int32_t sys_ptrace_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -1;  /* EPERM */
}

static int32_t sys_personality_impl(void* vm, uint32_t* args) {
    (void)vm;
    if (args[0] == 0xFFFFFFFF) {
        return 0;  /* Current personality */
    }
    return 0;
}

static int32_t sys_statfs_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* buf = (void*)(uintptr_t)args[1];
    if (buf) {
        platform_memset(buf, 0, 120);
        /* Set some reasonable values */
        ((uint64_t*)buf)[0] = 0x137D;  /* f_type - EXT2_SUPER_MAGIC */
        ((uint64_t*)buf)[1] = 4096;     /* f_bsize */
        ((uint64_t*)buf)[2] = 1000000;  /* f_blocks */
        ((uint64_t*)buf)[3] = 500000;   /* f_bfree */
        ((uint64_t*)buf)[4] = 500000;   /* f_bavail */
    }
    return 0;
}

static int32_t sys_fstatfs_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* buf = (void*)(uintptr_t)args[1];
    if (buf) {
        platform_memset(buf, 0, 120);
        ((uint64_t*)buf)[0] = 0x137D;
        ((uint64_t*)buf)[1] = 4096;
        ((uint64_t*)buf)[2] = 1000000;
        ((uint64_t*)buf)[3] = 500000;
        ((uint64_t*)buf)[4] = 500000;
    }
    return 0;
}

/* AT-style syscalls */
static int32_t sys_openat_impl(void* vm, uint32_t* args) {
    (void)vm;
    /* int32_t dirfd = (int32_t)args[0]; */
    const char* pathname = (const char*)(uintptr_t)args[1];
    uint32_t flags = args[2];
    
    if (!pathname) return -14;  /* EFAULT */
    
    int32_t fd = alloc_fd();
    if (fd >= 0) {
        g_fds[fd].flags = flags;
        g_fds[fd].type = 0;
    }
    return fd;
}

static int32_t sys_mkdirat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_unlinkat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_renameat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_renameat2_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_linkat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_symlinkat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_readlinkat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -22;  /* EINVAL */
}

static int32_t sys_fchmodat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_fchownat_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_newfstatat_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* statbuf = (void*)(uintptr_t)args[2];
    if (statbuf) platform_memset(statbuf, 0, 144);
    return 0;
}

/* Group IDs */
static int32_t sys_setreuid_impl(void* vm, uint32_t* args) {
    (void)vm;
    if (args[1] != (uint32_t)-1) g_current_uid = args[1];
    return 0;
}

static int32_t sys_setregid_impl(void* vm, uint32_t* args) {
    (void)vm;
    if (args[1] != (uint32_t)-1) g_current_gid = args[1];
    return 0;
}

static int32_t sys_setresuid_impl(void* vm, uint32_t* args) {
    (void)vm;
    if (args[2] != (uint32_t)-1) g_current_uid = args[2];
    return 0;
}

static int32_t sys_getresuid_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t* ruid = (uint32_t*)(uintptr_t)args[0];
    uint32_t* euid = (uint32_t*)(uintptr_t)args[1];
    uint32_t* suid = (uint32_t*)(uintptr_t)args[2];
    if (ruid) *ruid = g_current_uid;
    if (euid) *euid = g_current_uid;
    if (suid) *suid = g_current_uid;
    return 0;
}

static int32_t sys_setresgid_impl(void* vm, uint32_t* args) {
    (void)vm;
    if (args[2] != (uint32_t)-1) g_current_gid = args[2];
    return 0;
}

static int32_t sys_getresgid_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t* rgid = (uint32_t*)(uintptr_t)args[0];
    uint32_t* egid = (uint32_t*)(uintptr_t)args[1];
    uint32_t* sgid = (uint32_t*)(uintptr_t)args[2];
    if (rgid) *rgid = g_current_gid;
    if (egid) *egid = g_current_gid;
    if (sgid) *sgid = g_current_gid;
    return 0;
}

static int32_t sys_setfsuid_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t old = g_current_uid;
    g_current_uid = args[0];
    return (int32_t)old;
}

static int32_t sys_setfsgid_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t old = g_current_gid;
    g_current_gid = args[0];
    return (int32_t)old;
}

static int32_t sys_getgroups_impl(void* vm, uint32_t* args) {
    (void)vm;
    uint32_t size = args[0];
    uint32_t* list = (uint32_t*)(uintptr_t)args[1];
    if (size > 0 && list) {
        list[0] = g_current_gid;
        return 1;
    }
    return 1;  /* Number of groups */
}

static int32_t sys_setgroups_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_setpgid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_getpgrp_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_pid;
}

static int32_t sys_getpgid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_pid;
}

static int32_t sys_setsid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_pid;
}

static int32_t sys_getsid_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_pid;
}

/* Thread area */
static int32_t sys_set_tid_address_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return (int32_t)g_current_tid;
}

static int32_t sys_set_thread_area_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_get_thread_area_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

/* Capabilities */
static int32_t sys_capget_impl(void* vm, uint32_t* args) {
    (void)vm;
    void* datap = (void*)(uintptr_t)args[1];
    if (datap) platform_memset(datap, 0, 24);
    return 0;
}

static int32_t sys_capset_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

/* Robust futex list */
static int32_t sys_set_robust_list_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_get_robust_list_impl(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return 0;
}

static int32_t sys_not_implemented(void* vm, uint32_t* args) {
    (void)vm; (void)args;
    return -38;
}

/* ============================================================================
 * SYSCALL TABLE
 * ============================================================================ */

static syscall_handler_t g_syscall_table[SYS_COUNT] = {0};
static bool g_syscall_table_initialized = false;

void syscall_table_init(void) {
    if (g_syscall_table_initialized) return;
    
    for (int i = 0; i < SYS_COUNT; i++) {
        g_syscall_table[i] = sys_not_implemented;
    }
    
    /* File operations */
    g_syscall_table[SYS_READ] = sys_read_impl;
    g_syscall_table[SYS_WRITE] = sys_write_impl;
    g_syscall_table[SYS_OPEN] = sys_open_impl;
    g_syscall_table[SYS_CLOSE] = sys_close_impl;
    g_syscall_table[SYS_STAT] = sys_stat_impl;
    g_syscall_table[SYS_FSTAT] = sys_fstat_impl;
    g_syscall_table[SYS_LSTAT] = sys_lstat_impl;
    g_syscall_table[SYS_POLL] = sys_poll_impl;
    g_syscall_table[SYS_LSEEK] = sys_lseek_impl;
    g_syscall_table[SYS_MMAP] = sys_mmap_impl;
    g_syscall_table[SYS_MPROTECT] = sys_mprotect_impl;
    g_syscall_table[SYS_MUNMAP] = sys_munmap_impl;
    g_syscall_table[SYS_BRK] = sys_brk_impl;
    g_syscall_table[SYS_RT_SIGACTION] = sys_rt_sigaction_impl;
    g_syscall_table[SYS_RT_SIGPROCMASK] = sys_rt_sigprocmask_impl;
    g_syscall_table[SYS_IOCTL] = sys_ioctl_impl;
    g_syscall_table[SYS_ACCESS] = sys_access_impl;
    g_syscall_table[SYS_PIPE] = sys_pipe_impl;
    g_syscall_table[SYS_SELECT] = sys_select_impl;
    g_syscall_table[SYS_SCHED_YIELD] = sys_sched_yield_impl;
    g_syscall_table[SYS_MREMAP] = sys_mremap_impl;
    g_syscall_table[SYS_MADVISE] = sys_madvise_impl;
    g_syscall_table[SYS_DUP] = sys_dup_impl;
    g_syscall_table[SYS_DUP2] = sys_dup2_impl;
    g_syscall_table[SYS_PAUSE] = sys_pause_impl;
    g_syscall_table[SYS_NANOSLEEP] = sys_nanosleep_impl;
    g_syscall_table[SYS_GETITIMER] = sys_getitimer_impl;
    g_syscall_table[SYS_ALARM] = sys_alarm_impl;
    g_syscall_table[SYS_SETITIMER] = sys_setitimer_impl;
    g_syscall_table[SYS_GETPID] = sys_getpid_impl;
    
    /* Socket operations */
    g_syscall_table[SYS_SOCKET] = sys_socket_impl;
    g_syscall_table[SYS_CONNECT] = sys_connect_impl;
    g_syscall_table[SYS_ACCEPT] = sys_accept_impl;
    g_syscall_table[SYS_SENDTO] = sys_sendto_impl;
    g_syscall_table[SYS_RECVFROM] = sys_recvfrom_impl;
    g_syscall_table[SYS_SHUTDOWN] = sys_shutdown_impl;
    g_syscall_table[SYS_BIND] = sys_bind_impl;
    g_syscall_table[SYS_LISTEN] = sys_listen_impl;
    g_syscall_table[SYS_GETSOCKNAME] = sys_getsockname_impl;
    g_syscall_table[SYS_GETPEERNAME] = sys_getpeername_impl;
    g_syscall_table[SYS_SOCKETPAIR] = sys_socketpair_impl;
    g_syscall_table[SYS_SETSOCKOPT] = sys_setsockopt_impl;
    g_syscall_table[SYS_GETSOCKOPT] = sys_getsockopt_impl;
    
    /* Process operations */
    g_syscall_table[SYS_CLONE] = sys_clone_impl;
    g_syscall_table[SYS_FORK] = sys_fork_impl;
    g_syscall_table[SYS_VFORK] = sys_vfork_impl;
    g_syscall_table[SYS_EXECVE] = sys_execve_impl;
    g_syscall_table[SYS_EXIT] = sys_exit_impl;
    g_syscall_table[SYS_WAIT4] = sys_wait4_impl;
    g_syscall_table[SYS_KILL] = sys_kill_impl;
    g_syscall_table[SYS_UNAME] = sys_uname_impl;
    
    /* File control */
    g_syscall_table[SYS_FCNTL] = sys_fcntl_impl;
    g_syscall_table[SYS_FLOCK] = sys_flock_impl;
    g_syscall_table[SYS_FSYNC] = sys_fsync_impl;
    g_syscall_table[SYS_FDATASYNC] = sys_fdatasync_impl;
    g_syscall_table[SYS_TRUNCATE] = sys_truncate_impl;
    g_syscall_table[SYS_FTRUNCATE] = sys_ftruncate_impl;
    g_syscall_table[SYS_GETDENTS] = sys_getdents_impl;
    g_syscall_table[SYS_GETCWD] = sys_getcwd_impl;
    g_syscall_table[SYS_CHDIR] = sys_chdir_impl;
    g_syscall_table[SYS_FCHDIR] = sys_fchdir_impl;
    g_syscall_table[SYS_RENAME] = sys_rename_impl;
    g_syscall_table[SYS_MKDIR] = sys_mkdir_impl;
    g_syscall_table[SYS_RMDIR] = sys_rmdir_impl;
    g_syscall_table[SYS_LINK] = sys_link_impl;
    g_syscall_table[SYS_UNLINK] = sys_unlink_impl;
    g_syscall_table[SYS_SYMLINK] = sys_symlink_impl;
    g_syscall_table[SYS_READLINK] = sys_readlink_impl;
    g_syscall_table[SYS_CHMOD] = sys_chmod_impl;
    g_syscall_table[SYS_FCHMOD] = sys_fchmod_impl;
    g_syscall_table[SYS_CHOWN] = sys_chown_impl;
    g_syscall_table[SYS_FCHOWN] = sys_fchown_impl;
    g_syscall_table[SYS_LCHOWN] = sys_lchown_impl;
    g_syscall_table[SYS_UMASK] = sys_umask_impl;
    g_syscall_table[SYS_GETTIMEOFDAY] = sys_gettimeofday_impl;
    g_syscall_table[SYS_GETRLIMIT] = sys_getrlimit_impl;
    g_syscall_table[SYS_GETRUSAGE] = sys_getrusage_impl;
    g_syscall_table[SYS_SYSINFO] = sys_sysinfo_impl;
    g_syscall_table[SYS_TIMES] = sys_times_impl;
    g_syscall_table[SYS_PTRACE] = sys_ptrace_impl;
    
    /* User/Group IDs */
    g_syscall_table[SYS_GETUID] = sys_getuid_impl;
    g_syscall_table[SYS_GETGID] = sys_getgid_impl;
    g_syscall_table[SYS_SETUID] = sys_setuid_impl;
    g_syscall_table[SYS_SETGID] = sys_setgid_impl;
    g_syscall_table[SYS_GETEUID] = sys_geteuid_impl;
    g_syscall_table[SYS_GETEGID] = sys_getegid_impl;
    g_syscall_table[SYS_SETPGID] = sys_setpgid_impl;
    g_syscall_table[SYS_GETPPID] = sys_getppid_impl;
    g_syscall_table[SYS_GETPGRP] = sys_getpgrp_impl;
    g_syscall_table[SYS_SETSID] = sys_setsid_impl;
    g_syscall_table[SYS_SETREUID] = sys_setreuid_impl;
    g_syscall_table[SYS_SETREGID] = sys_setregid_impl;
    g_syscall_table[SYS_GETGROUPS] = sys_getgroups_impl;
    g_syscall_table[SYS_SETGROUPS] = sys_setgroups_impl;
    g_syscall_table[SYS_SETRESUID] = sys_setresuid_impl;
    g_syscall_table[SYS_GETRESUID] = sys_getresuid_impl;
    g_syscall_table[SYS_SETRESGID] = sys_setresgid_impl;
    g_syscall_table[SYS_GETRESGID] = sys_getresgid_impl;
    g_syscall_table[SYS_GETPGID] = sys_getpgid_impl;
    g_syscall_table[SYS_SETFSUID] = sys_setfsuid_impl;
    g_syscall_table[SYS_SETFSGID] = sys_setfsgid_impl;
    g_syscall_table[SYS_GETSID] = sys_getsid_impl;
    g_syscall_table[SYS_CAPGET] = sys_capget_impl;
    g_syscall_table[SYS_CAPSET] = sys_capset_impl;
    g_syscall_table[SYS_RT_SIGPENDING] = sys_rt_sigpending_impl;
    g_syscall_table[SYS_SIGALTSTACK] = sys_sigaltstack_impl;
    
    /* System */
    g_syscall_table[SYS_PERSONALITY] = sys_personality_impl;
    g_syscall_table[SYS_STATFS] = sys_statfs_impl;
    g_syscall_table[SYS_FSTATFS] = sys_fstatfs_impl;
    g_syscall_table[SYS_GETPRIORITY] = sys_getpriority_impl;
    g_syscall_table[SYS_SETPRIORITY] = sys_setpriority_impl;
    g_syscall_table[SYS_SCHED_SETPARAM] = sys_sched_setparam_impl;
    g_syscall_table[SYS_SCHED_GETPARAM] = sys_sched_getparam_impl;
    g_syscall_table[SYS_SCHED_SETSCHEDULER] = sys_sched_setscheduler_impl;
    g_syscall_table[SYS_SCHED_GETSCHEDULER] = sys_sched_getscheduler_impl;
    g_syscall_table[SYS_SCHED_GET_PRIORITY_MAX] = sys_sched_get_priority_max_impl;
    g_syscall_table[SYS_SCHED_GET_PRIORITY_MIN] = sys_sched_get_priority_min_impl;
    g_syscall_table[SYS_MLOCK] = sys_mlock_impl;
    g_syscall_table[SYS_MUNLOCK] = sys_munlock_impl;
    g_syscall_table[SYS_MLOCKALL] = sys_mlockall_impl;
    g_syscall_table[SYS_MUNLOCKALL] = sys_munlockall_impl;
    g_syscall_table[SYS_PRCTL] = sys_prctl_impl;
    g_syscall_table[SYS_ARCH_PRCTL] = sys_arch_prctl_impl;
    g_syscall_table[SYS_SETRLIMIT] = sys_setrlimit_impl;
    g_syscall_table[SYS_SYNC] = sys_sync_impl;
    g_syscall_table[SYS_SETTIMEOFDAY] = sys_settimeofday_impl;
    g_syscall_table[SYS_SETHOSTNAME] = sys_sethostname_impl;
    g_syscall_table[SYS_SETDOMAINNAME] = sys_setdomainname_impl;
    g_syscall_table[SYS_GETTID] = sys_gettid_impl;
    
    /* More file/dir operations */
    g_syscall_table[SYS_FUTEX] = sys_futex_impl;
    g_syscall_table[SYS_SCHED_SETAFFINITY] = sys_sched_setaffinity_impl;
    g_syscall_table[SYS_SCHED_GETAFFINITY] = sys_sched_getaffinity_impl;
    g_syscall_table[SYS_SET_THREAD_AREA] = sys_set_thread_area_impl;
    g_syscall_table[SYS_GET_THREAD_AREA] = sys_get_thread_area_impl;
    g_syscall_table[SYS_EPOLL_CREATE] = sys_epoll_create_impl;
    g_syscall_table[SYS_GETDENTS64] = sys_getdents64_impl;
    g_syscall_table[SYS_SET_TID_ADDRESS] = sys_set_tid_address_impl;
    g_syscall_table[SYS_CLOCK_SETTIME] = sys_clock_settime_impl;
    g_syscall_table[SYS_CLOCK_GETTIME] = sys_clock_gettime_impl;
    g_syscall_table[SYS_CLOCK_GETRES] = sys_clock_getres_impl;
    g_syscall_table[SYS_EXIT_GROUP] = sys_exit_impl;
    g_syscall_table[SYS_EPOLL_WAIT] = sys_epoll_wait_impl;
    g_syscall_table[SYS_EPOLL_CTL] = sys_epoll_ctl_impl;
    g_syscall_table[SYS_TGKILL] = sys_tgkill_impl;
    g_syscall_table[SYS_WAITID] = sys_waitid_impl;
    
    /* AT-style syscalls */
    g_syscall_table[SYS_OPENAT] = sys_openat_impl;
    g_syscall_table[SYS_MKDIRAT] = sys_mkdirat_impl;
    g_syscall_table[SYS_NEWFSTATAT] = sys_newfstatat_impl;
    g_syscall_table[SYS_UNLINKAT] = sys_unlinkat_impl;
    g_syscall_table[SYS_RENAMEAT] = sys_renameat_impl;
    g_syscall_table[SYS_LINKAT] = sys_linkat_impl;
    g_syscall_table[SYS_SYMLINKAT] = sys_symlinkat_impl;
    g_syscall_table[SYS_READLINKAT] = sys_readlinkat_impl;
    g_syscall_table[SYS_FCHMODAT] = sys_fchmodat_impl;
    g_syscall_table[SYS_FACCESSAT] = sys_faccessat_impl;
    g_syscall_table[SYS_PSELECT6] = sys_pselect6_impl;
    g_syscall_table[SYS_PPOLL] = sys_ppoll_impl;
    g_syscall_table[SYS_SET_ROBUST_LIST] = sys_set_robust_list_impl;
    g_syscall_table[SYS_GET_ROBUST_LIST] = sys_get_robust_list_impl;
    g_syscall_table[SYS_EPOLL_PWAIT] = sys_epoll_pwait_impl;
    g_syscall_table[SYS_SIGNALFD] = sys_signalfd_impl;
    g_syscall_table[SYS_TIMERFD_CREATE] = sys_timerfd_create_impl;
    g_syscall_table[SYS_EVENTFD] = sys_eventfd_impl;
    g_syscall_table[SYS_TIMERFD_SETTIME] = sys_timerfd_settime_impl;
    g_syscall_table[SYS_TIMERFD_GETTIME] = sys_timerfd_gettime_impl;
    g_syscall_table[SYS_ACCEPT4] = sys_accept4_impl;
    g_syscall_table[SYS_SIGNALFD4] = sys_signalfd4_impl;
    g_syscall_table[SYS_EVENTFD2] = sys_eventfd2_impl;
    g_syscall_table[SYS_EPOLL_CREATE1] = sys_epoll_create1_impl;
    g_syscall_table[SYS_DUP3] = sys_dup3_impl;
    g_syscall_table[SYS_PIPE2] = sys_pipe2_impl;
    g_syscall_table[SYS_INOTIFY_INIT1] = sys_inotify_init1_impl;
    g_syscall_table[SYS_PRLIMIT64] = sys_prlimit64_impl;
    g_syscall_table[SYS_SYNCFS] = sys_syncfs_impl;
    g_syscall_table[SYS_GETCPU] = sys_getcpu_impl;
    g_syscall_table[SYS_RENAMEAT2] = sys_renameat2_impl;
    g_syscall_table[SYS_SECCOMP] = sys_seccomp_impl;
    g_syscall_table[SYS_GETRANDOM] = sys_getrandom_impl;
    g_syscall_table[SYS_MEMFD_CREATE] = sys_memfd_create_impl;
    g_syscall_table[SYS_FCHOWNAT] = sys_fchownat_impl;
    g_syscall_table[SYS_INOTIFY_INIT] = sys_inotify_init_impl;
    g_syscall_table[SYS_INOTIFY_ADD_WATCH] = sys_inotify_add_watch_impl;
    g_syscall_table[SYS_INOTIFY_RM_WATCH] = sys_inotify_rm_watch_impl;
    g_syscall_table[SYS_TKILL] = sys_tkill_impl;
    
    g_syscall_table_initialized = true;
}

int32_t syscall_dispatch(void* vm, uint32_t syscall_num, uint32_t* args) {
    if (!g_syscall_table_initialized) syscall_table_init();
    if (syscall_num >= SYS_COUNT) return -38;
    return g_syscall_table[syscall_num](vm, args);
}

uint32_t syscall_get_count(void) {
    return SYS_COUNT;
}

bool syscall_is_implemented(uint32_t syscall_num) {
    if (!g_syscall_table_initialized) syscall_table_init();
    if (syscall_num >= SYS_COUNT) return false;
    return g_syscall_table[syscall_num] != sys_not_implemented;
}

const char* syscall_table_get_version(void) {
    return "1.0.0-aurora-syscall";
}
