/**
 * Aurora OS - Linux Security Module (LSM) Framework Header
 * 
 * Provides LSM-style security hooks and capability-based access control.
 */

#ifndef AURORA_LSM_H
#define AURORA_LSM_H

#include <stdint.h>
#include <stddef.h>

/* LSM return codes */
#define LSM_ALLOW       0
#define LSM_DENY       -1
#define LSM_ERROR      -2

/* Capability bits (Linux compatible) */
#define CAP_CHOWN              0
#define CAP_DAC_OVERRIDE       1
#define CAP_DAC_READ_SEARCH    2
#define CAP_FOWNER             3
#define CAP_FSETID             4
#define CAP_KILL               5
#define CAP_SETGID             6
#define CAP_SETUID             7
#define CAP_SETPCAP            8
#define CAP_LINUX_IMMUTABLE    9
#define CAP_NET_BIND_SERVICE  10
#define CAP_NET_BROADCAST     11
#define CAP_NET_ADMIN         12
#define CAP_NET_RAW           13
#define CAP_IPC_LOCK          14
#define CAP_IPC_OWNER         15
#define CAP_SYS_MODULE        16
#define CAP_SYS_RAWIO         17
#define CAP_SYS_CHROOT        18
#define CAP_SYS_PTRACE        19
#define CAP_SYS_PACCT         20
#define CAP_SYS_ADMIN         21
#define CAP_SYS_BOOT          22
#define CAP_SYS_NICE          23
#define CAP_SYS_RESOURCE      24
#define CAP_SYS_TIME          25
#define CAP_SYS_TTY_CONFIG    26
#define CAP_MKNOD             27
#define CAP_LEASE             28
#define CAP_AUDIT_WRITE       29
#define CAP_AUDIT_CONTROL     30
#define CAP_SETFCAP           31
#define CAP_MAC_OVERRIDE      32
#define CAP_MAC_ADMIN         33
#define CAP_SYSLOG            34
#define CAP_WAKE_ALARM        35
#define CAP_BLOCK_SUSPEND     36
#define CAP_AUDIT_READ        37
#define CAP_LAST_CAP          CAP_AUDIT_READ

/* Capability set size */
#define CAP_SET_SIZE  2  /* 64 bits total */

/* Maximum security modules */
#define LSM_MAX_MODULES   8
#define LSM_MAX_HOOKS    64

/* Hook types */
typedef enum {
    /* Task hooks */
    LSM_HOOK_TASK_CREATE,
    LSM_HOOK_TASK_FREE,
    LSM_HOOK_TASK_SETUID,
    LSM_HOOK_TASK_SETGID,
    LSM_HOOK_TASK_KILL,
    LSM_HOOK_TASK_SETNICE,
    LSM_HOOK_TASK_SETSCHEDULER,
    
    /* File hooks */
    LSM_HOOK_FILE_OPEN,
    LSM_HOOK_FILE_READ,
    LSM_HOOK_FILE_WRITE,
    LSM_HOOK_FILE_EXEC,
    LSM_HOOK_FILE_MMAP,
    LSM_HOOK_FILE_MPROTECT,
    LSM_HOOK_FILE_LOCK,
    LSM_HOOK_FILE_IOCTL,
    
    /* Inode hooks */
    LSM_HOOK_INODE_CREATE,
    LSM_HOOK_INODE_LINK,
    LSM_HOOK_INODE_UNLINK,
    LSM_HOOK_INODE_MKDIR,
    LSM_HOOK_INODE_RMDIR,
    LSM_HOOK_INODE_RENAME,
    LSM_HOOK_INODE_SETATTR,
    LSM_HOOK_INODE_GETATTR,
    LSM_HOOK_INODE_PERMISSION,
    
    /* Socket hooks */
    LSM_HOOK_SOCKET_CREATE,
    LSM_HOOK_SOCKET_BIND,
    LSM_HOOK_SOCKET_CONNECT,
    LSM_HOOK_SOCKET_LISTEN,
    LSM_HOOK_SOCKET_ACCEPT,
    LSM_HOOK_SOCKET_SENDMSG,
    LSM_HOOK_SOCKET_RECVMSG,
    
    /* IPC hooks */
    LSM_HOOK_IPC_PERMISSION,
    LSM_HOOK_MSG_QUEUE_ASSOCIATE,
    LSM_HOOK_SHM_ASSOCIATE,
    LSM_HOOK_SEM_ASSOCIATE,
    
    /* Module hooks */
    LSM_HOOK_MODULE_LOAD,
    LSM_HOOK_MODULE_FREE,
    
    /* System hooks */
    LSM_HOOK_PTRACE,
    LSM_HOOK_QUOTACTL,
    LSM_HOOK_MOUNT,
    LSM_HOOK_UMOUNT,
    LSM_HOOK_SYSCTL,
    
    /* Capability hooks */
    LSM_HOOK_CAPABLE,
    LSM_HOOK_CAPGET,
    LSM_HOOK_CAPSET,
    
    LSM_HOOK_COUNT
} lsm_hook_type_t;

