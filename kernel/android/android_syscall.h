/**
 * Aurora OS - Android Syscall Handler Header
 * 
 * Android-compatible syscall definitions and declarations
 */

#ifndef AURORA_ANDROID_SYSCALL_H
#define AURORA_ANDROID_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

/* Error codes */
#define EPERM 1
#define ENOENT 2
#define ESRCH 3
#define EINTR 4
#define EIO 5
#define ENXIO 6
#define E2BIG 7
#define ENOEXEC 8
#define EBADF 9
#define ECHILD 10
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define ENOTBLK 15
#define EBUSY 16
#define EEXIST 17
#define EXDEV 18
#define ENODEV 19
#define ENOTDIR 20
#define EISDIR 21
#define EINVAL 22
#define ENFILE 23
#define EMFILE 24
#define ENOTTY 25
#define ETXTBSY 26
#define EFBIG 27
#define ENOSPC 28
#define ESPIPE 29
#define EROFS 30
#define EMLINK 31
#define EPIPE 32
#define EDOM 33
#define ERANGE 34
#define ENOSYS 38
#define ELOOP 40
#define ENOTEMPTY 39

/* Android/ARM64 syscall numbers */
#define __NR_ANDROID_read 0
#define __NR_ANDROID_write 1
#define __NR_ANDROID_openat 56
#define __NR_ANDROID_close 57
#define __NR_ANDROID_fstat 80
#define __NR_ANDROID_newfstatat 79
#define __NR_ANDROID_lseek 62
#define __NR_ANDROID_mmap 222
#define __NR_ANDROID_mprotect 226
#define __NR_ANDROID_munmap 215
#define __NR_ANDROID_brk 214
#define __NR_ANDROID_ioctl 29
#define __NR_ANDROID_readv 65
#define __NR_ANDROID_writev 66
#define __NR_ANDROID_pipe2 59
#define __NR_ANDROID_dup 23
#define __NR_ANDROID_dup3 24
#define __NR_ANDROID_fcntl 25
#define __NR_ANDROID_mkdirat 34
#define __NR_ANDROID_unlinkat 35
#define __NR_ANDROID_renameat 38
#define __NR_ANDROID_faccessat 48
#define __NR_ANDROID_fchdir 50
#define __NR_ANDROID_chdir 49
#define __NR_ANDROID_getcwd 17
#define __NR_ANDROID_fchmod 52
#define __NR_ANDROID_fchmodat 53
#define __NR_ANDROID_fchown 55
#define __NR_ANDROID_fchownat 54
#define __NR_ANDROID_getdents64 61
#define __NR_ANDROID_socket 198
#define __NR_ANDROID_socketpair 199
#define __NR_ANDROID_bind 200
#define __NR_ANDROID_listen 201
#define __NR_ANDROID_accept 202
#define __NR_ANDROID_accept4 242
#define __NR_ANDROID_connect 203
#define __NR_ANDROID_getsockname 204
#define __NR_ANDROID_getpeername 205
#define __NR_ANDROID_sendto 206
#define __NR_ANDROID_recvfrom 207
#define __NR_ANDROID_setsockopt 208
#define __NR_ANDROID_getsockopt 209
#define __NR_ANDROID_shutdown 210
#define __NR_ANDROID_clone 220
#define __NR_ANDROID_execve 221
#define __NR_ANDROID_exit 93
#define __NR_ANDROID_exit_group 94
#define __NR_ANDROID_wait4 260
#define __NR_ANDROID_kill 129
#define __NR_ANDROID_tkill 130
#define __NR_ANDROID_tgkill 131
#define __NR_ANDROID_rt_sigaction 134
#define __NR_ANDROID_rt_sigprocmask 135
#define __NR_ANDROID_rt_sigreturn 139
#define __NR_ANDROID_getpid 172
#define __NR_ANDROID_getppid 173
#define __NR_ANDROID_getuid 174
#define __NR_ANDROID_geteuid 175
#define __NR_ANDROID_getgid 176
#define __NR_ANDROID_getegid 177
#define __NR_ANDROID_gettid 178
#define __NR_ANDROID_setuid 146
#define __NR_ANDROID_setgid 144
#define __NR_ANDROID_setpgid 154
#define __NR_ANDROID_getpgid 155
#define __NR_ANDROID_setsid 157
#define __NR_ANDROID_getsid 156
#define __NR_ANDROID_uname 160
#define __NR_ANDROID_sysinfo 179
#define __NR_ANDROID_getrlimit 163
#define __NR_ANDROID_setrlimit 164
#define __NR_ANDROID_prlimit64 261
#define __NR_ANDROID_getrusage 165
#define __NR_ANDROID_umask 166
#define __NR_ANDROID_prctl 167
#define __NR_ANDROID_getcpu 168
#define __NR_ANDROID_gettimeofday 169
#define __NR_ANDROID_settimeofday 170
#define __NR_ANDROID_clock_gettime 113
#define __NR_ANDROID_clock_settime 112
#define __NR_ANDROID_clock_getres 114
#define __NR_ANDROID_clock_nanosleep 115
#define __NR_ANDROID_nanosleep 101
#define __NR_ANDROID_futex 98
#define __NR_ANDROID_set_tid_address 96
#define __NR_ANDROID_set_robust_list 99
#define __NR_ANDROID_get_robust_list 100
#define __NR_ANDROID_epoll_create1 20
#define __NR_ANDROID_epoll_ctl 21
#define __NR_ANDROID_epoll_pwait 22
#define __NR_ANDROID_ppoll 73
#define __NR_ANDROID_eventfd2 19
#define __NR_ANDROID_signalfd4 74
#define __NR_ANDROID_timerfd_create 85
#define __NR_ANDROID_timerfd_settime 86
#define __NR_ANDROID_timerfd_gettime 87
#define __NR_ANDROID_inotify_init1 26
#define __NR_ANDROID_inotify_add_watch 27
#define __NR_ANDROID_inotify_rm_watch 28
#define __NR_ANDROID_mount 40
#define __NR_ANDROID_umount2 39
#define __NR_ANDROID_sync 81
#define __NR_ANDROID_fsync 82
#define __NR_ANDROID_fdatasync 83
#define __NR_ANDROID_truncate 45
#define __NR_ANDROID_ftruncate 46
#define __NR_ANDROID_statfs 43
#define __NR_ANDROID_fstatfs 44
#define __NR_ANDROID_readlinkat 78
#define __NR_ANDROID_symlinkat 36
#define __NR_ANDROID_linkat 37
#define __NR_ANDROID_madvise 233
#define __NR_ANDROID_mincore 232
#define __NR_ANDROID_mlock 228
#define __NR_ANDROID_munlock 229
#define __NR_ANDROID_mlockall 230
#define __NR_ANDROID_munlockall 231
#define __NR_ANDROID_mremap 216
#define __NR_ANDROID_msync 227
#define __NR_ANDROID_sched_yield 124
#define __NR_ANDROID_sched_getaffinity 123
#define __NR_ANDROID_sched_setaffinity 122
#define __NR_ANDROID_sched_getscheduler 120
#define __NR_ANDROID_sched_setscheduler 119
#define __NR_ANDROID_sched_getparam 121
#define __NR_ANDROID_sched_setparam 118
#define __NR_ANDROID_sched_get_priority_max 125
#define __NR_ANDROID_sched_get_priority_min 126
#define __NR_ANDROID_capget 90
#define __NR_ANDROID_capset 91
#define __NR_ANDROID_personality 92
#define __NR_ANDROID_setpriority 140
#define __NR_ANDROID_getpriority 141
#define __NR_ANDROID_getrandom 278
#define __NR_ANDROID_memfd_create 279
#define __NR_ANDROID_seccomp 277
#define __NR_ANDROID_ashmem_create_region 400
#define __NR_ANDROID_ashmem_set_prot_mask 401
#define __NR_ANDROID_ashmem_get_size 402
#define __NR_ANDROID_ashmem_pin 403
#define __NR_ANDROID_ashmem_unpin 404
#define __NR_ANDROID_MAX 512

