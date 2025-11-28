/**
 * Aurora OS - Container/Namespace Support Header
 * 
 * Provides Linux-compatible namespace infrastructure for process isolation.
 * Supports PID, mount, network, user, UTS, IPC, and cgroup namespaces.
 */

#ifndef AURORA_CONTAINER_H
#define AURORA_CONTAINER_H

#include <stdint.h>
#include <stddef.h>

/* Namespace types (compatible with Linux clone flags) */
#define CLONE_NEWNS     0x00020000  /* New mount namespace */
#define CLONE_NEWUTS    0x04000000  /* New UTS namespace (hostname) */
#define CLONE_NEWIPC    0x08000000  /* New IPC namespace */
#define CLONE_NEWUSER   0x10000000  /* New user namespace */
#define CLONE_NEWPID    0x20000000  /* New PID namespace */
#define CLONE_NEWNET    0x40000000  /* New network namespace */
#define CLONE_NEWCGROUP 0x02000000  /* New cgroup namespace */

/* Maximum containers */
#define MAX_CONTAINERS 64
#define MAX_NS_PER_TYPE 128

/* Namespace status codes */
#define NS_SUCCESS          0
#define NS_ERROR           -1
#define NS_INVALID_PARAM   -2
#define NS_NOT_FOUND       -3
#define NS_LIMIT_REACHED   -4
#define NS_PERMISSION      -5

/* Container status */
typedef enum {
    CONTAINER_CREATED,
    CONTAINER_RUNNING,
    CONTAINER_PAUSED,
    CONTAINER_STOPPED
} container_status_t;

/* Namespace types enumeration */
typedef enum {
    NS_TYPE_MNT = 0,
    NS_TYPE_UTS,
    NS_TYPE_IPC,
    NS_TYPE_USER,
    NS_TYPE_PID,
    NS_TYPE_NET,
    NS_TYPE_CGROUP,
    NS_TYPE_COUNT
} ns_type_t;

/* UTS namespace data */
typedef struct {
    char hostname[64];
    char domainname[64];
} uts_ns_data_t;

/* User namespace data */
typedef struct {
    uint32_t uid_map[32][3];   /* Inside UID, Outside UID, Count */
    uint32_t gid_map[32][3];   /* Inside GID, Outside GID, Count */
    uint32_t uid_map_count;
    uint32_t gid_map_count;
} user_ns_data_t;

/* PID namespace data */
typedef struct {
    uint32_t init_pid;         /* PID 1 in this namespace */
    uint32_t next_pid;         /* Next PID to allocate */
    uint32_t pid_count;        /* Number of processes */
} pid_ns_data_t;

/* Network namespace data */
typedef struct {
    uint32_t loopback_up;      /* Is loopback interface up */
    uint32_t veth_count;       /* Number of veth interfaces */
    uint8_t  ip_addr[4];       /* Container IP address */
    uint8_t  netmask[4];       /* Network mask */
    uint8_t  gateway[4];       /* Gateway address */
} net_ns_data_t;

/* IPC namespace data */
typedef struct {
    uint32_t shm_count;        /* Shared memory segments */
    uint32_t sem_count;        /* Semaphore sets */
    uint32_t msg_count;        /* Message queues */
} ipc_ns_data_t;

/* Mount namespace data */
typedef struct {
    char root_path[256];       /* Root filesystem path */
    uint32_t mount_count;      /* Number of mounts */
} mnt_ns_data_t;

/* Cgroup namespace data */
typedef struct {
    char cgroup_root[256];     /* Cgroup root path */
    uint64_t memory_limit;     /* Memory limit in bytes */
    uint64_t cpu_shares;       /* CPU shares */
    uint64_t cpu_quota_us;     /* CPU quota in microseconds */
    uint64_t cpu_period_us;    /* CPU period in microseconds */
} cgroup_ns_data_t;

/* Generic namespace structure */
typedef struct namespace {
    uint32_t id;               /* Unique namespace ID */
    ns_type_t type;            /* Namespace type */
    uint32_t ref_count;        /* Reference count */
    uint32_t owner_pid;        /* PID of namespace creator */
    uint32_t flags;            /* Namespace flags */
    union {
        uts_ns_data_t uts;
        user_ns_data_t user;
        pid_ns_data_t pid;
        net_ns_data_t net;
        ipc_ns_data_t ipc;
        mnt_ns_data_t mnt;
        cgroup_ns_data_t cgroup;
    } data;
    struct namespace* parent;  /* Parent namespace */
} namespace_t;

