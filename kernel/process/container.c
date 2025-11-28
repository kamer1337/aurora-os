/**
 * Aurora OS - Container/Namespace Support Implementation
 * 
 * Provides Linux-compatible namespace infrastructure for process isolation.
 */

#include "container.h"
#include "../memory/memory.h"
#include "../drivers/timer.h"

/* Container storage */
static container_t* containers[MAX_CONTAINERS];
static uint32_t container_count = 0;
static uint32_t next_container_id = 1;

/* Namespace storage */
static namespace_t* namespaces[NS_TYPE_COUNT][MAX_NS_PER_TYPE];
static uint32_t ns_counts[NS_TYPE_COUNT] = {0};
static uint32_t next_ns_id = 1;

/* Default/init namespace for each type */
static namespace_t init_namespaces[NS_TYPE_COUNT];

/* String comparison helper */
static int ns_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

/* String copy helper */
static void ns_strcpy(char* dest, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/**
 * Initialize container subsystem
 */
int container_subsystem_init(void) {
    /* Initialize container array */
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        containers[i] = NULL;
    }
    
    /* Initialize namespace arrays */
    for (int t = 0; t < NS_TYPE_COUNT; t++) {
        for (int i = 0; i < MAX_NS_PER_TYPE; i++) {
            namespaces[t][i] = NULL;
        }
        ns_counts[t] = 0;
    }
    
    /* Create initial/default namespaces */
    for (int t = 0; t < NS_TYPE_COUNT; t++) {
        init_namespaces[t].id = next_ns_id++;
        init_namespaces[t].type = (ns_type_t)t;
        init_namespaces[t].ref_count = 1;
        init_namespaces[t].owner_pid = 1;  /* init */
        init_namespaces[t].flags = 0;
        init_namespaces[t].parent = NULL;
        
        /* Initialize type-specific data */
        switch (t) {
            case NS_TYPE_UTS:
                ns_strcpy(init_namespaces[t].data.uts.hostname, "aurora", 64);
                ns_strcpy(init_namespaces[t].data.uts.domainname, "(none)", 64);
                break;
            case NS_TYPE_USER:
                init_namespaces[t].data.user.uid_map_count = 0;
                init_namespaces[t].data.user.gid_map_count = 0;
                break;
            case NS_TYPE_PID:
                init_namespaces[t].data.pid.init_pid = 1;
                init_namespaces[t].data.pid.next_pid = 2;
                init_namespaces[t].data.pid.pid_count = 1;
                break;
            case NS_TYPE_NET:
                init_namespaces[t].data.net.loopback_up = 1;
                init_namespaces[t].data.net.veth_count = 0;
                break;
            case NS_TYPE_IPC:
                init_namespaces[t].data.ipc.shm_count = 0;
                init_namespaces[t].data.ipc.sem_count = 0;
                init_namespaces[t].data.ipc.msg_count = 0;
                break;
            case NS_TYPE_MNT:
                ns_strcpy(init_namespaces[t].data.mnt.root_path, "/", 256);
                init_namespaces[t].data.mnt.mount_count = 0;
                break;
            case NS_TYPE_CGROUP:
                ns_strcpy(init_namespaces[t].data.cgroup.cgroup_root, "/sys/fs/cgroup", 256);
                init_namespaces[t].data.cgroup.memory_limit = 0;  /* Unlimited */
                init_namespaces[t].data.cgroup.cpu_shares = 1024;
                init_namespaces[t].data.cgroup.cpu_quota_us = 0;  /* Unlimited */
                init_namespaces[t].data.cgroup.cpu_period_us = 100000;  /* 100ms */
                break;
            default:
                break;
        }
    }
    
    container_count = 0;
    next_container_id = 1;
    
    return NS_SUCCESS;
}

/**
 * Cleanup container subsystem
 */