/* Data structures */
typedef struct {
    int64_t tv_sec;
    int64_t tv_nsec;
} android_timespec_t;

typedef struct {
    int64_t tv_sec;
    int64_t tv_usec;
} android_timeval_t;

typedef struct {
    uint64_t st_dev;
    uint64_t st_ino;
    uint64_t st_nlink;
    uint32_t st_mode;
    uint32_t st_uid;
    uint32_t st_gid;
    uint32_t __pad0;
    uint64_t st_rdev;
    int64_t st_size;
    int64_t st_blksize;
    int64_t st_blocks;
    android_timespec_t st_atim;
    android_timespec_t st_mtim;
    android_timespec_t st_ctim;
    int64_t __unused[3];
} android_stat64_t;

typedef struct {
    uint64_t d_ino;
    int64_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];
} android_dirent64_t;

typedef struct {
    uint16_t sa_family;
    char sa_data[14];
} android_sockaddr_t;

typedef struct {
    uint32_t events;
    uint64_t data;
} android_epoll_event_t;

typedef struct {
    int fd;
    short events;
    short revents;
} android_pollfd_t;

typedef struct {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
} android_utsname_t;

typedef struct {
    long uptime;
    unsigned long loads[3];
    unsigned long totalram;
    unsigned long freeram;
    unsigned long sharedram;
    unsigned long bufferram;
    unsigned long totalswap;
    unsigned long freeswap;
    unsigned short procs;
    unsigned short pad;
    unsigned long totalhigh;
    unsigned long freehigh;
    unsigned int mem_unit;
    char _f[20-2*sizeof(long)-sizeof(int)];
} android_sysinfo_t;