/* Container structure */
typedef struct container {
    uint32_t id;               /* Container ID */
    char name[64];             /* Container name */
    container_status_t status; /* Current status */
    uint32_t init_pid;         /* Init process PID */
    uint32_t owner_uid;        /* Owner user ID */
    namespace_t* namespaces[NS_TYPE_COUNT];  /* Namespaces */
    uint32_t ns_flags;         /* Active namespace flags */
    uint64_t created_time;     /* Creation timestamp */
    uint64_t cpu_usage;        /* CPU usage in nanoseconds */
    uint64_t memory_usage;     /* Memory usage in bytes */
} container_t;

/* Container configuration */
typedef struct {
    char name[64];
    uint32_t ns_flags;         /* Which namespaces to create */
    char hostname[64];         /* UTS hostname */
    char root_path[256];       /* Root filesystem */
    uint64_t memory_limit;     /* Memory limit */
    uint64_t cpu_shares;       /* CPU shares */
} container_config_t;

/* Initialization and cleanup */
int container_subsystem_init(void);
void container_subsystem_cleanup(void);

/* Container management */
int container_create(container_config_t* config, container_t** out_container);
int container_start(uint32_t container_id);
int container_stop(uint32_t container_id);
int container_pause(uint32_t container_id);
int container_resume(uint32_t container_id);
int container_destroy(uint32_t container_id);

/* Container information */
container_t* container_find(uint32_t container_id);
container_t* container_find_by_name(const char* name);
int container_get_status(uint32_t container_id, container_status_t* status);
int container_list(uint32_t* ids, uint32_t max_count, uint32_t* actual_count);

/* Namespace management */
int ns_create(ns_type_t type, uint32_t owner_pid, namespace_t** out_ns);
int ns_destroy(namespace_t* ns);
int ns_enter(uint32_t pid, namespace_t* ns);
int ns_exit(uint32_t pid, ns_type_t type);
namespace_t* ns_get(uint32_t pid, ns_type_t type);
int ns_clone(namespace_t* src, namespace_t** out_ns);

/* Process namespace operations */
int process_unshare(uint32_t pid, uint32_t ns_flags);
int process_setns(uint32_t pid, int fd, int ns_type);
int process_get_namespaces(uint32_t pid, namespace_t** ns_array);

/* UTS namespace operations */
int uts_set_hostname(namespace_t* ns, const char* hostname);
int uts_get_hostname(namespace_t* ns, char* hostname, size_t len);
int uts_set_domainname(namespace_t* ns, const char* domainname);

/* User namespace operations */
int user_ns_set_uid_map(namespace_t* ns, uint32_t inside, uint32_t outside, uint32_t count);
int user_ns_set_gid_map(namespace_t* ns, uint32_t inside, uint32_t outside, uint32_t count);
int user_ns_translate_uid(namespace_t* ns, uint32_t uid, uint32_t* translated);
int user_ns_translate_gid(namespace_t* ns, uint32_t gid, uint32_t* translated);

/* PID namespace operations */
int pid_ns_alloc_pid(namespace_t* ns);
int pid_ns_free_pid(namespace_t* ns, uint32_t pid);
int pid_ns_translate(namespace_t* ns, uint32_t local_pid, uint32_t* global_pid);

/* Network namespace operations */
int net_ns_setup_loopback(namespace_t* ns);
int net_ns_add_veth(namespace_t* ns, const char* name);
int net_ns_set_ip(namespace_t* ns, uint8_t ip[4], uint8_t mask[4]);

/* Cgroup operations */
int cgroup_set_memory_limit(container_t* container, uint64_t limit);
int cgroup_set_cpu_quota(container_t* container, uint64_t quota_us, uint64_t period_us);
int cgroup_get_memory_usage(container_t* container, uint64_t* usage);
int cgroup_get_cpu_usage(container_t* container, uint64_t* usage);

#endif /* AURORA_CONTAINER_H */
