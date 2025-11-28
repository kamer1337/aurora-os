/**
 * Aurora OS - Android Syscall Table
 * 
 * Expanded syscall table for Android compatibility (200+ syscalls)
 * Based on ARM64/x86_64 Linux syscall numbers
 */

#ifndef AURORA_ANDROID_SYSCALL_H
#define AURORA_ANDROID_SYSCALL_H

#include <stdint.h>
#include <stddef.h>

/* Android syscall numbers (ARM64 compatible) */
#define __NR_ANDROID_io_setup           0
#define __NR_ANDROID_io_destroy         1
#define __NR_ANDROID_io_submit          2
#define __NR_ANDROID_io_cancel          3
#define __NR_ANDROID_io_getevents       4
#define __NR_ANDROID_setxattr           5
#define __NR_ANDROID_lsetxattr          6
#define __NR_ANDROID_fsetxattr          7
#define __NR_ANDROID_getxattr           8
#define __NR_ANDROID_lgetxattr          9
#define __NR_ANDROID_fgetxattr          10
#define __NR_ANDROID_listxattr          11
#define __NR_ANDROID_llistxattr         12
#define __NR_ANDROID_flistxattr         13
#define __NR_ANDROID_removexattr        14
#define __NR_ANDROID_lremovexattr       15
#define __NR_ANDROID_fremovexattr       16
#define __NR_ANDROID_getcwd             17
#define __NR_ANDROID_lookup_dcookie     18
#define __NR_ANDROID_eventfd2           19
#define __NR_ANDROID_epoll_create1      20
#define __NR_ANDROID_epoll_ctl          21
#define __NR_ANDROID_epoll_pwait        22
#define __NR_ANDROID_dup                23
#define __NR_ANDROID_dup3               24
#define __NR_ANDROID_fcntl              25
#define __NR_ANDROID_inotify_init1      26
#define __NR_ANDROID_inotify_add_watch  27
#define __NR_ANDROID_inotify_rm_watch   28
#define __NR_ANDROID_ioctl              29
#define __NR_ANDROID_ioprio_set         30
#define __NR_ANDROID_ioprio_get         31
#define __NR_ANDROID_flock              32
#define __NR_ANDROID_mknodat            33
#define __NR_ANDROID_mkdirat            34
#define __NR_ANDROID_unlinkat           35
#define __NR_ANDROID_symlinkat          36
#define __NR_ANDROID_linkat             37
#define __NR_ANDROID_renameat           38
#define __NR_ANDROID_umount2            39
#define __NR_ANDROID_mount              40
#define __NR_ANDROID_pivot_root         41
#define __NR_ANDROID_nfsservctl         42
#define __NR_ANDROID_statfs             43
#define __NR_ANDROID_fstatfs            44
#define __NR_ANDROID_truncate           45
#define __NR_ANDROID_ftruncate          46
#define __NR_ANDROID_fallocate          47
#define __NR_ANDROID_faccessat          48
#define __NR_ANDROID_chdir              49
#define __NR_ANDROID_fchdir             50
#define __NR_ANDROID_chroot             51
#define __NR_ANDROID_fchmod             52
#define __NR_ANDROID_fchmodat           53
#define __NR_ANDROID_fchownat           54
#define __NR_ANDROID_fchown             55
#define __NR_ANDROID_openat             56
#define __NR_ANDROID_close              57
#define __NR_ANDROID_vhangup            58
#define __NR_ANDROID_pipe2              59
#define __NR_ANDROID_quotactl           60
#define __NR_ANDROID_getdents64         61
#define __NR_ANDROID_lseek              62
#define __NR_ANDROID_read               63
#define __NR_ANDROID_write              64
#define __NR_ANDROID_readv              65
#define __NR_ANDROID_writev             66
#define __NR_ANDROID_pread64            67
#define __NR_ANDROID_pwrite64           68
#define __NR_ANDROID_preadv             69
#define __NR_ANDROID_pwritev            70
#define __NR_ANDROID_sendfile           71
#define __NR_ANDROID_pselect6           72
#define __NR_ANDROID_ppoll              73
#define __NR_ANDROID_signalfd4          74
#define __NR_ANDROID_vmsplice           75
#define __NR_ANDROID_splice             76
#define __NR_ANDROID_tee                77
#define __NR_ANDROID_readlinkat         78
#define __NR_ANDROID_newfstatat         79
#define __NR_ANDROID_fstat              80
#define __NR_ANDROID_sync               81
#define __NR_ANDROID_fsync              82
#define __NR_ANDROID_fdatasync          83
#define __NR_ANDROID_sync_file_range    84
#define __NR_ANDROID_timerfd_create     85
#define __NR_ANDROID_timerfd_settime    86
#define __NR_ANDROID_timerfd_gettime    87
#define __NR_ANDROID_utimensat          88
#define __NR_ANDROID_acct               89
#define __NR_ANDROID_capget             90
#define __NR_ANDROID_capset             91
#define __NR_ANDROID_personality        92
#define __NR_ANDROID_exit               93
#define __NR_ANDROID_exit_group         94
#define __NR_ANDROID_waitid             95
#define __NR_ANDROID_set_tid_address    96
#define __NR_ANDROID_unshare            97
#define __NR_ANDROID_futex              98
#define __NR_ANDROID_set_robust_list    99
#define __NR_ANDROID_get_robust_list    100
#define __NR_ANDROID_nanosleep          101
#define __NR_ANDROID_getitimer          102
#define __NR_ANDROID_setitimer          103
#define __NR_ANDROID_kexec_load         104
#define __NR_ANDROID_init_module        105
#define __NR_ANDROID_delete_module      106
#define __NR_ANDROID_timer_create       107
#define __NR_ANDROID_timer_gettime      108
#define __NR_ANDROID_timer_getoverrun   109
#define __NR_ANDROID_timer_settime      110
#define __NR_ANDROID_timer_delete       111
#define __NR_ANDROID_clock_settime      112
#define __NR_ANDROID_clock_gettime      113
#define __NR_ANDROID_clock_getres       114
#define __NR_ANDROID_clock_nanosleep    115
#define __NR_ANDROID_syslog             116
#define __NR_ANDROID_ptrace             117
#define __NR_ANDROID_sched_setparam     118
#define __NR_ANDROID_sched_setscheduler 119
#define __NR_ANDROID_sched_getscheduler 120
#define __NR_ANDROID_sched_getparam     121
#define __NR_ANDROID_sched_setaffinity  122
#define __NR_ANDROID_sched_getaffinity  123
#define __NR_ANDROID_sched_yield        124
#define __NR_ANDROID_sched_get_priority_max 125
#define __NR_ANDROID_sched_get_priority_min 126
#define __NR_ANDROID_sched_rr_get_interval  127
#define __NR_ANDROID_restart_syscall    128
#define __NR_ANDROID_kill               129
#define __NR_ANDROID_tkill              130
#define __NR_ANDROID_tgkill             131
#define __NR_ANDROID_sigaltstack        132
#define __NR_ANDROID_rt_sigsuspend      133
#define __NR_ANDROID_rt_sigaction       134
#define __NR_ANDROID_rt_sigprocmask     135
#define __NR_ANDROID_rt_sigpending      136
#define __NR_ANDROID_rt_sigtimedwait    137
#define __NR_ANDROID_rt_sigqueueinfo    138
#define __NR_ANDROID_rt_sigreturn       139
#define __NR_ANDROID_setpriority        140
#define __NR_ANDROID_getpriority        141
#define __NR_ANDROID_reboot             142
#define __NR_ANDROID_setregid           143
#define __NR_ANDROID_setgid             144
#define __NR_ANDROID_setreuid           145
#define __NR_ANDROID_setuid             146
#define __NR_ANDROID_setresuid          147
#define __NR_ANDROID_getresuid          148
#define __NR_ANDROID_setresgid          149
#define __NR_ANDROID_getresgid          150
#define __NR_ANDROID_setfsuid           151
#define __NR_ANDROID_setfsgid           152
#define __NR_ANDROID_times              153
#define __NR_ANDROID_setpgid            154
#define __NR_ANDROID_getpgid            155
#define __NR_ANDROID_getsid             156
#define __NR_ANDROID_setsid             157
#define __NR_ANDROID_getgroups          158
#define __NR_ANDROID_setgroups          159
#define __NR_ANDROID_uname              160
#define __NR_ANDROID_sethostname        161
#define __NR_ANDROID_setdomainname      162
#define __NR_ANDROID_getrlimit          163
#define __NR_ANDROID_setrlimit          164
#define __NR_ANDROID_getrusage          165
#define __NR_ANDROID_umask              166
#define __NR_ANDROID_prctl              167
#define __NR_ANDROID_getcpu             168
#define __NR_ANDROID_gettimeofday       169
#define __NR_ANDROID_settimeofday       170
#define __NR_ANDROID_adjtimex           171
#define __NR_ANDROID_getpid             172
#define __NR_ANDROID_getppid            173
#define __NR_ANDROID_getuid             174
#define __NR_ANDROID_geteuid            175
#define __NR_ANDROID_getgid             176
#define __NR_ANDROID_getegid            177
#define __NR_ANDROID_gettid             178
#define __NR_ANDROID_sysinfo            179
#define __NR_ANDROID_mq_open            180
#define __NR_ANDROID_mq_unlink          181
#define __NR_ANDROID_mq_timedsend       182
#define __NR_ANDROID_mq_timedreceive    183
#define __NR_ANDROID_mq_notify          184
#define __NR_ANDROID_mq_getsetattr      185
#define __NR_ANDROID_msgget             186
#define __NR_ANDROID_msgctl             187
#define __NR_ANDROID_msgrcv             188
#define __NR_ANDROID_msgsnd             189
#define __NR_ANDROID_semget             190
#define __NR_ANDROID_semctl             191
#define __NR_ANDROID_semtimedop         192
#define __NR_ANDROID_semop              193
#define __NR_ANDROID_shmget             194
#define __NR_ANDROID_shmctl             195
#define __NR_ANDROID_shmat              196
#define __NR_ANDROID_shmdt              197
#define __NR_ANDROID_socket             198
#define __NR_ANDROID_socketpair         199
#define __NR_ANDROID_bind               200
#define __NR_ANDROID_listen             201
#define __NR_ANDROID_accept             202
#define __NR_ANDROID_connect            203
#define __NR_ANDROID_getsockname        204
#define __NR_ANDROID_getpeername        205
#define __NR_ANDROID_sendto             206
#define __NR_ANDROID_recvfrom           207
#define __NR_ANDROID_setsockopt         208
#define __NR_ANDROID_getsockopt         209
#define __NR_ANDROID_shutdown           210
#define __NR_ANDROID_sendmsg            211
#define __NR_ANDROID_recvmsg            212
#define __NR_ANDROID_readahead          213
#define __NR_ANDROID_brk                214
#define __NR_ANDROID_munmap             215
#define __NR_ANDROID_mremap             216
#define __NR_ANDROID_add_key            217
#define __NR_ANDROID_request_key        218
#define __NR_ANDROID_keyctl             219
#define __NR_ANDROID_clone              220
#define __NR_ANDROID_execve             221
#define __NR_ANDROID_mmap               222
#define __NR_ANDROID_fadvise64          223
#define __NR_ANDROID_swapon             224
#define __NR_ANDROID_swapoff            225
#define __NR_ANDROID_mprotect           226
#define __NR_ANDROID_msync              227
#define __NR_ANDROID_mlock              228
#define __NR_ANDROID_munlock            229
#define __NR_ANDROID_mlockall           230
#define __NR_ANDROID_munlockall         231
#define __NR_ANDROID_mincore            232
#define __NR_ANDROID_madvise            233
#define __NR_ANDROID_remap_file_pages   234
#define __NR_ANDROID_mbind              235
#define __NR_ANDROID_get_mempolicy      236
#define __NR_ANDROID_set_mempolicy      237
#define __NR_ANDROID_migrate_pages      238
#define __NR_ANDROID_move_pages         239
#define __NR_ANDROID_rt_tgsigqueueinfo  240
#define __NR_ANDROID_perf_event_open    241
#define __NR_ANDROID_accept4            242
#define __NR_ANDROID_recvmmsg           243
#define __NR_ANDROID_wait4              260
#define __NR_ANDROID_prlimit64          261
#define __NR_ANDROID_fanotify_init      262
#define __NR_ANDROID_fanotify_mark      263
#define __NR_ANDROID_name_to_handle_at  264
#define __NR_ANDROID_open_by_handle_at  265
#define __NR_ANDROID_clock_adjtime      266
#define __NR_ANDROID_syncfs             267
#define __NR_ANDROID_setns              268
#define __NR_ANDROID_sendmmsg           269
#define __NR_ANDROID_process_vm_readv   270
#define __NR_ANDROID_process_vm_writev  271
#define __NR_ANDROID_kcmp               272
#define __NR_ANDROID_finit_module       273
#define __NR_ANDROID_sched_setattr      274
#define __NR_ANDROID_sched_getattr      275
#define __NR_ANDROID_renameat2          276
#define __NR_ANDROID_seccomp            277
#define __NR_ANDROID_getrandom          278
#define __NR_ANDROID_memfd_create       279
#define __NR_ANDROID_bpf                280
#define __NR_ANDROID_execveat           281
#define __NR_ANDROID_userfaultfd        282
#define __NR_ANDROID_membarrier         283
#define __NR_ANDROID_mlock2             284
#define __NR_ANDROID_copy_file_range    285
#define __NR_ANDROID_preadv2            286
#define __NR_ANDROID_pwritev2           287
#define __NR_ANDROID_pkey_mprotect      288
#define __NR_ANDROID_pkey_alloc         289
#define __NR_ANDROID_pkey_free          290
#define __NR_ANDROID_statx              291
#define __NR_ANDROID_io_pgetevents      292
#define __NR_ANDROID_rseq               293