void container_subsystem_cleanup(void) {
    /* Destroy all containers */
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i]) {
            container_destroy(containers[i]->id);
        }
    }
    
    /* Cleanup namespace arrays */
    for (int t = 0; t < NS_TYPE_COUNT; t++) {
        for (int i = 0; i < MAX_NS_PER_TYPE; i++) {
            if (namespaces[t][i]) {
                kfree(namespaces[t][i]);
                namespaces[t][i] = NULL;
            }
        }
    }
}

/**
 * Create a new namespace
 */
int ns_create(ns_type_t type, uint32_t owner_pid, namespace_t** out_ns) {
    if (type >= NS_TYPE_COUNT || !out_ns) {
        return NS_INVALID_PARAM;
    }
    
    if (ns_counts[type] >= MAX_NS_PER_TYPE) {
        return NS_LIMIT_REACHED;
    }
    
    namespace_t* ns = (namespace_t*)kmalloc(sizeof(namespace_t));
    if (!ns) {
        return NS_ERROR;
    }
    
    ns->id = next_ns_id++;
    ns->type = type;
    ns->ref_count = 1;
    ns->owner_pid = owner_pid;
    ns->flags = 0;
    ns->parent = &init_namespaces[type];
    
    /* Initialize type-specific data from parent */
    switch (type) {
        case NS_TYPE_UTS:
            ns_strcpy(ns->data.uts.hostname, init_namespaces[type].data.uts.hostname, 64);
            ns_strcpy(ns->data.uts.domainname, init_namespaces[type].data.uts.domainname, 64);
            break;
        case NS_TYPE_USER:
            ns->data.user.uid_map_count = 0;
            ns->data.user.gid_map_count = 0;
            break;
        case NS_TYPE_PID:
            ns->data.pid.init_pid = 0;  /* Will be set when first process joins */
            ns->data.pid.next_pid = 1;
            ns->data.pid.pid_count = 0;
            break;
        case NS_TYPE_NET:
            ns->data.net.loopback_up = 0;
            ns->data.net.veth_count = 0;
            ns->data.net.ip_addr[0] = 0;
            ns->data.net.ip_addr[1] = 0;
            ns->data.net.ip_addr[2] = 0;
            ns->data.net.ip_addr[3] = 0;
            break;
        case NS_TYPE_IPC:
            ns->data.ipc.shm_count = 0;
            ns->data.ipc.sem_count = 0;
            ns->data.ipc.msg_count = 0;
            break;
        case NS_TYPE_MNT:
            ns_strcpy(ns->data.mnt.root_path, "/", 256);
            ns->data.mnt.mount_count = 0;
            break;
        case NS_TYPE_CGROUP:
            ns_strcpy(ns->data.cgroup.cgroup_root, "/sys/fs/cgroup", 256);
            ns->data.cgroup.memory_limit = 0;
            ns->data.cgroup.cpu_shares = 1024;
            ns->data.cgroup.cpu_quota_us = 0;
            ns->data.cgroup.cpu_period_us = 100000;
            break;
        default:
            break;
    }
    
    /* Add to namespace array */
    for (int i = 0; i < MAX_NS_PER_TYPE; i++) {
        if (!namespaces[type][i]) {
            namespaces[type][i] = ns;
            ns_counts[type]++;
            break;
        }
    }
    
    *out_ns = ns;
    return NS_SUCCESS;
}

/**
 * Destroy a namespace
 */
int ns_destroy(namespace_t* ns) {
    if (!ns) {
        return NS_INVALID_PARAM;
    }
    
    ns->ref_count--;
    if (ns->ref_count > 0) {
        return NS_SUCCESS;  /* Still referenced */
    }
    
    /* Remove from namespace array */
    for (int i = 0; i < MAX_NS_PER_TYPE; i++) {
        if (namespaces[ns->type][i] == ns) {
            namespaces[ns->type][i] = NULL;
            ns_counts[ns->type]--;
            break;
        }
    }
    
    kfree(ns);
    return NS_SUCCESS;
}

/**
 * Create a new container
 */
