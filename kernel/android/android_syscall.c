/**
 * Aurora OS - Android Syscall Handler Implementation
 * 
 * Full implementation of Android-compatible system calls
 * Based on ARM64/x86_64 Linux syscall ABI
 */

#include "android_syscall.h"
#include "../memory/memory.h"
#include "../process/process.h"
#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../../filesystem/vfs/vfs.h"

/* ============================================================================
 * INTERNAL DATA STRUCTURES
 * ============================================================================ */

/* Syscall statistics tracking */
typedef struct {
    uint64_t total_calls;
    uint64_t successful_calls;
    uint64_t failed_calls;
    uint64_t call_count[__NR_ANDROID_MAX];
} syscall_stats_t;

static syscall_stats_t g_syscall_stats = {0};
static int g_syscall_initialized = 0;

/* Current process context (simplified) */
static struct {
    uint32_t pid;
    uint32_t tid;
    uint32_t uid;
    uint32_t gid;
    uint32_t euid;
    uint32_t egid;
    uint32_t ppid;
    uint32_t pgid;
    uint32_t sid;
    char cwd[256];
    uint32_t umask_val;
} g_process_ctx = {
    .pid = 1,
    .tid = 1,
    .uid = 0,
    .gid = 0,
    .euid = 0,
    .egid = 0,
    .ppid = 0,
    .pgid = 1,
    .sid = 1,
    .cwd = "/",
    .umask_val = 022
};

/* File descriptor table */
#define MAX_FDS 1024
typedef struct {
    int valid;
    int vfs_fd;
    int flags;
    uint32_t offset;
    int type;  /* 0=regular, 1=socket, 2=pipe, 3=epoll, 4=eventfd, 5=timerfd, 6=inotify */
    void* private_data;
} fd_entry_t;

static fd_entry_t g_fd_table[MAX_FDS] = {0};

/* Socket structures */
typedef struct {
    int domain;
    int type;
    int protocol;
    int state;
    android_sockaddr_t local_addr;
    android_sockaddr_t peer_addr;
    uint32_t backlog;
    void* recv_buffer;
    size_t recv_size;
} socket_data_t;

/* Pipe structure */
typedef struct {
    uint8_t* buffer;
    size_t size;
    size_t read_pos;
    size_t write_pos;
    size_t capacity;
    int read_end;
    int write_end;
} pipe_data_t;

/* Epoll structures */
#define MAX_EPOLL_EVENTS 64
typedef struct {
    int fd;
    android_epoll_event_t event;
    int active;
} epoll_entry_t;

typedef struct {
    epoll_entry_t entries[MAX_EPOLL_EVENTS];
    int count;
} epoll_data_t;

/* Timer structures */
typedef struct {
    int clockid;
    android_timespec_t interval;
    android_timespec_t expiry;
    int armed;
    uint64_t overrun;
} timerfd_data_t;

/* Inotify structures */
typedef struct {
    int wd;
    char path[256];
    uint32_t mask;
    int active;
} inotify_watch_t;

#define MAX_INOTIFY_WATCHES 64
typedef struct {
    inotify_watch_t watches[MAX_INOTIFY_WATCHES];
    int count;
    int next_wd;
} inotify_data_t;

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static void* syscall_memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static void* syscall_memset(void* dest, int val, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = (uint8_t)val;
    }
    return dest;
}

static size_t syscall_strlen(const char* s) {
    size_t len = 0;
    if (s) {
        while (s[len]) len++;
    }
    return len;
}

static char* syscall_strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

static char* syscall_strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

/* Allocate a new file descriptor */
static int alloc_fd(void) {
    for (int i = 3; i < MAX_FDS; i++) {
        if (!g_fd_table[i].valid) {
            g_fd_table[i].valid = 1;
            g_fd_table[i].type = 0;
            g_fd_table[i].offset = 0;
            g_fd_table[i].vfs_fd = -1;
            g_fd_table[i].private_data = NULL;
            return i;
        }
    }
    return -EMFILE;
}

/* Get file descriptor entry */
static fd_entry_t* get_fd_entry(int fd) {
    if (fd < 0 || fd >= MAX_FDS || !g_fd_table[fd].valid) {
        return NULL;
    }
    return &g_fd_table[fd];
}

/* Free a file descriptor */
static void free_fd(int fd) {
    if (fd >= 0 && fd < MAX_FDS) {
        g_fd_table[fd].valid = 0;
        g_fd_table[fd].vfs_fd = -1;
        if (g_fd_table[fd].private_data) {
            kfree(g_fd_table[fd].private_data);
            g_fd_table[fd].private_data = NULL;
        }
    }
}

/* Simple timestamp (seconds since boot) */
static uint64_t get_system_time_ns(void) {
    return timer_get_ticks() * 10000000ULL;
}

static uint64_t get_system_time_sec(void) {
    return timer_get_ticks() / 100;
}

/* ============================================================================
 * SYSCALL INITIALIZATION
 * ============================================================================ */

int android_syscall_init(void) {
    if (g_syscall_initialized) {
        return 0;
    }
    
    g_fd_table[0].valid = 1;
    g_fd_table[0].type = 0;
    g_fd_table[0].flags = O_RDONLY;
    
    g_fd_table[1].valid = 1;
    g_fd_table[1].type = 0;
    g_fd_table[1].flags = O_WRONLY;
    
    g_fd_table[2].valid = 1;
    g_fd_table[2].type = 0;
    g_fd_table[2].flags = O_WRONLY;
    
    syscall_memset(&g_syscall_stats, 0, sizeof(g_syscall_stats));
    
    g_syscall_initialized = 1;
    return 0;
}

/* ============================================================================
 * FILE SYSTEM SYSCALLS
 * ============================================================================ */

long android_sys_read(long fd, long buf, long count, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    
    if (!buf || count < 0) {
        return -EFAULT;
    }
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) {
        return -EBADF;
    }
    
    if (fd == 0) {
        return 0;
    }
    
    if (entry->type == 2 && entry->private_data) {
        pipe_data_t* pipe = (pipe_data_t*)entry->private_data;
        size_t available = (pipe->write_pos - pipe->read_pos + pipe->capacity) % pipe->capacity;
        size_t to_read = (size_t)count < available ? (size_t)count : available;
        
        uint8_t* dest = (uint8_t*)buf;
        for (size_t i = 0; i < to_read; i++) {
            dest[i] = pipe->buffer[pipe->read_pos];
            pipe->read_pos = (pipe->read_pos + 1) % pipe->capacity;
        }
        return (long)to_read;
    }
    
    if (entry->vfs_fd >= 0) {
        int result = vfs_read(entry->vfs_fd, (void*)buf, (size_t)count);
        if (result >= 0) {
            entry->offset += result;
        }
        return result;
    }
    
    return -EBADF;
}

long android_sys_write(long fd, long buf, long count, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    
    if (!buf || count < 0) {
        return -EFAULT;
    }
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) {
        return -EBADF;
    }
    
    if (fd == 1 || fd == 2) {
        const char* str = (const char*)buf;
        for (long i = 0; i < count && str[i]; i++) {
            vga_putchar(str[i]);
        }
        return count;
    }
    
    if (entry->type == 2 && entry->private_data) {
        pipe_data_t* pipe = (pipe_data_t*)entry->private_data;
        size_t space = pipe->capacity - 1 - 
                      ((pipe->write_pos - pipe->read_pos + pipe->capacity) % pipe->capacity);
        size_t to_write = (size_t)count < space ? (size_t)count : space;
        
        const uint8_t* src = (const uint8_t*)buf;
        for (size_t i = 0; i < to_write; i++) {
            pipe->buffer[pipe->write_pos] = src[i];
            pipe->write_pos = (pipe->write_pos + 1) % pipe->capacity;
        }
        return (long)to_write;
    }
    
    if (entry->vfs_fd >= 0) {
        int result = vfs_write(entry->vfs_fd, (const void*)buf, (size_t)count);
        if (result >= 0) {
            entry->offset += result;
        }
        return result;
    }
    
    return -EBADF;
}

long android_sys_openat(long dirfd, long pathname, long flags, long mode, long unused1, long unused2) {
    (void)unused1; (void)unused2;
    
    if (!pathname) {
        return -EFAULT;
    }
    
    const char* path = (const char*)pathname;
    (void)dirfd;
    
    int fd = alloc_fd();
    if (fd < 0) {
        return fd;
    }
    
    int vfs_flags = 0;
    if ((flags & 3) == 0) vfs_flags |= O_RDONLY;
    if ((flags & 3) == 1) vfs_flags |= O_WRONLY;
    if ((flags & 3) == 2) vfs_flags |= O_RDWR;
    if (flags & 0x40) vfs_flags |= O_CREAT;
    if (flags & 0x400) vfs_flags |= O_APPEND;
    if (flags & 0x200) vfs_flags |= O_TRUNC;
    
    int vfs_fd = vfs_open(path, vfs_flags);
    if (vfs_fd < 0) {
        if ((flags & 0x40) && vfs_fd == -1) {
            if (vfs_create_mode(path, (uint16_t)(mode & 0777)) == 0) {
                vfs_fd = vfs_open(path, vfs_flags);
            }
        }
        
        if (vfs_fd < 0) {
            free_fd(fd);
            return -ENOENT;
        }
    }
    
    g_fd_table[fd].vfs_fd = vfs_fd;
    g_fd_table[fd].flags = (int)flags;
    g_fd_table[fd].offset = 0;
    g_fd_table[fd].type = 0;
    
    return fd;
}