/* Binder-specific syscalls */
#define __NR_ANDROID_BINDER_WRITE_READ    0x40086201
#define __NR_ANDROID_BINDER_SET_MAX_THREADS 0x40046205
#define __NR_ANDROID_BINDER_SET_CONTEXT_MGR 0x40046207
#define __NR_ANDROID_BINDER_THREAD_EXIT   0x40046208
#define __NR_ANDROID_BINDER_VERSION       0xc0046209

/* Android-specific syscalls */
#define __NR_ANDROID_ashmem_create_region  300
#define __NR_ANDROID_ashmem_set_prot_mask  301
#define __NR_ANDROID_ashmem_get_size       302
#define __NR_ANDROID_ashmem_pin            303
#define __NR_ANDROID_ashmem_unpin          304

/* Max syscall number */
#define __NR_ANDROID_MAX 350

/* Error codes */
#define ENOSYS      38
#define EBADF       9
#define EFAULT      14
#define EINVAL      22
#define ENOMEM      12
#define EPERM       1
#define EACCES      13
#define ENOENT      2
#define EEXIST      17
#define ENOTDIR     20
#define EISDIR      21
#define EMFILE      24
#define ENFILE      23
#define ENOTEMPTY   39
#define ELOOP       40
#define ENAMETOOLONG 36
#define ENOSPC      28
#define EIO         5
#define EAGAIN      11
#define EWOULDBLOCK EAGAIN
#define EINTR       4
#define EBUSY       16
#define ETIMEDOUT   110
#define ENODEV      19
#define ENOTTY      25
#define ECHILD      10
#define ESRCH       3