int container_create(container_config_t* config, container_t** out_container) {
    if (!config || !out_container) {
        return NS_INVALID_PARAM;
    }
    
    if (container_count >= MAX_CONTAINERS) {
        return NS_LIMIT_REACHED;
    }
    
    container_t* container = (container_t*)kmalloc(sizeof(container_t));
    if (!container) {
        return NS_ERROR;
    }
    
    container->id = next_container_id++;
    ns_strcpy(container->name, config->name, 64);
    container->status = CONTAINER_CREATED;
    container->init_pid = 0;
    container->owner_uid = 0;
    container->ns_flags = config->ns_flags;
    container->created_time = timer_get_ticks();
    container->cpu_usage = 0;
    container->memory_usage = 0;
    
    /* Create requested namespaces */
    for (int t = 0; t < NS_TYPE_COUNT; t++) {
        container->namespaces[t] = NULL;
    }
    
    if (config->ns_flags & CLONE_NEWNS) {
        ns_create(NS_TYPE_MNT, 0, &container->namespaces[NS_TYPE_MNT]);
        if (container->namespaces[NS_TYPE_MNT] && config->root_path[0]) {
            ns_strcpy(container->namespaces[NS_TYPE_MNT]->data.mnt.root_path, 
                     config->root_path, 256);
        }
    }
    
    if (config->ns_flags & CLONE_NEWUTS) {
        ns_create(NS_TYPE_UTS, 0, &container->namespaces[NS_TYPE_UTS]);
        if (container->namespaces[NS_TYPE_UTS] && config->hostname[0]) {
            ns_strcpy(container->namespaces[NS_TYPE_UTS]->data.uts.hostname, 
                     config->hostname, 64);
        }
    }
    
    if (config->ns_flags & CLONE_NEWIPC) {
        ns_create(NS_TYPE_IPC, 0, &container->namespaces[NS_TYPE_IPC]);
    }
    
    if (config->ns_flags & CLONE_NEWUSER) {
        ns_create(NS_TYPE_USER, 0, &container->namespaces[NS_TYPE_USER]);
    }
    
    if (config->ns_flags & CLONE_NEWPID) {
        ns_create(NS_TYPE_PID, 0, &container->namespaces[NS_TYPE_PID]);
    }
    
    if (config->ns_flags & CLONE_NEWNET) {
        ns_create(NS_TYPE_NET, 0, &container->namespaces[NS_TYPE_NET]);
        /* Setup loopback by default */
        if (container->namespaces[NS_TYPE_NET]) {
            net_ns_setup_loopback(container->namespaces[NS_TYPE_NET]);
        }
    }
    
    if (config->ns_flags & CLONE_NEWCGROUP) {
        ns_create(NS_TYPE_CGROUP, 0, &container->namespaces[NS_TYPE_CGROUP]);
        if (container->namespaces[NS_TYPE_CGROUP]) {
            if (config->memory_limit > 0) {
                container->namespaces[NS_TYPE_CGROUP]->data.cgroup.memory_limit = 
                    config->memory_limit;
            }
            if (config->cpu_shares > 0) {
                container->namespaces[NS_TYPE_CGROUP]->data.cgroup.cpu_shares = 
                    config->cpu_shares;
            }
        }
    }
    
    /* Add to container array */
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (!containers[i]) {
            containers[i] = container;
            container_count++;
            break;
        }
    }
    
    *out_container = container;
    return NS_SUCCESS;
}

/**
 * Start a container
 */
int container_start(uint32_t container_id) {
    container_t* container = container_find(container_id);
    if (!container) {
        return NS_NOT_FOUND;
    }
    
    if (container->status != CONTAINER_CREATED && 
        container->status != CONTAINER_STOPPED) {
        return NS_ERROR;
    }
    
    container->status = CONTAINER_RUNNING;
    return NS_SUCCESS;
}

/**
 * Stop a container
 */