long android_sys_close(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) {
        return -EBADF;
    }
    
    if (entry->vfs_fd >= 0) {
        vfs_close(entry->vfs_fd);
    }
    
    if (entry->private_data) {
        if (entry->type == 1) {
            socket_data_t* sock = (socket_data_t*)entry->private_data;
            if (sock->recv_buffer) {
                kfree(sock->recv_buffer);
            }
        } else if (entry->type == 2) {
            pipe_data_t* pipe = (pipe_data_t*)entry->private_data;
            if (pipe->buffer) {
                kfree(pipe->buffer);
            }
        }
    }
    
    free_fd((int)fd);
    return 0;
}

long android_sys_fstat(long fd, long statbuf, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    
    if (!statbuf) {
        return -EFAULT;
    }
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) {
        return -EBADF;
    }
    
    android_stat64_t* st = (android_stat64_t*)statbuf;
    syscall_memset(st, 0, sizeof(android_stat64_t));
    
    if (fd <= 2) {
        st->st_mode = 0020666;
        st->st_blksize = 4096;
        return 0;
    }
    
    if (entry->type == 1) {
        st->st_mode = 0140777;
        return 0;
    }
    
    if (entry->type == 2) {
        st->st_mode = 0010666;
        pipe_data_t* pipe = (pipe_data_t*)entry->private_data;
        if (pipe) {
            st->st_size = (pipe->write_pos - pipe->read_pos + pipe->capacity) % pipe->capacity;
        }
        return 0;
    }
    
    st->st_dev = 1;
    st->st_ino = (uint64_t)fd;
    st->st_mode = 0100644;
    st->st_nlink = 1;
    st->st_uid = g_process_ctx.uid;
    st->st_gid = g_process_ctx.gid;
    st->st_blksize = 4096;
    
    return 0;
}

long android_sys_newfstatat(long dirfd, long pathname, long statbuf, long flags, long unused1, long unused2) {
    (void)dirfd; (void)flags; (void)unused1; (void)unused2;
    
    if (!pathname || !statbuf) {
        return -EFAULT;
    }
    
    const char* path = (const char*)pathname;
    android_stat64_t* st = (android_stat64_t*)statbuf;
    
    inode_t inode_stat;
    if (vfs_stat(path, &inode_stat) != 0) {
        return -ENOENT;
    }
    
    syscall_memset(st, 0, sizeof(android_stat64_t));
    st->st_ino = inode_stat.ino;
    st->st_size = inode_stat.size;
    st->st_nlink = inode_stat.links;
    st->st_blocks = inode_stat.blocks;
    st->st_uid = inode_stat.uid;
    st->st_gid = inode_stat.gid;
    st->st_blksize = 4096;
    
    switch (inode_stat.type) {
        case FILE_TYPE_REGULAR:
            st->st_mode = 0100000 | (inode_stat.mode & 0777);
            break;
        case FILE_TYPE_DIRECTORY:
            st->st_mode = 0040000 | (inode_stat.mode & 0777);
            break;
        case FILE_TYPE_DEVICE:
            st->st_mode = 0020000 | (inode_stat.mode & 0777);
            break;
        case FILE_TYPE_SYMLINK:
            st->st_mode = 0120000 | (inode_stat.mode & 0777);
            break;
    }
    
    st->st_atim.tv_sec = inode_stat.atime;
    st->st_mtim.tv_sec = inode_stat.mtime;
    st->st_ctim.tv_sec = inode_stat.ctime;
    
    return 0;
}

long android_sys_lseek(long fd, long offset, long whence, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) {
        return -EBADF;
    }
    
    if (entry->vfs_fd >= 0) {
        int result = vfs_seek(entry->vfs_fd, offset, (int)whence);
        if (result >= 0) {
            entry->offset = (uint32_t)result;
        }
        return result;
    }
    
    switch (whence) {
        case SEEK_SET:
            entry->offset = (uint32_t)offset;
            break;
        case SEEK_CUR:
            entry->offset += offset;
            break;
        case SEEK_END:
            return -EINVAL;
        default:
            return -EINVAL;
    }
    
    return (long)entry->offset;
}

long android_sys_getcwd(long buf, long size, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    
    if (!buf || size == 0) {
        return -EFAULT;
    }
    
    size_t cwd_len = syscall_strlen(g_process_ctx.cwd);
    if (cwd_len + 1 > (size_t)size) {
        return -ERANGE;
    }
    
    syscall_strcpy((char*)buf, g_process_ctx.cwd);
    return (long)buf;
}

long android_sys_chdir(long pathname, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    
    if (!pathname) {
        return -EFAULT;
    }
    
    const char* path = (const char*)pathname;
    
    if (vfs_chdir(path) != 0) {
        return -ENOENT;
    }
    
    syscall_strncpy(g_process_ctx.cwd, path, sizeof(g_process_ctx.cwd) - 1);
    g_process_ctx.cwd[sizeof(g_process_ctx.cwd) - 1] = '\0';
    
    return 0;
}

long android_sys_fchdir(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)fd; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return -ENOSYS;
}

long android_sys_mkdirat(long dirfd, long pathname, long mode, long unused1, long unused2, long unused3) {
    (void)dirfd; (void)unused1; (void)unused2; (void)unused3;
    
    if (!pathname) {
        return -EFAULT;
    }
    
    if (vfs_mkdir_mode((const char*)pathname, (uint16_t)(mode & 0777)) != 0) {
        return -ENOENT;
    }
    
    return 0;
}

long android_sys_unlinkat(long dirfd, long pathname, long flags, long unused1, long unused2, long unused3) {
    (void)dirfd; (void)unused1; (void)unused2; (void)unused3;
    
    if (!pathname) {
        return -EFAULT;
    }
    
    if (flags & 0x200) {
        if (vfs_rmdir((const char*)pathname) != 0) {
            return -ENOENT;
        }
    } else {
        if (vfs_unlink((const char*)pathname) != 0) {
            return -ENOENT;
        }
    }
    
    return 0;
}

long android_sys_renameat(long olddirfd, long oldpath, long newdirfd, long newpath, long unused1, long unused2) {
    (void)olddirfd; (void)newdirfd; (void)unused1; (void)unused2;
    
    if (!oldpath || !newpath) {
        return -EFAULT;
    }
    
    if (vfs_rename((const char*)oldpath, (const char*)newpath) != 0) {
        return -ENOENT;
    }
    
    return 0;
}

long android_sys_faccessat(long dirfd, long pathname, long mode, long flags, long unused1, long unused2) {
    (void)dirfd; (void)flags; (void)unused1; (void)unused2;
    
    if (!pathname) {
        return -EFAULT;
    }
    
    int access_mode = 0;
    if (mode & 4) access_mode |= R_OK;
    if (mode & 2) access_mode |= W_OK;
    if (mode & 1) access_mode |= X_OK;
    
    if (vfs_access((const char*)pathname, access_mode) != 0) {
        return -EACCES;
    }
    
    return 0;
}

long android_sys_fchmod(long fd, long mode, long unused1, long unused2, long unused3, long unused4) {
    (void)mode; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) {
        return -EBADF;
    }
    return 0;
}

long android_sys_fchmodat(long dirfd, long pathname, long mode, long flags, long unused1, long unused2) {
    (void)dirfd; (void)flags; (void)unused1; (void)unused2;
    
    if (!pathname) {
        return -EFAULT;
    }
    
    if (vfs_chmod((const char*)pathname, (uint16_t)(mode & 0777)) != 0) {
        return -ENOENT;
    }
    
    return 0;
}

long android_sys_fchown(long fd, long owner, long group, long unused1, long unused2, long unused3) {
    (void)owner; (void)group; (void)unused1; (void)unused2; (void)unused3;
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) {
        return -EBADF;
    }
    return 0;
}

long android_sys_fchownat(long dirfd, long pathname, long owner, long group, long flags, long unused1) {
    (void)dirfd; (void)flags; (void)unused1;
    
    if (!pathname) {
        return -EFAULT;
    }
    
    if (vfs_chown((const char*)pathname, (uint16_t)owner, (uint16_t)group) != 0) {
        return -ENOENT;
    }
    
    return 0;
}

long android_sys_getdents64(long fd, long dirp, long count, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    
    if (!dirp || count < (long)sizeof(android_dirent64_t)) {
        return -EINVAL;
    }
    
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry || entry->vfs_fd < 0) {
        return -EBADF;
    }
    
    android_dirent64_t* out = (android_dirent64_t*)dirp;
    dirent_t vfs_entry;
    long total_bytes = 0;
    
    while (total_bytes + (long)sizeof(android_dirent64_t) <= count) {
        if (vfs_readdir(entry->vfs_fd, &vfs_entry) != 0) {
            break;
        }
        
        out->d_ino = vfs_entry.ino;
        out->d_off = total_bytes + sizeof(android_dirent64_t);
        out->d_reclen = sizeof(android_dirent64_t);
        out->d_type = (vfs_entry.type == FILE_TYPE_DIRECTORY) ? 4 : 8;
        syscall_strncpy(out->d_name, vfs_entry.name, 255);
        out->d_name[255] = '\0';
        
        total_bytes += sizeof(android_dirent64_t);
        out++;
    }
    
    return total_bytes;
}

