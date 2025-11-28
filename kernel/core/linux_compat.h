/**
 * Aurora OS - Linux Kernel Compatibility Layer
 * 
 * Provides Linux kernel compatibility with Aurora enhancements
 * including Crystal-Kyber encryption and performance optimizations
 */

#ifndef AURORA_LINUX_COMPAT_H
#define AURORA_LINUX_COMPAT_H

#include <stdint.h>
#include <stddef.h>

/* Linux kernel compatibility version */
#define AURORA_LINUX_COMPAT_VERSION "1.0.0"
#define AURORA_LINUX_KERNEL_VERSION "6.6-aurora"

/* Linux kernel compatibility status codes */
#define LINUX_COMPAT_SUCCESS 0
#define LINUX_COMPAT_ERROR -1
#define LINUX_COMPAT_NOT_INITIALIZED -2
#define LINUX_COMPAT_INVALID_PARAM -3

/* Aurora enhancement flags */
#define AURORA_ENHANCE_CRYPTO     (1 << 0)  /* Crystal-Kyber encryption */
#define AURORA_ENHANCE_PERF       (1 << 1)  /* Performance optimizations */
#define AURORA_ENHANCE_SECURITY   (1 << 2)  /* Enhanced security features */
#define AURORA_ENHANCE_QUANTUM    (1 << 3)  /* Quantum computing support */
#define AURORA_ENHANCE_ALL        (0xFF)    /* All enhancements */

/* Linux syscall numbers (x86-64 compatible) */
#define __NR_read       0
#define __NR_write      1
#define __NR_open       2
#define __NR_close      3
#define __NR_stat       4
#define __NR_fstat      5
#define __NR_lstat      6
#define __NR_poll       7
#define __NR_lseek      8
#define __NR_mmap       9
#define __NR_mprotect   10
#define __NR_munmap     11
#define __NR_brk        12
#define __NR_ioctl      16
#define __NR_pipe       22
#define __NR_dup        32
#define __NR_dup2       33
#define __NR_getpid     39
#define __NR_socket     41
#define __NR_connect    42
#define __NR_accept     43
#define __NR_sendto     44
#define __NR_recvfrom   45
#define __NR_bind       49
#define __NR_listen     50
#define __NR_fork       57
#define __NR_execve     59
#define __NR_exit       60
#define __NR_wait4      61
#define __NR_fcntl      72
#define __NR_chdir      80
#define __NR_mkdir      83
#define __NR_rmdir      84
#define __NR_getcwd     79
#define __NR_readdir    89
#define __NR_getuid     102

/* File types */
#define S_IFMT   0170000  /* File type mask */
#define S_IFDIR  0040000  /* Directory */
#define S_IFREG  0100000  /* Regular file */
#define S_IFLNK  0120000  /* Symbolic link */
#define S_IFBLK  0060000  /* Block device */
#define S_IFCHR  0020000  /* Character device */
#define S_IFIFO  0010000  /* FIFO */
#define S_IFSOCK 0140000  /* Socket */

/* File permissions */
#define S_IRWXU  00700    /* Owner RWX */
#define S_IRUSR  00400    /* Owner read */
#define S_IWUSR  00200    /* Owner write */
#define S_IXUSR  00100    /* Owner execute */
#define S_IRWXG  00070    /* Group RWX */
#define S_IRWXO  00007    /* Other RWX */

/* Open flags (Linux compatible, may conflict with VFS) */
#ifndef O_RDONLY
#define O_RDONLY    0x0000
#endif
#ifndef O_WRONLY
#define O_WRONLY    0x0001
#endif
#ifndef O_RDWR
#define O_RDWR      0x0002
#endif
#ifndef O_CREAT
#define O_CREAT     0x0040
#endif
#ifndef O_TRUNC
#define O_TRUNC     0x0200
#endif
#ifndef O_APPEND
#define O_APPEND    0x0400
#endif
#ifndef O_NONBLOCK
#define O_NONBLOCK  0x0800
#endif

/* Seek constants */
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

/* Memory protection flags */
#define PROT_READ   0x1
#define PROT_WRITE  0x2
#define PROT_EXEC   0x4
#define PROT_NONE   0x0

/* mmap flags */
#define MAP_SHARED    0x01
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FIXED     0x10
#define MAP_FAILED    ((void*)-1)

/* fcntl commands */
#define F_DUPFD     0
#define F_GETFD     1
#define F_SETFD     2
#define F_GETFL     3
#define F_SETFL     4

/* ioctl requests */
#define TIOCGWINSZ  0x5413
#define TIOCSWINSZ  0x5414
#define TCGETS      0x5401
#define TCSETS      0x5402

/* Module metadata macros */
#define MODULE_LICENSE(lic) \
    static const char __module_license[] __attribute__((unused)) = lic
#define MODULE_AUTHOR(auth) \
    static const char __module_author[] __attribute__((unused)) = auth
#define MODULE_DESCRIPTION(desc) \
    static const char __module_description[] __attribute__((unused)) = desc
#define MODULE_VERSION(ver) \
    static const char __module_version[] __attribute__((unused)) = ver

/* Symbol export macros */
#define EXPORT_SYMBOL(sym) \
    static void* __ksym_##sym __attribute__((used, section(".ksymtab"))) = (void*)&sym
#define EXPORT_SYMBOL_GPL(sym) \
    static void* __ksym_gpl_##sym __attribute__((used, section(".ksymtab_gpl"))) = (void*)&sym

/* Linux stat structure */
typedef struct linux_stat {
    uint64_t st_dev;
    uint64_t st_ino;
    uint32_t st_mode;
    uint32_t st_nlink;
    uint32_t st_uid;
    uint32_t st_gid;
    uint64_t st_rdev;
    int64_t  st_size;
    int64_t  st_blksize;
    int64_t  st_blocks;
    int64_t  st_atime;
    int64_t  st_mtime;
    int64_t  st_ctime;
} linux_stat_t;