int container_stop(uint32_t container_id) {
    container_t* container = container_find(container_id);
    if (!container) {
        return NS_NOT_FOUND;
    }
    
    if (container->status != CONTAINER_RUNNING && 
        container->status != CONTAINER_PAUSED) {
        return NS_ERROR;
    }
    
    container->status = CONTAINER_STOPPED;
    return NS_SUCCESS;
}

/**
 * Pause a container
 */
int container_pause(uint32_t container_id) {
    container_t* container = container_find(container_id);
    if (!container) {
        return NS_NOT_FOUND;
    }
    
    if (container->status != CONTAINER_RUNNING) {
        return NS_ERROR;
    }
    
    container->status = CONTAINER_PAUSED;
    return NS_SUCCESS;
}

/**
 * Resume a paused container
 */
int container_resume(uint32_t container_id) {
    container_t* container = container_find(container_id);
    if (!container) {
        return NS_NOT_FOUND;
    }
    
    if (container->status != CONTAINER_PAUSED) {
        return NS_ERROR;
    }
    
    container->status = CONTAINER_RUNNING;
    return NS_SUCCESS;
}

/**
 * Destroy a container
 */
int container_destroy(uint32_t container_id) {
    container_t* container = NULL;
    int index = -1;
    
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i] && containers[i]->id == container_id) {
            container = containers[i];
            index = i;
            break;
        }
    }
    
    if (!container) {
        return NS_NOT_FOUND;
    }
    
    /* Destroy namespaces */
    for (int t = 0; t < NS_TYPE_COUNT; t++) {
        if (container->namespaces[t]) {
            ns_destroy(container->namespaces[t]);
        }
    }
    
    /* Remove from array */
    containers[index] = NULL;
    container_count--;
    
    kfree(container);
    return NS_SUCCESS;
}

/**
 * Find container by ID
 */
container_t* container_find(uint32_t container_id) {
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i] && containers[i]->id == container_id) {
            return containers[i];
        }
    }
    return NULL;
}

/**
 * Find container by name
 */
container_t* container_find_by_name(const char* name) {
    if (!name) return NULL;
    
    for (int i = 0; i < MAX_CONTAINERS; i++) {
        if (containers[i] && ns_strcmp(containers[i]->name, name) == 0) {
            return containers[i];
        }
    }
    return NULL;
}

/**
 * Get container status
 */
int container_get_status(uint32_t container_id, container_status_t* status) {
    if (!status) {
        return NS_INVALID_PARAM;
    }
    
    container_t* container = container_find(container_id);
    if (!container) {
        return NS_NOT_FOUND;
    }
    
    *status = container->status;
    return NS_SUCCESS;
}

/**
 * List all container IDs
 */
int container_list(uint32_t* ids, uint32_t max_count, uint32_t* actual_count) {
    if (!ids || !actual_count) {
        return NS_INVALID_PARAM;
    }
    
    uint32_t count = 0;
    for (int i = 0; i < MAX_CONTAINERS && count < max_count; i++) {
        if (containers[i]) {
            ids[count++] = containers[i]->id;
        }
    }
    
    *actual_count = count;
    return NS_SUCCESS;
}

/**
 * Set hostname in UTS namespace
 */
int uts_set_hostname(namespace_t* ns, const char* hostname) {
    if (!ns || !hostname || ns->type != NS_TYPE_UTS) {
        return NS_INVALID_PARAM;
    }
    
    ns_strcpy(ns->data.uts.hostname, hostname, 64);
    return NS_SUCCESS;
}

/**
 * Get hostname from UTS namespace
 */
int uts_get_hostname(namespace_t* ns, char* hostname, size_t len) {
    if (!ns || !hostname || ns->type != NS_TYPE_UTS) {
        return NS_INVALID_PARAM;
    }
    
    ns_strcpy(hostname, ns->data.uts.hostname, len);
    return NS_SUCCESS;
}

/**
 * Set domainname in UTS namespace
 */