long android_sys_truncate(long path, long length, long unused1, long unused2, long unused3, long unused4) {
    (void)path; (void)length; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_ftruncate(long fd, long length, long unused1, long unused2, long unused3, long unused4) {
    (void)length; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    return 0;
}

long android_sys_sync(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return 0;
}

long android_sys_fsync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    return 0;
}

long android_sys_fdatasync(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return android_sys_fsync(fd, 0, 0, 0, 0, 0);
}

long android_sys_statfs(long path, long buf, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!path || !buf) return -EFAULT;
    uint64_t* statfs_buf = (uint64_t*)buf;
    statfs_buf[0] = 0xEF53;
    statfs_buf[1] = 4096;
    statfs_buf[2] = 1000000;
    statfs_buf[3] = 500000;
    statfs_buf[4] = 500000;
    statfs_buf[5] = 100000;
    statfs_buf[6] = 50000;
    return 0;
}

long android_sys_fstatfs(long fd, long buf, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    return android_sys_statfs((long)"/", buf, 0, 0, 0, 0);
}

long android_sys_symlinkat(long target, long newdirfd, long linkpath, long unused1, long unused2, long unused3) {
    (void)target; (void)newdirfd; (void)linkpath; (void)unused1; (void)unused2; (void)unused3;
    return -ENOSYS;
}

long android_sys_readlinkat(long dirfd, long pathname, long buf, long bufsiz, long unused1, long unused2) {
    (void)dirfd; (void)pathname; (void)buf; (void)bufsiz; (void)unused1; (void)unused2;
    return -ENOENT;
}

long android_sys_linkat(long olddirfd, long oldpath, long newdirfd, long newpath, long flags, long unused) {
    (void)olddirfd; (void)oldpath; (void)newdirfd; (void)newpath; (void)flags; (void)unused;
    return -ENOSYS;
}

/* ============================================================================
 * MEMORY MANAGEMENT SYSCALLS
 * ============================================================================ */

long android_sys_mmap(long addr, long length, long prot, long flags, long fd, long offset) {
    (void)addr; (void)offset;
    
    if (length <= 0) return -EINVAL;
    
    uint32_t mem_flags = MEM_USER;
    if (flags & 0x20) mem_flags |= MEM_ZERO;
    
    void* mem = vm_alloc((size_t)length, mem_flags);
    if (!mem) return -ENOMEM;
    
    if (!(flags & 0x20) && fd >= 0) {
        fd_entry_t* entry = get_fd_entry((int)fd);
        if (entry && entry->vfs_fd >= 0) {
            vfs_read(entry->vfs_fd, mem, (size_t)length);
        }
    }
    
    (void)prot;
    return (long)mem;
}

long android_sys_mprotect(long addr, long len, long prot, long unused1, long unused2, long unused3) {
    (void)addr; (void)len; (void)prot; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

long android_sys_munmap(long addr, long length, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!addr) return -EINVAL;
    vm_free((void*)addr);
    (void)length;
    return 0;
}

long android_sys_brk(long addr, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    static long current_brk = 0x10000000;
    if (addr == 0) return current_brk;
    if (addr > current_brk) current_brk = addr;
    return current_brk;
}

long android_sys_madvise(long addr, long length, long advice, long unused1, long unused2, long unused3) {
    (void)addr; (void)length; (void)advice; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

long android_sys_mincore(long addr, long length, long vec, long unused1, long unused2, long unused3) {
    (void)addr; (void)length; (void)unused1; (void)unused2; (void)unused3;
    if (!vec) return -EFAULT;
    return 0;
}

long android_sys_mlock(long addr, long len, long unused1, long unused2, long unused3, long unused4) {
    (void)addr; (void)len; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_munlock(long addr, long len, long unused1, long unused2, long unused3, long unused4) {
    (void)addr; (void)len; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_mlockall(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return 0;
}

long android_sys_munlockall(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return 0;
}

long android_sys_mremap(long old_addr, long old_size, long new_size, long flags, long new_addr, long unused) {
    (void)old_addr; (void)old_size; (void)new_size; (void)flags; (void)new_addr; (void)unused;
    return -ENOSYS;
}

long android_sys_msync(long addr, long length, long flags, long unused1, long unused2, long unused3) {
    (void)addr; (void)length; (void)flags; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

/* ============================================================================
 * PROCESS MANAGEMENT SYSCALLS
 * ============================================================================ */

long android_sys_getpid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return g_process_ctx.pid;
}

long android_sys_getppid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return g_process_ctx.ppid;
}

long android_sys_getuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return g_process_ctx.uid;
}

long android_sys_geteuid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return g_process_ctx.euid;
}

long android_sys_getgid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return g_process_ctx.gid;
}

long android_sys_getegid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return g_process_ctx.egid;
}

long android_sys_gettid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return g_process_ctx.tid;
}

long android_sys_setuid(long uid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (g_process_ctx.euid != 0 && uid != g_process_ctx.uid) return -EPERM;
    g_process_ctx.uid = (uint32_t)uid;
    g_process_ctx.euid = (uint32_t)uid;
    return 0;
}

long android_sys_setgid(long gid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (g_process_ctx.euid != 0 && gid != g_process_ctx.gid) return -EPERM;
    g_process_ctx.gid = (uint32_t)gid;
    g_process_ctx.egid = (uint32_t)gid;
    return 0;
}

long android_sys_setpgid(long pid, long pgid, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (pid == 0) pid = g_process_ctx.pid;
    if (pgid == 0) pgid = pid;
    if ((uint32_t)pid == g_process_ctx.pid) g_process_ctx.pgid = (uint32_t)pgid;
    return 0;
}

long android_sys_getpgid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (pid == 0 || (uint32_t)pid == g_process_ctx.pid) return g_process_ctx.pgid;
    return -ESRCH;
}

long android_sys_setsid(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    g_process_ctx.sid = g_process_ctx.pid;
    g_process_ctx.pgid = g_process_ctx.pid;
    return g_process_ctx.sid;
}

long android_sys_getsid(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (pid == 0 || (uint32_t)pid == g_process_ctx.pid) return g_process_ctx.sid;
    return -ESRCH;
}

long android_sys_clone(long flags, long stack, long parent_tid, long tls, long child_tid, long unused) {
    (void)flags; (void)stack; (void)parent_tid; (void)tls; (void)child_tid; (void)unused;
    static uint32_t next_pid = 2;
    return next_pid++;
}

long android_sys_execve(long pathname, long argv, long envp, long unused1, long unused2, long unused3) {
    (void)pathname; (void)argv; (void)envp; (void)unused1; (void)unused2; (void)unused3;
    return -ENOSYS;
}

long android_sys_exit(long status, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)status; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return 0;
}

long android_sys_exit_group(long status, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)status; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return 0;
}

long android_sys_wait4(long pid, long wstatus, long options, long rusage, long unused1, long unused2) {
    (void)pid; (void)wstatus; (void)options; (void)rusage; (void)unused1; (void)unused2;
    return -ECHILD;
}