/* Process-related structures */
typedef struct {
    long tv_sec;
    long tv_nsec;
} android_timespec_t;

typedef struct {
    long tv_sec;
    long tv_usec;
} android_timeval_t;

typedef struct {
    unsigned long rlim_cur;
    unsigned long rlim_max;
} android_rlimit_t;

typedef struct {
    long ru_utime_sec;
    long ru_utime_usec;
    long ru_stime_sec;
    long ru_stime_usec;
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
    uint16_t procs;
    uint16_t pad;
    unsigned long totalhigh;
    unsigned long freehigh;
    uint32_t mem_unit;
} android_sysinfo_t;

/* Socket-related structures */
typedef struct {
    uint16_t sa_family;
    char sa_data[14];
} android_sockaddr_t;

typedef struct {
    uint32_t s_addr;
} android_in_addr_t;

typedef struct {
    uint16_t sin_family;
    uint16_t sin_port;
    android_in_addr_t sin_addr;
    uint8_t sin_zero[8];
} android_sockaddr_in_t;

/* Signal structures */
typedef struct {
    unsigned long sig[2];
} android_sigset_t;

typedef struct {
    void* ss_sp;
    int ss_flags;
    size_t ss_size;
} android_stack_t;

/* Poll structures */
typedef struct {
    int fd;
    short events;
    short revents;
} android_pollfd_t;