/* Linux directory entry */
typedef struct linux_dirent {
    uint64_t d_ino;
    int64_t  d_off;
    uint16_t d_reclen;
    uint8_t  d_type;
    char     d_name[256];
} linux_dirent_t;

/* Symbol table entry */
typedef struct {
    const char* name;
    void* address;
    uint32_t flags;
} kernel_symbol_t;

/* Max exported symbols */
#define MAX_EXPORTED_SYMBOLS 256

/**
 * Linux kernel module structure
 */
typedef struct {
    const char* name;
    const char* version;
    const char* license;
    const char* author;
    const char* description;
    uint32_t enhancement_flags;
    void (*init_func)(void);
    void (*cleanup_func)(void);
    uint8_t is_loaded;
    uint32_t load_address;
    size_t module_size;
    kernel_symbol_t* exported_symbols;
    uint32_t symbol_count;
} linux_module_t;

/**
 * Aurora enhancement context
 */
typedef struct {
    uint8_t crypto_enabled;
    uint8_t perf_optimization_enabled;
    uint8_t quantum_enabled;
    uint32_t enhancement_flags;
    uint64_t optimizations_applied;
    uint64_t crypto_operations;
} aurora_enhancement_ctx_t;

/**
 * Linux kernel compatibility context
 */
typedef struct {
    uint8_t initialized;
    uint32_t module_count;
    linux_module_t modules[32];
    aurora_enhancement_ctx_t enhancement_ctx;
    uint64_t uptime_ticks;
    /* Symbol table for exported symbols */
    kernel_symbol_t symbol_table[MAX_EXPORTED_SYMBOLS];
    uint32_t symbol_count;
    /* Current program break for brk() */
    void* program_break;
    void* program_break_start;
} linux_compat_ctx_t;

/* Initialization and cleanup */
int linux_compat_init(void);
void linux_compat_cleanup(void);

/* Module management */
int linux_compat_load_module(const char* name, const char* version,
                             uint32_t enhancement_flags,
                             void (*init_func)(void),
                             void (*cleanup_func)(void));
int linux_compat_unload_module(const char* name);
linux_module_t* linux_compat_find_module(const char* name);
void linux_compat_list_modules(void);

/* Extended module functions */
int linux_compat_export_symbol(const char* name, void* address, uint32_t flags);
void* linux_compat_find_symbol(const char* name);
int linux_compat_set_module_license(linux_module_t* module, const char* license);
int linux_compat_set_module_author(linux_module_t* module, const char* author);
int linux_compat_set_module_description(linux_module_t* module, const char* desc);

/* Aurora enhancements */
int aurora_enable_crypto_enhancement(void);
int aurora_enable_perf_optimization(void);
int aurora_enable_quantum_support(void);
int aurora_apply_enhancement(uint32_t flags);

/* Crystal-Kyber integration */
int linux_compat_kyber_encrypt(const uint8_t* plaintext, size_t length,
                               uint8_t* ciphertext, size_t* out_length);
int linux_compat_kyber_decrypt(const uint8_t* ciphertext, size_t length,
                               uint8_t* plaintext, size_t* out_length);

/* Performance optimizations */
void linux_compat_optimize_memory(void);
void linux_compat_optimize_io(void);
void linux_compat_optimize_scheduling(void);

/* Status and statistics */
const linux_compat_ctx_t* linux_compat_get_context(void);
void linux_compat_print_stats(void);

/* Linux kernel syscall compatibility */
long linux_syscall(long syscall_num, long arg1, long arg2, long arg3, 
                   long arg4, long arg5, long arg6);

/* Individual syscall implementations */
long sys_read(int fd, void* buf, size_t count);
long sys_write(int fd, const void* buf, size_t count);
long sys_open(const char* pathname, int flags, int mode);
long sys_close(int fd);
long sys_stat(const char* pathname, linux_stat_t* statbuf);
long sys_fstat(int fd, linux_stat_t* statbuf);
long sys_lseek(int fd, long offset, int whence);
long sys_mmap(void* addr, size_t length, int prot, int flags, int fd, long offset);
long sys_munmap(void* addr, size_t length);
long sys_brk(void* addr);
long sys_ioctl(int fd, unsigned long request, void* argp);
long sys_pipe(int pipefd[2]);
long sys_dup(int oldfd);
long sys_dup2(int oldfd, int newfd);
long sys_getpid(void);
long sys_getuid(void);
long sys_fork(void);
long sys_execve(const char* pathname, char* const argv[], char* const envp[]);
long sys_wait4(int pid, int* wstatus, int options, void* rusage);
long sys_exit(int status);
long sys_chdir(const char* path);
long sys_getcwd(char* buf, size_t size);
long sys_mkdir(const char* pathname, int mode);
long sys_rmdir(const char* pathname);
long sys_readdir(int fd, linux_dirent_t* dirp, unsigned int count);
long sys_fcntl(int fd, int cmd, long arg);
long sys_socket(int domain, int type, int protocol);
long sys_bind(int sockfd, const void* addr, uint32_t addrlen);
long sys_listen(int sockfd, int backlog);
long sys_accept(int sockfd, void* addr, uint32_t* addrlen);
long sys_connect(int sockfd, const void* addr, uint32_t addrlen);
long sys_sendto(int sockfd, const void* buf, size_t len, int flags,
                const void* dest_addr, uint32_t addrlen);
long sys_recvfrom(int sockfd, void* buf, size_t len, int flags,
                  void* src_addr, uint32_t* addrlen);

#endif /* AURORA_LINUX_COMPAT_H */