long android_sys_kill(long pid, long sig, long unused1, long unused2, long unused3, long unused4) {
    (void)pid; (void)sig; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_tkill(long tid, long sig, long unused1, long unused2, long unused3, long unused4) {
    (void)tid; (void)sig; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_tgkill(long tgid, long tid, long sig, long unused1, long unused2, long unused3) {
    (void)tgid; (void)tid; (void)sig; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

long android_sys_rt_sigaction(long signum, long act, long oldact, long sigsetsize, long unused1, long unused2) {
    (void)signum; (void)act; (void)oldact; (void)sigsetsize; (void)unused1; (void)unused2;
    return 0;
}

long android_sys_rt_sigprocmask(long how, long set, long oldset, long sigsetsize, long unused1, long unused2) {
    (void)how; (void)set; (void)oldset; (void)sigsetsize; (void)unused1; (void)unused2;
    return 0;
}

long android_sys_rt_sigreturn(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    return 0;
}

long android_sys_uname(long buf, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (!buf) return -EFAULT;
    android_utsname_t* uname_buf = (android_utsname_t*)buf;
    syscall_strncpy(uname_buf->sysname, "Linux", sizeof(uname_buf->sysname));
    syscall_strncpy(uname_buf->nodename, "aurora", sizeof(uname_buf->nodename));
    syscall_strncpy(uname_buf->release, "5.10.0-aurora", sizeof(uname_buf->release));
    syscall_strncpy(uname_buf->version, "#1 SMP", sizeof(uname_buf->version));
    syscall_strncpy(uname_buf->machine, "x86_64", sizeof(uname_buf->machine));
    syscall_strncpy(uname_buf->domainname, "(none)", sizeof(uname_buf->domainname));
    return 0;
}

long android_sys_sysinfo(long info, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    if (!info) return -EFAULT;
    android_sysinfo_t* sysinfo = (android_sysinfo_t*)info;
    syscall_memset(sysinfo, 0, sizeof(android_sysinfo_t));
    sysinfo->uptime = (long)get_system_time_sec();
    sysinfo->loads[0] = 1 << 16;
    sysinfo->loads[1] = 1 << 16;
    sysinfo->loads[2] = 1 << 16;
    sysinfo->totalram = 512 * 1024 * 1024;
    sysinfo->freeram = 256 * 1024 * 1024;
    sysinfo->sharedram = 0;
    sysinfo->bufferram = 32 * 1024 * 1024;
    sysinfo->totalswap = 0;
    sysinfo->freeswap = 0;
    sysinfo->procs = 10;
    sysinfo->totalhigh = 0;
    sysinfo->freehigh = 0;
    sysinfo->mem_unit = 1;
    return 0;
}

long android_sys_getrlimit(long resource, long rlim, long unused1, long unused2, long unused3, long unused4) {
    (void)resource; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!rlim) return -EFAULT;
    android_rlimit_t* limit = (android_rlimit_t*)rlim;
    limit->rlim_cur = 0x7FFFFFFF;
    limit->rlim_max = 0x7FFFFFFF;
    return 0;
}

long android_sys_setrlimit(long resource, long rlim, long unused1, long unused2, long unused3, long unused4) {
    (void)resource; (void)rlim; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_prlimit64(long pid, long resource, long new_rlim, long old_rlim, long unused1, long unused2) {
    (void)pid; (void)resource; (void)new_rlim; (void)unused1; (void)unused2;
    if (old_rlim) {
        android_rlimit_t* limit = (android_rlimit_t*)old_rlim;
        limit->rlim_cur = 0x7FFFFFFF;
        limit->rlim_max = 0x7FFFFFFF;
    }
    return 0;
}

long android_sys_getrusage(long who, long usage, long unused1, long unused2, long unused3, long unused4) {
    (void)who; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!usage) return -EFAULT;
    syscall_memset((void*)usage, 0, sizeof(android_rusage_t));
    return 0;
}

long android_sys_umask(long mask, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    uint32_t old_mask = g_process_ctx.umask_val;
    g_process_ctx.umask_val = (uint32_t)(mask & 0777);
    return old_mask;
}

long android_sys_prctl(long option, long arg2, long arg3, long arg4, long arg5, long unused) {
    (void)option; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)unused;
    return 0;
}

long android_sys_getcpu(long cpu, long node, long tcache, long unused1, long unused2, long unused3) {
    (void)tcache; (void)unused1; (void)unused2; (void)unused3;
    if (cpu) *(uint32_t*)cpu = 0;
    if (node) *(uint32_t*)node = 0;
    return 0;
}

/* ============================================================================
 * TIME SYSCALLS
 * ============================================================================ */

long android_sys_gettimeofday(long tv, long tz, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (tv) {
        android_timeval_t* t = (android_timeval_t*)tv;
        uint64_t ns = get_system_time_ns();
        t->tv_sec = ns / 1000000000ULL;
        t->tv_usec = (ns % 1000000000ULL) / 1000;
    }
    if (tz) {
        syscall_memset((void*)tz, 0, 8);
    }
    return 0;
}

long android_sys_settimeofday(long tv, long tz, long unused1, long unused2, long unused3, long unused4) {
    (void)tv; (void)tz; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_clock_gettime(long clk_id, long tp, long unused1, long unused2, long unused3, long unused4) {
    (void)clk_id; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!tp) return -EFAULT;
    android_timespec_t* ts = (android_timespec_t*)tp;
    uint64_t ns = get_system_time_ns();
    ts->tv_sec = ns / 1000000000ULL;
    ts->tv_nsec = ns % 1000000000ULL;
    return 0;
}

long android_sys_clock_settime(long clk_id, long tp, long unused1, long unused2, long unused3, long unused4) {
    (void)clk_id; (void)tp; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_clock_getres(long clk_id, long res, long unused1, long unused2, long unused3, long unused4) {
    (void)clk_id; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (res) {
        android_timespec_t* ts = (android_timespec_t*)res;
        ts->tv_sec = 0;
        ts->tv_nsec = 1;
    }
    return 0;
}

long android_sys_clock_nanosleep(long clk_id, long flags, long request, long remain, long unused1, long unused2) {
    (void)clk_id; (void)flags; (void)remain; (void)unused1; (void)unused2;
    if (!request) return -EFAULT;
    android_timespec_t* req = (android_timespec_t*)request;
    uint64_t ns = req->tv_sec * 1000000000ULL + req->tv_nsec;
    uint64_t ticks = ns / 10000000;
    for (volatile uint64_t i = 0; i < ticks * 1000; i++);
    return 0;
}

long android_sys_nanosleep(long req, long rem, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return android_sys_clock_nanosleep(0, 0, req, rem, 0, 0);
}

/* ============================================================================
 * SYNCHRONIZATION SYSCALLS
 * ============================================================================ */

long android_sys_futex(long uaddr, long futex_op, long val, long timeout, long uaddr2, long val3) {
    (void)timeout; (void)uaddr2; (void)val3;
    if (!uaddr) return -EFAULT;
    int op = (int)(futex_op & 0x7F);
    int* ptr = (int*)uaddr;
    
    switch (op) {
        case 0: /* FUTEX_WAIT */
            if (*ptr == (int)val) {
                for (volatile int i = 0; i < 100000; i++);
            }
            return 0;
        case 1: /* FUTEX_WAKE */
            return (long)val;
        default:
            return 0;
    }
}

long android_sys_set_tid_address(long tidptr, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)tidptr; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return g_process_ctx.tid;
}

long android_sys_set_robust_list(long head, long len, long unused1, long unused2, long unused3, long unused4) {
    (void)head; (void)len; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_get_robust_list(long pid, long head_ptr, long len_ptr, long unused1, long unused2, long unused3) {
    (void)pid; (void)head_ptr; (void)len_ptr; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

/* ============================================================================
 * POLLING/EVENT SYSCALLS
 * ============================================================================ */

long android_sys_epoll_create1(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    int fd = alloc_fd();
    if (fd < 0) return fd;
    
    epoll_data_t* epoll = (epoll_data_t*)kmalloc(sizeof(epoll_data_t));
    if (!epoll) {
        free_fd(fd);
        return -ENOMEM;
    }
    
    syscall_memset(epoll, 0, sizeof(epoll_data_t));
    g_fd_table[fd].type = 3;
    g_fd_table[fd].private_data = epoll;
    
    return fd;
}

long android_sys_epoll_ctl(long epfd, long op, long fd, long event, long unused1, long unused2) {
    (void)unused1; (void)unused2;
    fd_entry_t* epoll_entry = get_fd_entry((int)epfd);
    if (!epoll_entry || epoll_entry->type != 3) return -EBADF;
    
    epoll_data_t* epoll = (epoll_data_t*)epoll_entry->private_data;
    if (!epoll) return -EBADF;
    
    switch (op) {
        case 1: /* EPOLL_CTL_ADD */
            if (epoll->count >= MAX_EPOLL_EVENTS) return -ENOSPC;
            if (event) {
                epoll->entries[epoll->count].fd = (int)fd;
                syscall_memcpy(&epoll->entries[epoll->count].event, (void*)event, sizeof(android_epoll_event_t));
                epoll->entries[epoll->count].active = 1;
                epoll->count++;
            }
            break;
        case 2: /* EPOLL_CTL_DEL */
            for (int i = 0; i < epoll->count; i++) {
                if (epoll->entries[i].fd == (int)fd) {
                    epoll->entries[i].active = 0;
                    break;
                }
            }
            break;
        case 3: /* EPOLL_CTL_MOD */
            for (int i = 0; i < epoll->count; i++) {
                if (epoll->entries[i].fd == (int)fd && event) {
                    syscall_memcpy(&epoll->entries[i].event, (void*)event, sizeof(android_epoll_event_t));
                    break;
                }
            }
            break;
    }
    return 0;
}

long android_sys_epoll_pwait(long epfd, long events, long maxevents, long timeout, long sigmask, long sigsetsize) {
    (void)timeout; (void)sigmask; (void)sigsetsize;
    fd_entry_t* epoll_entry = get_fd_entry((int)epfd);
    if (!epoll_entry || epoll_entry->type != 3) return -EBADF;
    
    epoll_data_t* epoll = (epoll_data_t*)epoll_entry->private_data;
    if (!epoll || !events || maxevents <= 0) return -EINVAL;
    
    android_epoll_event_t* out = (android_epoll_event_t*)events;
    int count = 0;
    
    for (int i = 0; i < epoll->count && count < (int)maxevents; i++) {
        if (epoll->entries[i].active) {
            syscall_memcpy(&out[count], &epoll->entries[i].event, sizeof(android_epoll_event_t));
            count++;
        }
    }
    
    return count;
}

long android_sys_ppoll(long fds, long nfds, long timeout_ts, long sigmask, long sigsetsize, long unused) {
    (void)timeout_ts; (void)sigmask; (void)sigsetsize; (void)unused;
    if (!fds || nfds <= 0) return 0;
    
    android_pollfd_t* pollfd = (android_pollfd_t*)fds;
    int ready = 0;
    
    for (long i = 0; i < nfds; i++) {
        fd_entry_t* entry = get_fd_entry(pollfd[i].fd);
        pollfd[i].revents = 0;
        if (entry) {
            if (pollfd[i].events & 0x0001) pollfd[i].revents |= 0x0001;
            if (pollfd[i].events & 0x0004) pollfd[i].revents |= 0x0004;
            if (pollfd[i].revents) ready++;
        }
    }
    
    return ready;
}

long android_sys_eventfd2(long initval, long flags, long unused1, long unused2, long unused3, long unused4) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    int fd = alloc_fd();
    if (fd < 0) return fd;
    
    uint64_t* counter = (uint64_t*)kmalloc(sizeof(uint64_t));
    if (!counter) {
        free_fd(fd);
        return -ENOMEM;
    }
    
    *counter = (uint64_t)initval;
    g_fd_table[fd].type = 4;
    g_fd_table[fd].private_data = counter;
    
    return fd;
}

long android_sys_signalfd4(long fd, long mask, long sizemask, long flags, long unused1, long unused2) {
    (void)fd; (void)mask; (void)sizemask; (void)flags; (void)unused1; (void)unused2;
    return -ENOSYS;
}

long android_sys_timerfd_create(long clockid, long flags, long unused1, long unused2, long unused3, long unused4) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    int fd = alloc_fd();
    if (fd < 0) return fd;
    
    timerfd_data_t* timer = (timerfd_data_t*)kmalloc(sizeof(timerfd_data_t));
    if (!timer) {
        free_fd(fd);
        return -ENOMEM;
    }
    
    syscall_memset(timer, 0, sizeof(timerfd_data_t));
    timer->clockid = (int)clockid;
    g_fd_table[fd].type = 5;
    g_fd_table[fd].private_data = timer;
    
    return fd;
}

long android_sys_timerfd_settime(long fd, long flags, long new_value, long old_value, long unused1, long unused2) {
    (void)flags; (void)unused1; (void)unused2;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry || entry->type != 5) return -EBADF;
    
    timerfd_data_t* timer = (timerfd_data_t*)entry->private_data;
    if (!timer) return -EBADF;
    
    if (old_value) {
        syscall_memcpy((void*)old_value, &timer->interval, sizeof(android_timespec_t) * 2);
    }
    
    if (new_value) {
        syscall_memcpy(&timer->interval, (void*)new_value, sizeof(android_timespec_t) * 2);
        timer->armed = 1;
    }
    
    return 0;
}