/* Epoll structures */
typedef union {
    void* ptr;
    int fd;
    uint32_t u32;
    uint64_t u64;
} android_epoll_data_t;

typedef struct {
    uint32_t events;
    android_epoll_data_t data;
} android_epoll_event_t;

/* Stat structure */
typedef struct {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    uint64_t __pad1;
    int64_t st_size;
    int32_t st_blksize;
    int32_t __pad2;
    int64_t st_blocks;
    android_timespec_t st_atim;
    android_timespec_t st_mtim;
    android_timespec_t st_ctim;
    uint32_t __unused[2];
} android_stat64_t;

/* Directory entry */
typedef struct {
    uint64_t d_ino;
    int64_t d_off;
    uint16_t d_reclen;
    uint8_t d_type;
    char d_name[256];
} android_dirent64_t;

/* Syscall handler function type */
typedef long (*android_syscall_handler_t)(long arg0, long arg1, long arg2, 
                                          long arg3, long arg4, long arg5);

/* Syscall table entry */
typedef struct {
    int syscall_num;
    const char* name;
    android_syscall_handler_t handler;
    int num_args;
} android_syscall_entry_t;

/**
 * Initialize Android syscall subsystem
 */
int android_syscall_init(void);

/**
 * Handle Android syscall
 * @param syscall_num Syscall number
 * @param arg0-arg5 Syscall arguments
 * @return Syscall return value
 */