int uts_set_domainname(namespace_t* ns, const char* domainname) {
    if (!ns || !domainname || ns->type != NS_TYPE_UTS) {
        return NS_INVALID_PARAM;
    }
    
    ns_strcpy(ns->data.uts.domainname, domainname, 64);
    return NS_SUCCESS;
}

/**
 * Set UID mapping in user namespace
 */
int user_ns_set_uid_map(namespace_t* ns, uint32_t inside, uint32_t outside, uint32_t count) {
    if (!ns || ns->type != NS_TYPE_USER) {
        return NS_INVALID_PARAM;
    }
    
    if (ns->data.user.uid_map_count >= 32) {
        return NS_LIMIT_REACHED;
    }
    
    uint32_t idx = ns->data.user.uid_map_count;
    ns->data.user.uid_map[idx][0] = inside;
    ns->data.user.uid_map[idx][1] = outside;
    ns->data.user.uid_map[idx][2] = count;
    ns->data.user.uid_map_count++;
    
    return NS_SUCCESS;
}

/**
 * Set GID mapping in user namespace
 */
int user_ns_set_gid_map(namespace_t* ns, uint32_t inside, uint32_t outside, uint32_t count) {
    if (!ns || ns->type != NS_TYPE_USER) {
        return NS_INVALID_PARAM;
    }
    
    if (ns->data.user.gid_map_count >= 32) {
        return NS_LIMIT_REACHED;
    }
    
    uint32_t idx = ns->data.user.gid_map_count;
    ns->data.user.gid_map[idx][0] = inside;
    ns->data.user.gid_map[idx][1] = outside;
    ns->data.user.gid_map[idx][2] = count;
    ns->data.user.gid_map_count++;
    
    return NS_SUCCESS;
}

/**
 * Translate UID using namespace mapping
 */
int user_ns_translate_uid(namespace_t* ns, uint32_t uid, uint32_t* translated) {
    if (!ns || !translated || ns->type != NS_TYPE_USER) {
        return NS_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < ns->data.user.uid_map_count; i++) {
        uint32_t inside = ns->data.user.uid_map[i][0];
        uint32_t outside = ns->data.user.uid_map[i][1];
        uint32_t count = ns->data.user.uid_map[i][2];
        
        if (uid >= inside && uid < inside + count) {
            *translated = outside + (uid - inside);
            return NS_SUCCESS;
        }
    }
    
    /* No mapping found, return overflow UID */
    *translated = 65534;
    return NS_NOT_FOUND;
}

/**
 * Translate GID using namespace mapping
 */
int user_ns_translate_gid(namespace_t* ns, uint32_t gid, uint32_t* translated) {
    if (!ns || !translated || ns->type != NS_TYPE_USER) {
        return NS_INVALID_PARAM;
    }
    
    for (uint32_t i = 0; i < ns->data.user.gid_map_count; i++) {
        uint32_t inside = ns->data.user.gid_map[i][0];
        uint32_t outside = ns->data.user.gid_map[i][1];
        uint32_t count = ns->data.user.gid_map[i][2];
        
        if (gid >= inside && gid < inside + count) {
            *translated = outside + (gid - inside);
            return NS_SUCCESS;
        }
    }
    
    /* No mapping found, return overflow GID */
    *translated = 65534;
    return NS_NOT_FOUND;
}

/**
 * Allocate PID in PID namespace
 */
int pid_ns_alloc_pid(namespace_t* ns) {
    if (!ns || ns->type != NS_TYPE_PID) {
        return -1;
    }
    
    int pid = (int)ns->data.pid.next_pid++;
    ns->data.pid.pid_count++;
    
    /* Set init PID if this is the first process */
    if (ns->data.pid.init_pid == 0) {
        ns->data.pid.init_pid = (uint32_t)pid;
    }
    
    return pid;
}

/**
 * Free PID in PID namespace
 */