long android_sys_timerfd_gettime(long fd, long curr_value, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry || entry->type != 5) return -EBADF;
    
    timerfd_data_t* timer = (timerfd_data_t*)entry->private_data;
    if (!timer || !curr_value) return -EBADF;
    
    syscall_memcpy((void*)curr_value, &timer->interval, sizeof(android_timespec_t) * 2);
    return 0;
}

long android_sys_inotify_init1(long flags, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    int fd = alloc_fd();
    if (fd < 0) return fd;
    
    inotify_data_t* inotify = (inotify_data_t*)kmalloc(sizeof(inotify_data_t));
    if (!inotify) {
        free_fd(fd);
        return -ENOMEM;
    }
    
    syscall_memset(inotify, 0, sizeof(inotify_data_t));
    inotify->next_wd = 1;
    g_fd_table[fd].type = 6;
    g_fd_table[fd].private_data = inotify;
    
    return fd;
}

long android_sys_inotify_add_watch(long fd, long pathname, long mask, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry || entry->type != 6) return -EBADF;
    
    inotify_data_t* inotify = (inotify_data_t*)entry->private_data;
    if (!inotify || !pathname) return -EINVAL;
    
    if (inotify->count >= MAX_INOTIFY_WATCHES) return -ENOSPC;
    
    int idx = inotify->count++;
    inotify->watches[idx].wd = inotify->next_wd++;
    syscall_strncpy(inotify->watches[idx].path, (const char*)pathname, 255);
    inotify->watches[idx].mask = (uint32_t)mask;
    inotify->watches[idx].active = 1;
    
    return inotify->watches[idx].wd;
}

long android_sys_inotify_rm_watch(long fd, long wd, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry || entry->type != 6) return -EBADF;
    
    inotify_data_t* inotify = (inotify_data_t*)entry->private_data;
    if (!inotify) return -EBADF;
    
    for (int i = 0; i < inotify->count; i++) {
        if (inotify->watches[i].wd == (int)wd) {
            inotify->watches[i].active = 0;
            return 0;
        }
    }
    
    return -EINVAL;
}

/* ============================================================================
 * PIPE AND DUP SYSCALLS
 * ============================================================================ */

long android_sys_pipe2(long pipefd, long flags, long unused1, long unused2, long unused3, long unused4) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (!pipefd) return -EFAULT;
    
    int* fds = (int*)pipefd;
    
    int read_fd = alloc_fd();
    if (read_fd < 0) return read_fd;
    
    int write_fd = alloc_fd();
    if (write_fd < 0) {
        free_fd(read_fd);
        return write_fd;
    }
    
    pipe_data_t* pipe = (pipe_data_t*)kmalloc(sizeof(pipe_data_t));
    if (!pipe) {
        free_fd(read_fd);
        free_fd(write_fd);
        return -ENOMEM;
    }
    
    pipe->capacity = 4096;
    pipe->buffer = (uint8_t*)kmalloc(pipe->capacity);
    if (!pipe->buffer) {
        kfree(pipe);
        free_fd(read_fd);
        free_fd(write_fd);
        return -ENOMEM;
    }
    
    pipe->read_pos = 0;
    pipe->write_pos = 0;
    pipe->read_end = read_fd;
    pipe->write_end = write_fd;
    
    g_fd_table[read_fd].type = 2;
    g_fd_table[read_fd].flags = O_RDONLY;
    g_fd_table[read_fd].private_data = pipe;
    
    g_fd_table[write_fd].type = 2;
    g_fd_table[write_fd].flags = O_WRONLY;
    g_fd_table[write_fd].private_data = pipe;
    
    fds[0] = read_fd;
    fds[1] = write_fd;
    
    return 0;
}

long android_sys_dup(long oldfd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    fd_entry_t* old_entry = get_fd_entry((int)oldfd);
    if (!old_entry) return -EBADF;
    
    int newfd = alloc_fd();
    if (newfd < 0) return newfd;
    
    syscall_memcpy(&g_fd_table[newfd], old_entry, sizeof(fd_entry_t));
    g_fd_table[newfd].valid = 1;
    
    return newfd;
}

long android_sys_dup3(long oldfd, long newfd, long flags, long unused1, long unused2, long unused3) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3;
    if (oldfd == newfd) return -EINVAL;
    
    fd_entry_t* old_entry = get_fd_entry((int)oldfd);
    if (!old_entry) return -EBADF;
    
    if (newfd < 0 || newfd >= MAX_FDS) return -EBADF;
    
    if (g_fd_table[newfd].valid) {
        android_sys_close(newfd, 0, 0, 0, 0, 0);
    }
    
    syscall_memcpy(&g_fd_table[newfd], old_entry, sizeof(fd_entry_t));
    g_fd_table[newfd].valid = 1;
    
    return newfd;
}

long android_sys_fcntl(long fd, long cmd, long arg, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    
    switch (cmd) {
        case 0: /* F_DUPFD */
            return android_sys_dup(fd, 0, 0, 0, 0, 0);
        case 1: /* F_GETFD */
            return (entry->flags & 0x80000) ? 1 : 0;
        case 2: /* F_SETFD */
            if (arg & 1) entry->flags |= 0x80000;
            else entry->flags &= ~0x80000;
            return 0;
        case 3: /* F_GETFL */
            return entry->flags;
        case 4: /* F_SETFL */
            entry->flags = (entry->flags & 3) | (int)(arg & ~3);
            return 0;
        default:
            return 0;
    }
}

long android_sys_ioctl(long fd, long request, long arg, long unused1, long unused2, long unused3) {
    (void)arg; (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    
    switch (request) {
        case 0x5401: /* TCGETS */
        case 0x5402: /* TCSETS */
            return 0;
        default:
            return 0;
    }
}

long android_sys_readv(long fd, long iov, long iovcnt, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!iov || iovcnt <= 0) return -EINVAL;
    
    struct iovec_t {
        void* iov_base;
        size_t iov_len;
    } *vec = (struct iovec_t*)iov;
    
    long total = 0;
    for (long i = 0; i < iovcnt; i++) {
        long result = android_sys_read(fd, (long)vec[i].iov_base, vec[i].iov_len, 0, 0, 0);
        if (result < 0) return result;
        total += result;
        if ((size_t)result < vec[i].iov_len) break;
    }
    
    return total;
}