long android_syscall(long syscall_num, long arg0, long arg1, long arg2,
                     long arg3, long arg4, long arg5);

/**
 * Get syscall name by number
 */
const char* android_syscall_get_name(int syscall_num);

/**
 * Print syscall statistics
 */
void android_syscall_print_stats(void);

/* Individual syscall declarations */
long android_sys_read(long fd, long buf, long count, long unused1, long unused2, long unused3);
long android_sys_write(long fd, long buf, long count, long unused1, long unused2, long unused3);
long android_sys_openat(long dirfd, long pathname, long flags, long mode, long unused1, long unused2);
long android_sys_close(long fd, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_fstat(long fd, long statbuf, long unused1, long unused2, long unused3, long unused4);
long android_sys_newfstatat(long dirfd, long pathname, long statbuf, long flags, long unused1, long unused2);
long android_sys_lseek(long fd, long offset, long whence, long unused1, long unused2, long unused3);
long android_sys_mmap(long addr, long length, long prot, long flags, long fd, long offset);
long android_sys_mprotect(long addr, long len, long prot, long unused1, long unused2, long unused3);
long android_sys_munmap(long addr, long length, long unused1, long unused2, long unused3, long unused4);
long android_sys_brk(long addr, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_ioctl(long fd, long request, long arg, long unused1, long unused2, long unused3);
long android_sys_readv(long fd, long iov, long iovcnt, long unused1, long unused2, long unused3);
long android_sys_writev(long fd, long iov, long iovcnt, long unused1, long unused2, long unused3);
long android_sys_pipe2(long pipefd, long flags, long unused1, long unused2, long unused3, long unused4);
long android_sys_dup(long oldfd, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_dup3(long oldfd, long newfd, long flags, long unused1, long unused2, long unused3);
long android_sys_fcntl(long fd, long cmd, long arg, long unused1, long unused2, long unused3);
long android_sys_mkdirat(long dirfd, long pathname, long mode, long unused1, long unused2, long unused3);
long android_sys_unlinkat(long dirfd, long pathname, long flags, long unused1, long unused2, long unused3);
long android_sys_renameat(long olddirfd, long oldpath, long newdirfd, long newpath, long unused1, long unused2);
long android_sys_faccessat(long dirfd, long pathname, long mode, long flags, long unused1, long unused2);
long android_sys_fchdir(long fd, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_chdir(long pathname, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_getcwd(long buf, long size, long unused1, long unused2, long unused3, long unused4);
long android_sys_fchmod(long fd, long mode, long unused1, long unused2, long unused3, long unused4);
long android_sys_fchmodat(long dirfd, long pathname, long mode, long flags, long unused1, long unused2);
long android_sys_fchown(long fd, long owner, long group, long unused1, long unused2, long unused3);
long android_sys_fchownat(long dirfd, long pathname, long owner, long group, long flags, long unused1);
long android_sys_getdents64(long fd, long dirp, long count, long unused1, long unused2, long unused3);
long android_sys_socket(long domain, long type, long protocol, long unused1, long unused2, long unused3);
long android_sys_socketpair(long domain, long type, long protocol, long sv, long unused1, long unused2);
long android_sys_bind(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3);
long android_sys_listen(long sockfd, long backlog, long unused1, long unused2, long unused3, long unused4);
long android_sys_accept(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3);
long android_sys_accept4(long sockfd, long addr, long addrlen, long flags, long unused1, long unused2);
long android_sys_connect(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3);
long android_sys_getsockname(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3);
long android_sys_getpeername(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3);
long android_sys_sendto(long sockfd, long buf, long len, long flags, long dest_addr, long addrlen);
long android_sys_recvfrom(long sockfd, long buf, long len, long flags, long src_addr, long addrlen);
long android_sys_setsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused);
long android_sys_getsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused);
long android_sys_shutdown(long sockfd, long how, long unused1, long unused2, long unused3, long unused4);
long android_sys_clone(long flags, long stack, long parent_tid, long tls, long child_tid, long unused);
long android_sys_execve(long pathname, long argv, long envp, long unused1, long unused2, long unused3);
long android_sys_exit(long status, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_exit_group(long status, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_wait4(long pid, long wstatus, long options, long rusage, long unused1, long unused2);
long android_sys_kill(long pid, long sig, long unused1, long unused2, long unused3, long unused4);
long android_sys_tkill(long tid, long sig, long unused1, long unused2, long unused3, long unused4);
long android_sys_tgkill(long tgid, long tid, long sig, long unused1, long unused2, long unused3);
long android_sys_rt_sigaction(long signum, long act, long oldact, long sigsetsize, long unused1, long unused2);
long android_sys_rt_sigprocmask(long how, long set, long oldset, long sigsetsize, long unused1, long unused2);
long android_sys_rt_sigreturn(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_getpid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_getppid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_getuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_geteuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_getgid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_getegid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_gettid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_setuid(long uid, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_setgid(long gid, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_setpgid(long pid, long pgid, long unused1, long unused2, long unused3, long unused4);
long android_sys_getpgid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_setsid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_getsid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_uname(long buf, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_sysinfo(long info, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_getrlimit(long resource, long rlim, long unused1, long unused2, long unused3, long unused4);
long android_sys_setrlimit(long resource, long rlim, long unused1, long unused2, long unused3, long unused4);
long android_sys_prlimit64(long pid, long resource, long new_rlim, long old_rlim, long unused1, long unused2);
long android_sys_getrusage(long who, long usage, long unused1, long unused2, long unused3, long unused4);
long android_sys_umask(long mask, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_prctl(long option, long arg2, long arg3, long arg4, long arg5, long unused);
long android_sys_getcpu(long cpu, long node, long tcache, long unused1, long unused2, long unused3);
long android_sys_gettimeofday(long tv, long tz, long unused1, long unused2, long unused3, long unused4);
long android_sys_settimeofday(long tv, long tz, long unused1, long unused2, long unused3, long unused4);
long android_sys_clock_gettime(long clk_id, long tp, long unused1, long unused2, long unused3, long unused4);
long android_sys_clock_settime(long clk_id, long tp, long unused1, long unused2, long unused3, long unused4);
long android_sys_clock_getres(long clk_id, long res, long unused1, long unused2, long unused3, long unused4);
long android_sys_clock_nanosleep(long clk_id, long flags, long request, long remain, long unused1, long unused2);
long android_sys_nanosleep(long req, long rem, long unused1, long unused2, long unused3, long unused4);
long android_sys_futex(long uaddr, long futex_op, long val, long timeout, long uaddr2, long val3);
long android_sys_set_tid_address(long tidptr, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_set_robust_list(long head, long len, long unused1, long unused2, long unused3, long unused4);
long android_sys_get_robust_list(long pid, long head_ptr, long len_ptr, long unused1, long unused2, long unused3);
long android_sys_epoll_create1(long flags, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_epoll_ctl(long epfd, long op, long fd, long event, long unused1, long unused2);
long android_sys_epoll_pwait(long epfd, long events, long maxevents, long timeout, long sigmask, long sigsetsize);
long android_sys_ppoll(long fds, long nfds, long timeout_ts, long sigmask, long sigsetsize, long unused);
long android_sys_eventfd2(long initval, long flags, long unused1, long unused2, long unused3, long unused4);
long android_sys_signalfd4(long fd, long mask, long sizemask, long flags, long unused1, long unused2);
long android_sys_timerfd_create(long clockid, long flags, long unused1, long unused2, long unused3, long unused4);
long android_sys_timerfd_settime(long fd, long flags, long new_value, long old_value, long unused1, long unused2);
long android_sys_timerfd_gettime(long fd, long curr_value, long unused1, long unused2, long unused3, long unused4);
long android_sys_inotify_init1(long flags, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_inotify_add_watch(long fd, long pathname, long mask, long unused1, long unused2, long unused3);
long android_sys_inotify_rm_watch(long fd, long wd, long unused1, long unused2, long unused3, long unused4);
long android_sys_mount(long source, long target, long fstype, long flags, long data, long unused);
long android_sys_umount2(long target, long flags, long unused1, long unused2, long unused3, long unused4);
long android_sys_sync(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_fsync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_fdatasync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_truncate(long path, long length, long unused1, long unused2, long unused3, long unused4);
long android_sys_ftruncate(long fd, long length, long unused1, long unused2, long unused3, long unused4);
long android_sys_statfs(long path, long buf, long unused1, long unused2, long unused3, long unused4);
long android_sys_fstatfs(long fd, long buf, long unused1, long unused2, long unused3, long unused4);
long android_sys_readlinkat(long dirfd, long pathname, long buf, long bufsiz, long unused1, long unused2);
long android_sys_symlinkat(long target, long newdirfd, long linkpath, long unused1, long unused2, long unused3);
long android_sys_linkat(long olddirfd, long oldpath, long newdirfd, long newpath, long flags, long unused);
long android_sys_madvise(long addr, long length, long advice, long unused1, long unused2, long unused3);
long android_sys_mincore(long addr, long length, long vec, long unused1, long unused2, long unused3);
long android_sys_mlock(long addr, long len, long unused1, long unused2, long unused3, long unused4);
long android_sys_munlock(long addr, long len, long unused1, long unused2, long unused3, long unused4);
long android_sys_mlockall(long flags, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_munlockall(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_mremap(long old_addr, long old_size, long new_size, long flags, long new_addr, long unused);
long android_sys_msync(long addr, long length, long flags, long unused1, long unused2, long unused3);
long android_sys_sched_yield(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6);
long android_sys_sched_getaffinity(long pid, long cpusetsize, long mask, long unused1, long unused2, long unused3);
long android_sys_sched_setaffinity(long pid, long cpusetsize, long mask, long unused1, long unused2, long unused3);
long android_sys_sched_getscheduler(long pid, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_sched_setscheduler(long pid, long policy, long param, long unused1, long unused2, long unused3);
long android_sys_sched_getparam(long pid, long param, long unused1, long unused2, long unused3, long unused4);
long android_sys_sched_setparam(long pid, long param, long unused1, long unused2, long unused3, long unused4);
long android_sys_sched_get_priority_max(long policy, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_sched_get_priority_min(long policy, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_capget(long hdrp, long datap, long unused1, long unused2, long unused3, long unused4);
long android_sys_capset(long hdrp, long datap, long unused1, long unused2, long unused3, long unused4);
long android_sys_personality(long persona, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_setpriority(long which, long who, long prio, long unused1, long unused2, long unused3);
long android_sys_getpriority(long which, long who, long unused1, long unused2, long unused3, long unused4);
long android_sys_getrandom(long buf, long buflen, long flags, long unused1, long unused2, long unused3);
long android_sys_memfd_create(long name, long flags, long unused1, long unused2, long unused3, long unused4);
long android_sys_seccomp(long operation, long flags, long args, long unused1, long unused2, long unused3);

/* Android-specific syscalls */
long android_sys_ashmem_create_region(long name, long size, long unused1, long unused2, long unused3, long unused4);
long android_sys_ashmem_set_prot_mask(long fd, long prot, long unused1, long unused2, long unused3, long unused4);
long android_sys_ashmem_get_size(long fd, long unused1, long unused2, long unused3, long unused4, long unused5);
long android_sys_ashmem_pin(long fd, long pin, long unused1, long unused2, long unused3, long unused4);
long android_sys_ashmem_unpin(long fd, long pin, long unused1, long unused2, long unused3, long unused4);

#endif /* AURORA_ANDROID_SYSCALL_H */