int pid_ns_free_pid(namespace_t* ns, uint32_t pid) {
    if (!ns || ns->type != NS_TYPE_PID) {
        return NS_INVALID_PARAM;
    }
    
    (void)pid;  /* PIDs are not reused in this simple implementation */
    ns->data.pid.pid_count--;
    
    return NS_SUCCESS;
}

/**
 * Setup loopback interface in network namespace
 */
int net_ns_setup_loopback(namespace_t* ns) {
    if (!ns || ns->type != NS_TYPE_NET) {
        return NS_INVALID_PARAM;
    }
    
    ns->data.net.loopback_up = 1;
    ns->data.net.ip_addr[0] = 127;
    ns->data.net.ip_addr[1] = 0;
    ns->data.net.ip_addr[2] = 0;
    ns->data.net.ip_addr[3] = 1;
    
    return NS_SUCCESS;
}

/**
 * Add veth interface to network namespace
 */
int net_ns_add_veth(namespace_t* ns, const char* name) {
    if (!ns || !name || ns->type != NS_TYPE_NET) {
        return NS_INVALID_PARAM;
    }
    
    (void)name;  /* In real implementation, would track interface names */
    ns->data.net.veth_count++;
    
    return NS_SUCCESS;
}

/**
 * Set IP address in network namespace
 */
int net_ns_set_ip(namespace_t* ns, uint8_t ip[4], uint8_t mask[4]) {
    if (!ns || !ip || !mask || ns->type != NS_TYPE_NET) {
        return NS_INVALID_PARAM;
    }
    
    for (int i = 0; i < 4; i++) {
        ns->data.net.ip_addr[i] = ip[i];
        ns->data.net.netmask[i] = mask[i];
    }
    
    return NS_SUCCESS;
}

/**
 * Set memory limit for container cgroup
 */
int cgroup_set_memory_limit(container_t* container, uint64_t limit) {
    if (!container || !container->namespaces[NS_TYPE_CGROUP]) {
        return NS_INVALID_PARAM;
    }
    
    container->namespaces[NS_TYPE_CGROUP]->data.cgroup.memory_limit = limit;
    return NS_SUCCESS;
}

/**
 * Set CPU quota for container cgroup
 */
int cgroup_set_cpu_quota(container_t* container, uint64_t quota_us, uint64_t period_us) {
    if (!container || !container->namespaces[NS_TYPE_CGROUP]) {
        return NS_INVALID_PARAM;
    }
    
    container->namespaces[NS_TYPE_CGROUP]->data.cgroup.cpu_quota_us = quota_us;
    container->namespaces[NS_TYPE_CGROUP]->data.cgroup.cpu_period_us = period_us;
    return NS_SUCCESS;
}

/**
 * Get memory usage for container
 */
int cgroup_get_memory_usage(container_t* container, uint64_t* usage) {
    if (!container || !usage) {
        return NS_INVALID_PARAM;
    }
    
    *usage = container->memory_usage;
    return NS_SUCCESS;
}

/**
 * Get CPU usage for container
 */
int cgroup_get_cpu_usage(container_t* container, uint64_t* usage) {
    if (!container || !usage) {
        return NS_INVALID_PARAM;
    }
    
    *usage = container->cpu_usage;
    return NS_SUCCESS;
}

/**
 * Unshare namespaces for a process
 */
int process_unshare(uint32_t pid, uint32_t ns_flags) {
    (void)pid;
    (void)ns_flags;
    /* Would create new namespaces and move process to them */
    return NS_SUCCESS;
}

/**
 * Enter an existing namespace
 */
int ns_enter(uint32_t pid, namespace_t* ns) {
    if (!ns) {
        return NS_INVALID_PARAM;
    }
    
    (void)pid;
    ns->ref_count++;
    
    return NS_SUCCESS;
}

/**
 * Exit a namespace
 */
int ns_exit(uint32_t pid, ns_type_t type) {
    (void)pid;
    (void)type;
    /* Would decrement ref count and possibly destroy namespace */
    return NS_SUCCESS;
}