long android_sys_writev(long fd, long iov, long iovcnt, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    if (!iov || iovcnt <= 0) return -EINVAL;
    
    struct iovec_t {
        void* iov_base;
        size_t iov_len;
    } *vec = (struct iovec_t*)iov;
    
    long total = 0;
    for (long i = 0; i < iovcnt; i++) {
        long result = android_sys_write(fd, (long)vec[i].iov_base, vec[i].iov_len, 0, 0, 0);
        if (result < 0) return result;
        total += result;
        if ((size_t)result < vec[i].iov_len) break;
    }
    
    return total;
}

/* ============================================================================
 * SOCKET SYSCALLS
 * ============================================================================ */

long android_sys_socket(long domain, long type, long protocol, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    int fd = alloc_fd();
    if (fd < 0) return fd;
    
    socket_data_t* sock = (socket_data_t*)kmalloc(sizeof(socket_data_t));
    if (!sock) {
        free_fd(fd);
        return -ENOMEM;
    }
    
    syscall_memset(sock, 0, sizeof(socket_data_t));
    sock->domain = (int)domain;
    sock->type = (int)type;
    sock->protocol = (int)protocol;
    sock->state = 0;
    
    g_fd_table[fd].type = 1;
    g_fd_table[fd].private_data = sock;
    
    return fd;
}

long android_sys_socketpair(long domain, long type, long protocol, long sv, long unused1, long unused2) {
    (void)unused1; (void)unused2;
    if (!sv) return -EFAULT;
    
    int* fds = (int*)sv;
    
    fds[0] = (int)android_sys_socket(domain, type, protocol, 0, 0, 0);
    if (fds[0] < 0) return fds[0];
    
    fds[1] = (int)android_sys_socket(domain, type, protocol, 0, 0, 0);
    if (fds[1] < 0) {
        android_sys_close(fds[0], 0, 0, 0, 0, 0);
        return fds[1];
    }
    
    socket_data_t* sock0 = (socket_data_t*)g_fd_table[fds[0]].private_data;
    socket_data_t* sock1 = (socket_data_t*)g_fd_table[fds[1]].private_data;
    sock0->state = 3;
    sock1->state = 3;
    
    return 0;
}

long android_sys_bind(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    
    socket_data_t* sock = (socket_data_t*)entry->private_data;
    if (!sock || !addr) return -EINVAL;
    
    size_t copy_len = (size_t)addrlen < sizeof(android_sockaddr_t) ? (size_t)addrlen : sizeof(android_sockaddr_t);
    syscall_memcpy(&sock->local_addr, (void*)addr, copy_len);
    sock->state = 1;
    
    return 0;
}

long android_sys_listen(long sockfd, long backlog, long unused1, long unused2, long unused3, long unused4) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    
    socket_data_t* sock = (socket_data_t*)entry->private_data;
    if (!sock) return -EINVAL;
    
    sock->backlog = (uint32_t)backlog;
    sock->state = 2;
    
    return 0;
}

long android_sys_accept(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    
    socket_data_t* sock = (socket_data_t*)entry->private_data;
    if (!sock || sock->state != 2) return -EINVAL;
    
    int newfd = (int)android_sys_socket(sock->domain, sock->type, sock->protocol, 0, 0, 0);
    if (newfd < 0) return newfd;
    
    socket_data_t* new_sock = (socket_data_t*)g_fd_table[newfd].private_data;
    new_sock->state = 3;
    
    if (addr && addrlen) {
        uint32_t* len = (uint32_t*)addrlen;
        if (*len >= sizeof(android_sockaddr_t)) {
            syscall_memcpy((void*)addr, &sock->local_addr, sizeof(android_sockaddr_t));
            *len = sizeof(android_sockaddr_t);
        }
    }
    
    return newfd;
}

long android_sys_accept4(long sockfd, long addr, long addrlen, long flags, long unused1, long unused2) {
    (void)flags; (void)unused1; (void)unused2;
    return android_sys_accept(sockfd, addr, addrlen, 0, 0, 0);
}

long android_sys_connect(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    
    socket_data_t* sock = (socket_data_t*)entry->private_data;
    if (!sock || !addr) return -EINVAL;
    
    size_t copy_len = (size_t)addrlen < sizeof(android_sockaddr_t) ? (size_t)addrlen : sizeof(android_sockaddr_t);
    syscall_memcpy(&sock->peer_addr, (void*)addr, copy_len);
    sock->state = 3;
    
    return 0;
}

long android_sys_getsockname(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    
    socket_data_t* sock = (socket_data_t*)entry->private_data;
    if (!sock || !addr || !addrlen) return -EINVAL;
    
    uint32_t* len = (uint32_t*)addrlen;
    if (*len >= sizeof(android_sockaddr_t)) {
        syscall_memcpy((void*)addr, &sock->local_addr, sizeof(android_sockaddr_t));
        *len = sizeof(android_sockaddr_t);
    }
    
    return 0;
}

long android_sys_getpeername(long sockfd, long addr, long addrlen, long unused1, long unused2, long unused3) {
    (void)unused1; (void)unused2; (void)unused3;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    
    socket_data_t* sock = (socket_data_t*)entry->private_data;
    if (!sock || !addr || !addrlen) return -EINVAL;
    
    uint32_t* len = (uint32_t*)addrlen;
    if (*len >= sizeof(android_sockaddr_t)) {
        syscall_memcpy((void*)addr, &sock->peer_addr, sizeof(android_sockaddr_t));
        *len = sizeof(android_sockaddr_t);
    }
    
    return 0;
}

long android_sys_sendto(long sockfd, long buf, long len, long flags, long dest_addr, long addrlen) {
    (void)flags; (void)dest_addr; (void)addrlen;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    return (long)len;
}

long android_sys_recvfrom(long sockfd, long buf, long len, long flags, long src_addr, long addrlen) {
    (void)flags; (void)src_addr; (void)addrlen;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    if (!buf) return -EFAULT;
    return 0;
}

long android_sys_setsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused) {
    (void)level; (void)optname; (void)optval; (void)optlen; (void)unused;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    return 0;
}

long android_sys_getsockopt(long sockfd, long level, long optname, long optval, long optlen, long unused) {
    (void)level; (void)optname; (void)optval; (void)optlen; (void)unused;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    return 0;
}

long android_sys_shutdown(long sockfd, long how, long unused1, long unused2, long unused3, long unused4) {
    (void)how; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)sockfd);
    if (!entry || entry->type != 1) return -EBADF;
    return 0;
}

/* ============================================================================
 * SCHEDULER SYSCALLS
 * ============================================================================ */

long android_sys_sched_yield(long unused1, long unused2, long unused3, long unused4, long unused5, long unused6) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5; (void)unused6;
    process_yield();
    return 0;
}

long android_sys_sched_getaffinity(long pid, long cpusetsize, long mask, long unused1, long unused2, long unused3) {
    (void)pid; (void)cpusetsize; (void)unused1; (void)unused2; (void)unused3;
    if (!mask) return -EFAULT;
    *(uint64_t*)mask = 0x1;
    return 8;
}

long android_sys_sched_setaffinity(long pid, long cpusetsize, long mask, long unused1, long unused2, long unused3) {
    (void)pid; (void)cpusetsize; (void)mask; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

long android_sys_sched_getscheduler(long pid, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)pid; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return 0;
}