/* Capability set structure */
typedef struct {
    uint32_t cap[CAP_SET_SIZE];
} cap_t;

/* Credential structure */
typedef struct {
    uint32_t uid;
    uint32_t gid;
    uint32_t euid;
    uint32_t egid;
    uint32_t suid;
    uint32_t sgid;
    uint32_t fsuid;
    uint32_t fsgid;
    cap_t cap_inheritable;
    cap_t cap_permitted;
    cap_t cap_effective;
    cap_t cap_bset;           /* Capability bounding set */
    cap_t cap_ambient;        /* Ambient capabilities */
    uint32_t securebits;
} cred_t;

/* Security context for tasks */
typedef struct {
    uint32_t task_id;
    cred_t cred;
    void* security_data;      /* Module-specific data */
    uint32_t audit_context;
} task_security_t;

/* Security context for files */
typedef struct {
    uint32_t inode_id;
    uint32_t mode;
    uint32_t owner_uid;
    uint32_t owner_gid;
    void* security_data;
} file_security_t;

/* Security context for inodes */
typedef struct {
    uint32_t inode_id;
    uint32_t mode;
    uint32_t uid;
    uint32_t gid;
    void* security_data;
} inode_security_t;

/* Security context for sockets */
typedef struct {
    int domain;
    int type;
    int protocol;
    uint16_t port;
    void* security_data;
} socket_security_t;

/* Forward declarations */
struct lsm_module;

/* Hook function type */
typedef int (*lsm_hook_fn_t)(void* data);

/* Hook entry */
typedef struct lsm_hook {
    lsm_hook_fn_t fn;
    struct lsm_module* module;
    struct lsm_hook* next;
} lsm_hook_t;

/* Security module structure */
typedef struct lsm_module {
    char name[32];
    uint32_t id;
    uint8_t enabled;
    int (*init)(void);
    void (*cleanup)(void);
    /* Module-specific hooks */
    lsm_hook_fn_t hooks[LSM_HOOK_COUNT];
} lsm_module_t;

/* Hook data structures */
typedef struct {
    task_security_t* task;
    uint32_t clone_flags;
} lsm_task_create_data_t;

typedef struct {
    task_security_t* task;
    uint32_t new_uid;
} lsm_task_setuid_data_t;

typedef struct {
    task_security_t* task;
    task_security_t* target;
    int signal;
} lsm_task_kill_data_t;

typedef struct {
    task_security_t* task;
    file_security_t* file;
    int flags;
} lsm_file_open_data_t;

typedef struct {
    task_security_t* task;
    file_security_t* file;
    unsigned long prot;
    unsigned long flags;
} lsm_file_mmap_data_t;

typedef struct {
    task_security_t* task;
    inode_security_t* dir;
    const char* name;
    int mode;
} lsm_inode_create_data_t;