typedef struct {
    uint64_t rlim_cur;
    uint64_t rlim_max;
} android_rlimit_t;

typedef struct {
    android_timeval_t ru_utime;
    android_timeval_t ru_stime;
    long ru_maxrss;
    long ru_ixrss;
    long ru_idrss;
    long ru_isrss;
    long ru_minflt;
    long ru_majflt;
    long ru_nswap;
    long ru_inblock;
    long ru_oublock;
    long ru_msgsnd;
    long ru_msgrcv;
    long ru_nsignals;
    long ru_nvcsw;
    long ru_nivcsw;
} android_rusage_t;

/* Syscall handler function type */
typedef long (*android_syscall_handler_t)(long, long, long, long, long, long);

/* Syscall table entry */
typedef struct {
    int syscall_num;
    const char* name;
    android_syscall_handler_t handler;
    int num_args;
} android_syscall_entry_t;

/* API functions */
int android_syscall_init(void);
long android_syscall(long syscall_num, long arg0, long arg1, long arg2, long arg3, long arg4, long arg5);
const char* android_syscall_get_name(int syscall_num);
void android_syscall_print_stats(void);

/* Individual syscall handlers */
long android_sys_read(long fd, long buf, long count, long unused1, long unused2, long unused3);
long android_sys_write(long fd, long buf, long count, long unused1, long unused2, long unused3);
long android_sys_openat(long dirfd, long pathname, long flags, long mode, long unused1, long unused2);
long android_sys_close(long fd, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_fstat(long fd, long statbuf, long unused1, long unused2, long unused3, long unused4);
long android_sys_getpid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_gettid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_mmap(long addr, long length, long prot, long flags, long fd, long offset);
long android_sys_munmap(long addr, long length, long unused1, long unused2, long unused3, long unused4);
long android_sys_clone(long flags, long stack, long parent_tid, long tls, long child_tid, long unused);
long android_sys_futex(long uaddr, long futex_op, long val, long timeout, long uaddr2, long val3);

#endif /* AURORA_ANDROID_SYSCALL_H */