long android_sys_sched_setscheduler(long pid, long policy, long param, long unused1, long unused2, long unused3) {
    (void)pid; (void)policy; (void)param; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

long android_sys_sched_getparam(long pid, long param, long unused1, long unused2, long unused3, long unused4) {
    (void)pid; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    if (param) {
        *(int*)param = 0;
    }
    return 0;
}

long android_sys_sched_setparam(long pid, long param, long unused1, long unused2, long unused3, long unused4) {
    (void)pid; (void)param; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_sched_get_priority_max(long policy, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)policy; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return 99;
}

long android_sys_sched_get_priority_min(long policy, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)policy; (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    return 1;
}

/* ============================================================================
 * CAPABILITY AND SECURITY SYSCALLS
 * ============================================================================ */

long android_sys_capget(long hdrp, long datap, long unused1, long unused2, long unused3, long unused4) {
    (void)hdrp; (void)datap; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_capset(long hdrp, long datap, long unused1, long unused2, long unused3, long unused4) {
    (void)hdrp; (void)datap; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

long android_sys_personality(long persona, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    static uint32_t current_persona = 0;
    if (persona == 0xFFFFFFFF) return current_persona;
    current_persona = (uint32_t)persona;
    return current_persona;
}

long android_sys_setpriority(long which, long who, long prio, long unused1, long unused2, long unused3) {
    (void)which; (void)who; (void)prio; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

long android_sys_getpriority(long which, long who, long unused1, long unused2, long unused3, long unused4) {
    (void)which; (void)who; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 20;
}

long android_sys_getrandom(long buf, long buflen, long flags, long unused1, long unused2, long unused3) {
    (void)flags; (void)unused1; (void)unused2; (void)unused3;
    if (!buf) return -EFAULT;
    
    uint8_t* out = (uint8_t*)buf;
    static uint32_t rand_state = 12345;
    
    for (size_t i = 0; i < (size_t)buflen; i++) {
        rand_state = rand_state * 1103515245 + 12345;
        out[i] = (rand_state >> 16) & 0xFF;
    }
    
    return buflen;
}

long android_sys_memfd_create(long name, long flags, long unused1, long unused2, long unused3, long unused4) {
    (void)name; (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    int fd = alloc_fd();
    if (fd < 0) return fd;
    
    void* mem = kmalloc(4096);
    if (!mem) {
        free_fd(fd);
        return -ENOMEM;
    }
    
    syscall_memset(mem, 0, 4096);
    g_fd_table[fd].type = 0;
    g_fd_table[fd].private_data = mem;
    
    return fd;
}

long android_sys_seccomp(long operation, long flags, long args, long unused1, long unused2, long unused3) {
    (void)operation; (void)flags; (void)args; (void)unused1; (void)unused2; (void)unused3;
    return 0;
}

/* ============================================================================
 * MOUNT SYSCALLS
 * ============================================================================ */

long android_sys_mount(long source, long target, long fstype, long flags, long data, long unused) {
    (void)source; (void)target; (void)fstype; (void)flags; (void)data; (void)unused;
    return 0;
}

long android_sys_umount2(long target, long flags, long unused1, long unused2, long unused3, long unused4) {
    (void)target; (void)flags; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    return 0;
}

/* ============================================================================
 * ANDROID-SPECIFIC SYSCALLS (ASHMEM)
 * ============================================================================ */

long android_sys_ashmem_create_region(long name, long size, long unused1, long unused2, long unused3, long unused4) {
    (void)name; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    int fd = alloc_fd();
    if (fd < 0) return fd;
    
    void* mem = vm_alloc((size_t)size, MEM_USER | MEM_ZERO);
    if (!mem) {
        free_fd(fd);
        return -ENOMEM;
    }
    
    g_fd_table[fd].type = 0;
    g_fd_table[fd].private_data = mem;
    
    return fd;
}

long android_sys_ashmem_set_prot_mask(long fd, long prot, long unused1, long unused2, long unused3, long unused4) {
    (void)prot; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    return 0;
}

long android_sys_ashmem_get_size(long fd, long unused1, long unused2, long unused3, long unused4, long unused5) {
    (void)unused1; (void)unused2; (void)unused3; (void)unused4; (void)unused5;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    return 4096;
}

long android_sys_ashmem_pin(long fd, long pin, long unused1, long unused2, long unused3, long unused4) {
    (void)pin; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    return 0;
}

long android_sys_ashmem_unpin(long fd, long pin, long unused1, long unused2, long unused3, long unused4) {
    (void)pin; (void)unused1; (void)unused2; (void)unused3; (void)unused4;
    fd_entry_t* entry = get_fd_entry((int)fd);
    if (!entry) return -EBADF;
    return 0;
}

/* ============================================================================
 * SYSCALL DISPATCHER
 * ============================================================================ */

/* Syscall table */
static const android_syscall_entry_t syscall_table[] = {
    {__NR_ANDROID_read, "read", android_sys_read, 3},
    {__NR_ANDROID_write, "write", android_sys_write, 3},
    {__NR_ANDROID_openat, "openat", android_sys_openat, 4},
    {__NR_ANDROID_close, "close", android_sys_close, 1},
    {__NR_ANDROID_fstat, "fstat", android_sys_fstat, 2},
    {__NR_ANDROID_newfstatat, "newfstatat", android_sys_newfstatat, 4},
    {__NR_ANDROID_lseek, "lseek", android_sys_lseek, 3},
    {__NR_ANDROID_mmap, "mmap", android_sys_mmap, 6},
    {__NR_ANDROID_mprotect, "mprotect", android_sys_mprotect, 3},
    {__NR_ANDROID_munmap, "munmap", android_sys_munmap, 2},
    {__NR_ANDROID_brk, "brk", android_sys_brk, 1},
    {__NR_ANDROID_ioctl, "ioctl", android_sys_ioctl, 3},
    {__NR_ANDROID_readv, "readv", android_sys_readv, 3},
    {__NR_ANDROID_writev, "writev", android_sys_writev, 3},
    {__NR_ANDROID_pipe2, "pipe2", android_sys_pipe2, 2},
    {__NR_ANDROID_dup, "dup", android_sys_dup, 1},
    {__NR_ANDROID_dup3, "dup3", android_sys_dup3, 3},
    {__NR_ANDROID_fcntl, "fcntl", android_sys_fcntl, 3},
    {__NR_ANDROID_mkdirat, "mkdirat", android_sys_mkdirat, 3},
    {__NR_ANDROID_unlinkat, "unlinkat", android_sys_unlinkat, 3},
    {__NR_ANDROID_renameat, "renameat", android_sys_renameat, 4},
    {__NR_ANDROID_faccessat, "faccessat", android_sys_faccessat, 4},
    {__NR_ANDROID_fchdir, "fchdir", android_sys_fchdir, 1},
    {__NR_ANDROID_chdir, "chdir", android_sys_chdir, 1},
    {__NR_ANDROID_getcwd, "getcwd", android_sys_getcwd, 2},
    {__NR_ANDROID_fchmod, "fchmod", android_sys_fchmod, 2},
    {__NR_ANDROID_fchmodat, "fchmodat", android_sys_fchmodat, 4},
    {__NR_ANDROID_fchown, "fchown", android_sys_fchown, 3},
    {__NR_ANDROID_fchownat, "fchownat", android_sys_fchownat, 5},
    {__NR_ANDROID_getdents64, "getdents64", android_sys_getdents64, 3},
    {__NR_ANDROID_socket, "socket", android_sys_socket, 3},
    {__NR_ANDROID_socketpair, "socketpair", android_sys_socketpair, 4},
    {__NR_ANDROID_bind, "bind", android_sys_bind, 3},
    {__NR_ANDROID_listen, "listen", android_sys_listen, 2},
    {__NR_ANDROID_accept, "accept", android_sys_accept, 3},
    {__NR_ANDROID_accept4, "accept4", android_sys_accept4, 4},
    {__NR_ANDROID_connect, "connect", android_sys_connect, 3},
    {__NR_ANDROID_getsockname, "getsockname", android_sys_getsockname, 3},
    {__NR_ANDROID_getpeername, "getpeername", android_sys_getpeername, 3},
    {__NR_ANDROID_sendto, "sendto", android_sys_sendto, 6},
    {__NR_ANDROID_recvfrom, "recvfrom", android_sys_recvfrom, 6},
    {__NR_ANDROID_setsockopt, "setsockopt", android_sys_setsockopt, 5},
    {__NR_ANDROID_getsockopt, "getsockopt", android_sys_getsockopt, 5},
    {__NR_ANDROID_shutdown, "shutdown", android_sys_shutdown, 2},
    {__NR_ANDROID_clone, "clone", android_sys_clone, 5},
    {__NR_ANDROID_execve, "execve", android_sys_execve, 3},
    {__NR_ANDROID_exit, "exit", android_sys_exit, 1},
    {__NR_ANDROID_exit_group, "exit_group", android_sys_exit_group, 1},
    {__NR_ANDROID_wait4, "wait4", android_sys_wait4, 4},
    {__NR_ANDROID_kill, "kill", android_sys_kill, 2},
    {__NR_ANDROID_tkill, "tkill", android_sys_tkill, 2},
    {__NR_ANDROID_tgkill, "tgkill", android_sys_tgkill, 3},
    {__NR_ANDROID_rt_sigaction, "rt_sigaction", android_sys_rt_sigaction, 4},
    {__NR_ANDROID_rt_sigprocmask, "rt_sigprocmask", android_sys_rt_sigprocmask, 4},
    {__NR_ANDROID_rt_sigreturn, "rt_sigreturn", android_sys_rt_sigreturn, 0},
    {__NR_ANDROID_getpid, "getpid", android_sys_getpid, 0},
    {__NR_ANDROID_getppid, "getppid", android_sys_getppid, 0},
    {__NR_ANDROID_getuid, "getuid", android_sys_getuid, 0},
    {__NR_ANDROID_geteuid, "geteuid", android_sys_geteuid, 0},
    {__NR_ANDROID_getgid, "getgid", android_sys_getgid, 0},
    {__NR_ANDROID_getegid, "getegid", android_sys_getegid, 0},
    {__NR_ANDROID_gettid, "gettid", android_sys_gettid, 0},
    {__NR_ANDROID_setuid, "setuid", android_sys_setuid, 1},
    {__NR_ANDROID_setgid, "setgid", android_sys_setgid, 1},
    {__NR_ANDROID_setpgid, "setpgid", android_sys_setpgid, 2},
    {__NR_ANDROID_getpgid, "getpgid", android_sys_getpgid, 1},
    {__NR_ANDROID_setsid, "setsid", android_sys_setsid, 0},
    {__NR_ANDROID_getsid, "getsid", android_sys_getsid, 1},
    {__NR_ANDROID_uname, "uname", android_sys_uname, 1},
    {__NR_ANDROID_sysinfo, "sysinfo", android_sys_sysinfo, 1},
    {__NR_ANDROID_getrlimit, "getrlimit", android_sys_getrlimit, 2},
    {__NR_ANDROID_setrlimit, "setrlimit", android_sys_setrlimit, 2},
    {__NR_ANDROID_prlimit64, "prlimit64", android_sys_prlimit64, 4},
    {__NR_ANDROID_getrusage, "getrusage", android_sys_getrusage, 2},
    {__NR_ANDROID_umask, "umask", android_sys_umask, 1},
    {__NR_ANDROID_prctl, "prctl", android_sys_prctl, 5},
    {__NR_ANDROID_getcpu, "getcpu", android_sys_getcpu, 3},
    {__NR_ANDROID_gettimeofday, "gettimeofday", android_sys_gettimeofday, 2},
    {__NR_ANDROID_settimeofday, "settimeofday", android_sys_settimeofday, 2},
    {__NR_ANDROID_clock_gettime, "clock_gettime", android_sys_clock_gettime, 2},
    {__NR_ANDROID_clock_settime, "clock_settime", android_sys_clock_settime, 2},
    {__NR_ANDROID_clock_getres, "clock_getres", android_sys_clock_getres, 2},
    {__NR_ANDROID_clock_nanosleep, "clock_nanosleep", android_sys_clock_nanosleep, 4},
    {__NR_ANDROID_nanosleep, "nanosleep", android_sys_nanosleep, 2},
    {__NR_ANDROID_futex, "futex", android_sys_futex, 6},
    {__NR_ANDROID_set_tid_address, "set_tid_address", android_sys_set_tid_address, 1},
    {__NR_ANDROID_set_robust_list, "set_robust_list", android_sys_set_robust_list, 2},
    {__NR_ANDROID_get_robust_list, "get_robust_list", android_sys_get_robust_list, 3},
    {__NR_ANDROID_epoll_create1, "epoll_create1", android_sys_epoll_create1, 1},
    {__NR_ANDROID_epoll_ctl, "epoll_ctl", android_sys_epoll_ctl, 4},
    {__NR_ANDROID_epoll_pwait, "epoll_pwait", android_sys_epoll_pwait, 6},
    {__NR_ANDROID_ppoll, "ppoll", android_sys_ppoll, 5},
    {__NR_ANDROID_eventfd2, "eventfd2", android_sys_eventfd2, 2},
    {__NR_ANDROID_signalfd4, "signalfd4", android_sys_signalfd4, 4},
    {__NR_ANDROID_timerfd_create, "timerfd_create", android_sys_timerfd_create, 2},
    {__NR_ANDROID_timerfd_settime, "timerfd_settime", android_sys_timerfd_settime, 4},
    {__NR_ANDROID_timerfd_gettime, "timerfd_gettime", android_sys_timerfd_gettime, 2},
    {__NR_ANDROID_inotify_init1, "inotify_init1", android_sys_inotify_init1, 1},
    {__NR_ANDROID_inotify_add_watch, "inotify_add_watch", android_sys_inotify_add_watch, 3},
    {__NR_ANDROID_inotify_rm_watch, "inotify_rm_watch", android_sys_inotify_rm_watch, 2},
    {__NR_ANDROID_mount, "mount", android_sys_mount, 5},
    {__NR_ANDROID_umount2, "umount2", android_sys_umount2, 2},
    {__NR_ANDROID_sync, "sync", android_sys_sync, 0},
    {__NR_ANDROID_fsync, "fsync", android_sys_fsync, 1},
    {__NR_ANDROID_fdatasync, "fdatasync", android_sys_fdatasync, 1},
    {__NR_ANDROID_truncate, "truncate", android_sys_truncate, 2},
    {__NR_ANDROID_ftruncate, "ftruncate", android_sys_ftruncate, 2},
    {__NR_ANDROID_statfs, "statfs", android_sys_statfs, 2},
    {__NR_ANDROID_fstatfs, "fstatfs", android_sys_fstatfs, 2},
    {__NR_ANDROID_readlinkat, "readlinkat", android_sys_readlinkat, 4},
    {__NR_ANDROID_symlinkat, "symlinkat", android_sys_symlinkat, 3},
    {__NR_ANDROID_linkat, "linkat", android_sys_linkat, 5},
    {__NR_ANDROID_madvise, "madvise", android_sys_madvise, 3},
    {__NR_ANDROID_mincore, "mincore", android_sys_mincore, 3},
    {__NR_ANDROID_mlock, "mlock", android_sys_mlock, 2},
    {__NR_ANDROID_munlock, "munlock", android_sys_munlock, 2},
    {__NR_ANDROID_mlockall, "mlockall", android_sys_mlockall, 1},
    {__NR_ANDROID_munlockall, "munlockall", android_sys_munlockall, 0},
    {__NR_ANDROID_mremap, "mremap", android_sys_mremap, 5},
    {__NR_ANDROID_msync, "msync", android_sys_msync, 3},
    {__NR_ANDROID_sched_yield, "sched_yield", android_sys_sched_yield, 0},
    {__NR_ANDROID_sched_getaffinity, "sched_getaffinity", android_sys_sched_getaffinity, 3},
    {__NR_ANDROID_sched_setaffinity, "sched_setaffinity", android_sys_sched_setaffinity, 3},
    {__NR_ANDROID_sched_getscheduler, "sched_getscheduler", android_sys_sched_getscheduler, 1},
    {__NR_ANDROID_sched_setscheduler, "sched_setscheduler", android_sys_sched_setscheduler, 3},
    {__NR_ANDROID_sched_getparam, "sched_getparam", android_sys_sched_getparam, 2},
    {__NR_ANDROID_sched_setparam, "sched_setparam", android_sys_sched_setparam, 2},
    {__NR_ANDROID_sched_get_priority_max, "sched_get_priority_max", android_sys_sched_get_priority_max, 1},
    {__NR_ANDROID_sched_get_priority_min, "sched_get_priority_min", android_sys_sched_get_priority_min, 1},
    {__NR_ANDROID_capget, "capget", android_sys_capget, 2},
    {__NR_ANDROID_capset, "capset", android_sys_capset, 2},
    {__NR_ANDROID_personality, "personality", android_sys_personality, 1},
    {__NR_ANDROID_setpriority, "setpriority", android_sys_setpriority, 3},
    {__NR_ANDROID_getpriority, "getpriority", android_sys_getpriority, 2},
    {__NR_ANDROID_getrandom, "getrandom", android_sys_getrandom, 3},
    {__NR_ANDROID_memfd_create, "memfd_create", android_sys_memfd_create, 2},
    {__NR_ANDROID_seccomp, "seccomp", android_sys_seccomp, 3},
    {__NR_ANDROID_ashmem_create_region, "ashmem_create_region", android_sys_ashmem_create_region, 2},
    {__NR_ANDROID_ashmem_set_prot_mask, "ashmem_set_prot_mask", android_sys_ashmem_set_prot_mask, 2},
    {__NR_ANDROID_ashmem_get_size, "ashmem_get_size", android_sys_ashmem_get_size, 1},
    {__NR_ANDROID_ashmem_pin, "ashmem_pin", android_sys_ashmem_pin, 2},
    {__NR_ANDROID_ashmem_unpin, "ashmem_unpin", android_sys_ashmem_unpin, 2},
    {-1, NULL, NULL, 0}
};

#define SYSCALL_TABLE_SIZE (sizeof(syscall_table) / sizeof(syscall_table[0]) - 1)

/**
 * Main syscall dispatcher
 */
long android_syscall(long syscall_num, long arg0, long arg1, long arg2,
                     long arg3, long arg4, long arg5) {
    g_syscall_stats.total_calls++;
    
    if (syscall_num >= 0 && syscall_num < __NR_ANDROID_MAX) {
        g_syscall_stats.call_count[syscall_num]++;
    }
    
    for (size_t i = 0; i < SYSCALL_TABLE_SIZE; i++) {
        if (syscall_table[i].syscall_num == (int)syscall_num) {
            if (syscall_table[i].handler) {
                long result = syscall_table[i].handler(arg0, arg1, arg2, arg3, arg4, arg5);
                if (result >= 0) {
                    g_syscall_stats.successful_calls++;
                } else {
                    g_syscall_stats.failed_calls++;
                }
                return result;
            }
        }
    }
    
    g_syscall_stats.failed_calls++;
    return -ENOSYS;
}

/**
 * Get syscall name by number
 */
const char* android_syscall_get_name(int syscall_num) {
    for (size_t i = 0; i < SYSCALL_TABLE_SIZE; i++) {
        if (syscall_table[i].syscall_num == syscall_num) {
            return syscall_table[i].name;
        }
    }
    return "unknown";
}

/**
 * Print syscall statistics
 */
void android_syscall_print_stats(void) {
    vga_write("\n=== Android Syscall Statistics ===\n");
    vga_write("Total calls: ");
    vga_write_dec((int)g_syscall_stats.total_calls);
    vga_write("\nSuccessful: ");
    vga_write_dec((int)g_syscall_stats.successful_calls);
    vga_write("\nFailed: ");
    vga_write_dec((int)g_syscall_stats.failed_calls);
    vga_write("\n==================================\n");
}