typedef struct {
    task_security_t* task;
    inode_security_t* inode;
    int mask;
} lsm_inode_permission_data_t;

typedef struct {
    task_security_t* task;
    socket_security_t* sock;
} lsm_socket_create_data_t;

typedef struct {
    task_security_t* task;
    socket_security_t* sock;
    uint16_t port;
} lsm_socket_bind_data_t;

typedef struct {
    task_security_t* task;
    int capability;
    int audit;
} lsm_capable_data_t;

typedef struct {
    const char* module_name;
    void* module_image;
    size_t module_size;
} lsm_module_load_data_t;

/* Initialization and cleanup */
int lsm_init(void);
void lsm_cleanup(void);

/* Module management */
int lsm_register_module(lsm_module_t* module);
int lsm_unregister_module(const char* name);
lsm_module_t* lsm_find_module(const char* name);
int lsm_enable_module(const char* name);
int lsm_disable_module(const char* name);

/* Hook management */
int lsm_register_hook(lsm_module_t* module, lsm_hook_type_t type, lsm_hook_fn_t fn);
int lsm_unregister_hook(lsm_module_t* module, lsm_hook_type_t type);

/* Hook invocation */
int lsm_call_hook(lsm_hook_type_t type, void* data);

/* Capability operations */
int cap_init(cap_t* cap);
int cap_set(cap_t* cap, int capability);
int cap_clear(cap_t* cap, int capability);
int cap_isset(const cap_t* cap, int capability);
int cap_clear_all(cap_t* cap);
int cap_set_full(cap_t* cap);
int cap_copy(cap_t* dest, const cap_t* src);

/* Capability checking */
int capable(task_security_t* task, int capability);
int capable_ns(task_security_t* task, int capability, void* ns);

/* Credential operations */
int cred_init(cred_t* cred);
int cred_set_uid(cred_t* cred, uint32_t uid);
int cred_set_gid(cred_t* cred, uint32_t gid);
int cred_copy(cred_t* dest, const cred_t* src);

/* Task security operations */
int task_security_init(task_security_t* sec, uint32_t task_id);
void task_security_cleanup(task_security_t* sec);

/* Security hooks (called by kernel) */
int security_task_create(task_security_t* task, uint32_t clone_flags);
int security_task_kill(task_security_t* task, task_security_t* target, int signal);
int security_file_open(task_security_t* task, file_security_t* file, int flags);
int security_file_read(task_security_t* task, file_security_t* file);
int security_file_write(task_security_t* task, file_security_t* file);
int security_file_exec(task_security_t* task, file_security_t* file);
int security_file_mmap(task_security_t* task, file_security_t* file, 
                       unsigned long prot, unsigned long flags);
int security_inode_create(task_security_t* task, inode_security_t* dir, 
                          const char* name, int mode);
int security_inode_permission(task_security_t* task, inode_security_t* inode, int mask);
int security_socket_create(task_security_t* task, int domain, int type, int protocol);
int security_socket_bind(task_security_t* task, socket_security_t* sock, uint16_t port);
int security_capable(task_security_t* task, int capability, int audit);
int security_module_load(const char* name, void* image, size_t size);
int security_ptrace(task_security_t* tracer, task_security_t* tracee);

/* Audit support */
int audit_log(const char* fmt, ...);
int audit_enabled(void);
void audit_set_enabled(int enabled);

/* Built-in security modules */
int capability_module_init(void);
int selinux_stub_init(void);
int apparmor_stub_init(void);

/* Enhanced security policy management */
int lsm_get_module_count(void);
int lsm_list_modules(char* buffer, size_t buffer_size);
int lsm_get_module_status(const char* name, int* enabled, int* hook_count);
int lsm_set_enforcing_mode(const char* module_name, int enforcing);
int lsm_get_enforcing_mode(const char* module_name);

#endif /* AURORA_LSM_H */
