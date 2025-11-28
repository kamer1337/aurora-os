/**
 * Aurora OS - Linux Security Module (LSM) Framework Implementation
 * 
 * Provides LSM-style security hooks and capability-based access control.
 */

#include "lsm.h"
#include "../memory/memory.h"
#include "../drivers/vga.h"

/* Registered security modules */
static lsm_module_t* lsm_modules[LSM_MAX_MODULES];
static uint32_t lsm_module_count = 0;
static uint32_t next_module_id = 1;

/* Hook chains for each hook type */
static lsm_hook_t* hook_chains[LSM_HOOK_COUNT];

/* Audit state */
static int audit_state = 0;

/* String helper */
static int lsm_strcmp(const char* s1, const char* s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void lsm_strcpy(char* dest, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src[i]; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/**
 * Initialize LSM framework
 */
int lsm_init(void) {
    /* Initialize module array */
    for (int i = 0; i < LSM_MAX_MODULES; i++) {
        lsm_modules[i] = NULL;
    }
    
    /* Initialize hook chains */
    for (int i = 0; i < LSM_HOOK_COUNT; i++) {
        hook_chains[i] = NULL;
    }
    
    lsm_module_count = 0;
    next_module_id = 1;
    audit_state = 0;
    
    /* Initialize built-in capability module */
    capability_module_init();
    
    return LSM_ALLOW;
}

/**
 * Cleanup LSM framework
 */
void lsm_cleanup(void) {
    /* Unregister all modules */
    for (int i = 0; i < LSM_MAX_MODULES; i++) {
        if (lsm_modules[i]) {
            lsm_unregister_module(lsm_modules[i]->name);
        }
    }
    
    /* Free hook chains */
    for (int i = 0; i < LSM_HOOK_COUNT; i++) {
        lsm_hook_t* hook = hook_chains[i];
        while (hook) {
            lsm_hook_t* next = hook->next;
            kfree(hook);
            hook = next;
        }
        hook_chains[i] = NULL;
    }
}

/**
 * Register a security module
 */
int lsm_register_module(lsm_module_t* module) {
    if (!module || !module->name[0]) {
        return LSM_ERROR;
    }
    
    /* Check if module already registered */
    if (lsm_find_module(module->name)) {
        return LSM_ERROR;
    }
    
    if (lsm_module_count >= LSM_MAX_MODULES) {
        return LSM_ERROR;
    }
    
    /* Find free slot */
    for (int i = 0; i < LSM_MAX_MODULES; i++) {
        if (!lsm_modules[i]) {
            module->id = next_module_id++;
            module->enabled = 1;
            lsm_modules[i] = module;
            lsm_module_count++;
            
            /* Register module hooks */
            for (int h = 0; h < LSM_HOOK_COUNT; h++) {
                if (module->hooks[h]) {
                    lsm_register_hook(module, (lsm_hook_type_t)h, module->hooks[h]);
                }
            }
            
            /* Call module init */
            if (module->init) {
                module->init();
            }
            
            return LSM_ALLOW;
        }
    }
    
    return LSM_ERROR;
}

/**
 * Unregister a security module
 */
int lsm_unregister_module(const char* name) {
    if (!name) {
        return LSM_ERROR;
    }
    
    for (int i = 0; i < LSM_MAX_MODULES; i++) {
        if (lsm_modules[i] && lsm_strcmp(lsm_modules[i]->name, name) == 0) {
            lsm_module_t* module = lsm_modules[i];
            
            /* Unregister all hooks */
            for (int h = 0; h < LSM_HOOK_COUNT; h++) {
                lsm_unregister_hook(module, (lsm_hook_type_t)h);
            }
            
            /* Call module cleanup */
            if (module->cleanup) {
                module->cleanup();
            }
            
            lsm_modules[i] = NULL;
            lsm_module_count--;
            
            return LSM_ALLOW;
        }
    }
    
    return LSM_ERROR;
}

/**
 * Find a registered module by name
 */
lsm_module_t* lsm_find_module(const char* name) {
    if (!name) {
        return NULL;
    }
    
    for (int i = 0; i < LSM_MAX_MODULES; i++) {
        if (lsm_modules[i] && lsm_strcmp(lsm_modules[i]->name, name) == 0) {
            return lsm_modules[i];
        }
    }
    
    return NULL;
}

/**
 * Enable a security module
 */
int lsm_enable_module(const char* name) {
    lsm_module_t* module = lsm_find_module(name);
    if (!module) {
        return LSM_ERROR;
    }
    
    module->enabled = 1;
    return LSM_ALLOW;
}

/**
 * Disable a security module
 */
int lsm_disable_module(const char* name) {
    lsm_module_t* module = lsm_find_module(name);
    if (!module) {
        return LSM_ERROR;
    }
    
    module->enabled = 0;
    return LSM_ALLOW;
}

/**
 * Register a hook for a module
 */
int lsm_register_hook(lsm_module_t* module, lsm_hook_type_t type, lsm_hook_fn_t fn) {
    if (!module || !fn || type >= LSM_HOOK_COUNT) {
        return LSM_ERROR;
    }
    
    lsm_hook_t* hook = (lsm_hook_t*)kmalloc(sizeof(lsm_hook_t));
    if (!hook) {
        return LSM_ERROR;
    }
    
    hook->fn = fn;
    hook->module = module;
    hook->next = hook_chains[type];
    hook_chains[type] = hook;
    
    return LSM_ALLOW;
}

/**
 * Unregister a hook for a module
 */
int lsm_unregister_hook(lsm_module_t* module, lsm_hook_type_t type) {
    if (!module || type >= LSM_HOOK_COUNT) {
        return LSM_ERROR;
    }
    
    lsm_hook_t** prev = &hook_chains[type];
    lsm_hook_t* curr = hook_chains[type];
    
    while (curr) {
        if (curr->module == module) {
            *prev = curr->next;
            kfree(curr);
            return LSM_ALLOW;
        }
        prev = &curr->next;
        curr = curr->next;
    }
    
    return LSM_ERROR;
}

/**
 * Call all registered hooks for a type
 * Returns LSM_DENY if any hook denies, LSM_ALLOW otherwise
 */
int lsm_call_hook(lsm_hook_type_t type, void* data) {
    if (type >= LSM_HOOK_COUNT) {
        return LSM_ERROR;
    }
    
    lsm_hook_t* hook = hook_chains[type];
    while (hook) {
        /* Skip disabled modules */
        if (hook->module && !hook->module->enabled) {
            hook = hook->next;
            continue;
        }
        
        int result = hook->fn(data);
        if (result != LSM_ALLOW) {
            return result;
        }
        
        hook = hook->next;
    }
    
    return LSM_ALLOW;
}

/* Capability operations */

/**
 * Initialize a capability set to empty
 */
int cap_init(cap_t* cap) {
    if (!cap) {
        return LSM_ERROR;
    }
    
    for (int i = 0; i < CAP_SET_SIZE; i++) {
        cap->cap[i] = 0;
    }
    
    return LSM_ALLOW;
}

/**
 * Set a capability bit
 */
int cap_set(cap_t* cap, int capability) {
    if (!cap || capability < 0 || capability > CAP_LAST_CAP) {
        return LSM_ERROR;
    }
    
    int word = capability / 32;
    int bit = capability % 32;
    cap->cap[word] |= (1U << bit);
    
    return LSM_ALLOW;
}

/**
 * Clear a capability bit
 */
int cap_clear(cap_t* cap, int capability) {
    if (!cap || capability < 0 || capability > CAP_LAST_CAP) {
        return LSM_ERROR;
    }
    
    int word = capability / 32;
    int bit = capability % 32;
    cap->cap[word] &= ~(1U << bit);
    
    return LSM_ALLOW;
}

/**
 * Check if a capability is set
 */
int cap_isset(const cap_t* cap, int capability) {
    if (!cap || capability < 0 || capability > CAP_LAST_CAP) {
        return 0;
    }
    
    int word = capability / 32;
    int bit = capability % 32;
    
    return (cap->cap[word] & (1U << bit)) ? 1 : 0;
}

/**
 * Clear all capabilities
 */
int cap_clear_all(cap_t* cap) {
    return cap_init(cap);
}

/**
 * Set all capabilities (for root)
 */
int cap_set_full(cap_t* cap) {
    if (!cap) {
        return LSM_ERROR;
    }
    
    for (int i = 0; i < CAP_SET_SIZE; i++) {
        cap->cap[i] = 0xFFFFFFFF;
    }
    
    return LSM_ALLOW;
}

/**
 * Copy capabilities
 */
int cap_copy(cap_t* dest, const cap_t* src) {
    if (!dest || !src) {
        return LSM_ERROR;
    }
    
    for (int i = 0; i < CAP_SET_SIZE; i++) {
        dest->cap[i] = src->cap[i];
    }
    
    return LSM_ALLOW;
}

/**
 * Check if task has capability
 */
int capable(task_security_t* task, int capability) {
    if (!task || capability < 0 || capability > CAP_LAST_CAP) {
        return 0;
    }
    
    /* Root (UID 0) has all capabilities unless restricted */
    if (task->cred.euid == 0) {
        return 1;
    }
    
    /* Check effective capability set */
    return cap_isset(&task->cred.cap_effective, capability);
}

/**
 * Check capability with namespace support
 */
int capable_ns(task_security_t* task, int capability, void* ns) {
    (void)ns;  /* Namespace support not fully implemented */
    return capable(task, capability);
}

/**
 * Initialize credentials
 */
int cred_init(cred_t* cred) {
    if (!cred) {
        return LSM_ERROR;
    }
    
    cred->uid = 0;
    cred->gid = 0;
    cred->euid = 0;
    cred->egid = 0;
    cred->suid = 0;
    cred->sgid = 0;
    cred->fsuid = 0;
    cred->fsgid = 0;
    cred->securebits = 0;
    
    cap_init(&cred->cap_inheritable);
    cap_init(&cred->cap_permitted);
    cap_init(&cred->cap_effective);
    cap_init(&cred->cap_bset);
    cap_init(&cred->cap_ambient);
    
    /* Root gets all capabilities */
    cap_set_full(&cred->cap_permitted);
    cap_set_full(&cred->cap_effective);
    cap_set_full(&cred->cap_bset);
    
    return LSM_ALLOW;
}

/**
 * Set UID in credentials
 */
int cred_set_uid(cred_t* cred, uint32_t uid) {
    if (!cred) {
        return LSM_ERROR;
    }
    
    cred->uid = uid;
    cred->euid = uid;
    cred->suid = uid;
    cred->fsuid = uid;
    
    /* Non-root loses capabilities */
    if (uid != 0) {
        cap_clear_all(&cred->cap_effective);
    }
    
    return LSM_ALLOW;
}

/**
 * Set GID in credentials
 */
int cred_set_gid(cred_t* cred, uint32_t gid) {
    if (!cred) {
        return LSM_ERROR;
    }
    
    cred->gid = gid;
    cred->egid = gid;
    cred->sgid = gid;
    cred->fsgid = gid;
    
    return LSM_ALLOW;
}

/**
 * Copy credentials
 */
int cred_copy(cred_t* dest, const cred_t* src) {
    if (!dest || !src) {
        return LSM_ERROR;
    }
    
    dest->uid = src->uid;
    dest->gid = src->gid;
    dest->euid = src->euid;
    dest->egid = src->egid;
    dest->suid = src->suid;
    dest->sgid = src->sgid;
    dest->fsuid = src->fsuid;
    dest->fsgid = src->fsgid;
    dest->securebits = src->securebits;
    
    cap_copy(&dest->cap_inheritable, &src->cap_inheritable);
    cap_copy(&dest->cap_permitted, &src->cap_permitted);
    cap_copy(&dest->cap_effective, &src->cap_effective);
    cap_copy(&dest->cap_bset, &src->cap_bset);
    cap_copy(&dest->cap_ambient, &src->cap_ambient);
    
    return LSM_ALLOW;
}

/**
 * Initialize task security context
 */
int task_security_init(task_security_t* sec, uint32_t task_id) {
    if (!sec) {
        return LSM_ERROR;
    }
    
    sec->task_id = task_id;
    sec->security_data = NULL;
    sec->audit_context = 0;
    
    cred_init(&sec->cred);
    
    return LSM_ALLOW;
}

/**
 * Cleanup task security context
 */
void task_security_cleanup(task_security_t* sec) {
    if (!sec) {
        return;
    }
    
    if (sec->security_data) {
        kfree(sec->security_data);
        sec->security_data = NULL;
    }
}

/* Security hook implementations */

int security_task_create(task_security_t* task, uint32_t clone_flags) {
    lsm_task_create_data_t data = {
        .task = task,
        .clone_flags = clone_flags
    };
    return lsm_call_hook(LSM_HOOK_TASK_CREATE, &data);
}

int security_task_kill(task_security_t* task, task_security_t* target, int signal) {
    lsm_task_kill_data_t data = {
        .task = task,
        .target = target,
        .signal = signal
    };
    return lsm_call_hook(LSM_HOOK_TASK_KILL, &data);
}

int security_file_open(task_security_t* task, file_security_t* file, int flags) {
    lsm_file_open_data_t data = {
        .task = task,
        .file = file,
        .flags = flags
    };
    return lsm_call_hook(LSM_HOOK_FILE_OPEN, &data);
}

int security_file_read(task_security_t* task, file_security_t* file) {
    lsm_file_open_data_t data = {
        .task = task,
        .file = file,
        .flags = 0
    };
    return lsm_call_hook(LSM_HOOK_FILE_READ, &data);
}

int security_file_write(task_security_t* task, file_security_t* file) {
    lsm_file_open_data_t data = {
        .task = task,
        .file = file,
        .flags = 0
    };
    return lsm_call_hook(LSM_HOOK_FILE_WRITE, &data);
}

int security_file_exec(task_security_t* task, file_security_t* file) {
    lsm_file_open_data_t data = {
        .task = task,
        .file = file,
        .flags = 0
    };
    return lsm_call_hook(LSM_HOOK_FILE_EXEC, &data);
}

int security_file_mmap(task_security_t* task, file_security_t* file,
                       unsigned long prot, unsigned long flags) {
    lsm_file_mmap_data_t data = {
        .task = task,
        .file = file,
        .prot = prot,
        .flags = flags
    };
    return lsm_call_hook(LSM_HOOK_FILE_MMAP, &data);
}

int security_inode_create(task_security_t* task, inode_security_t* dir,
                          const char* name, int mode) {
    lsm_inode_create_data_t data = {
        .task = task,
        .dir = dir,
        .name = name,
        .mode = mode
    };
    return lsm_call_hook(LSM_HOOK_INODE_CREATE, &data);
}

int security_inode_permission(task_security_t* task, inode_security_t* inode, int mask) {
    lsm_inode_permission_data_t data = {
        .task = task,
        .inode = inode,
        .mask = mask
    };
    return lsm_call_hook(LSM_HOOK_INODE_PERMISSION, &data);
}

int security_socket_create(task_security_t* task, int domain, int type, int protocol) {
    socket_security_t sock = {
        .domain = domain,
        .type = type,
        .protocol = protocol,
        .port = 0,
        .security_data = NULL
    };
    lsm_socket_create_data_t data = {
        .task = task,
        .sock = &sock
    };
    return lsm_call_hook(LSM_HOOK_SOCKET_CREATE, &data);
}

int security_socket_bind(task_security_t* task, socket_security_t* sock, uint16_t port) {
    lsm_socket_bind_data_t data = {
        .task = task,
        .sock = sock,
        .port = port
    };
    return lsm_call_hook(LSM_HOOK_SOCKET_BIND, &data);
}

int security_capable(task_security_t* task, int capability, int audit) {
    lsm_capable_data_t data = {
        .task = task,
        .capability = capability,
        .audit = audit
    };
    return lsm_call_hook(LSM_HOOK_CAPABLE, &data);
}

int security_module_load(const char* name, void* image, size_t size) {
    lsm_module_load_data_t data = {
        .module_name = name,
        .module_image = image,
        .module_size = size
    };
    return lsm_call_hook(LSM_HOOK_MODULE_LOAD, &data);
}

int security_ptrace(task_security_t* tracer, task_security_t* tracee) {
    lsm_task_kill_data_t data = {
        .task = tracer,
        .target = tracee,
        .signal = 0
    };
    return lsm_call_hook(LSM_HOOK_PTRACE, &data);
}

/* Audit support */

int audit_enabled(void) {
    return audit_state;
}

void audit_set_enabled(int enabled) {
    audit_state = enabled;
}

int audit_log(const char* fmt, ...) {
    if (!audit_state || !fmt) {
        return LSM_ALLOW;
    }
    
    /* Simple audit log - just print to VGA */
    vga_write("[AUDIT] ");
    vga_write(fmt);
    vga_write("\n");
    
    return LSM_ALLOW;
}

/* Built-in capability security module */

static int cap_hook_capable(void* data) {
    lsm_capable_data_t* cap_data = (lsm_capable_data_t*)data;
    
    if (!cap_data || !cap_data->task) {
        return LSM_DENY;
    }
    
    /* Check if task has the capability */
    if (capable(cap_data->task, cap_data->capability)) {
        if (cap_data->audit && audit_enabled()) {
            audit_log("Capability granted");
        }
        return LSM_ALLOW;
    }
    
    if (cap_data->audit && audit_enabled()) {
        audit_log("Capability denied");
    }
    
    return LSM_DENY;
}

static int cap_hook_task_kill(void* data) {
    lsm_task_kill_data_t* kill_data = (lsm_task_kill_data_t*)data;
    
    if (!kill_data || !kill_data->task || !kill_data->target) {
        return LSM_DENY;
    }
    
    /* Check CAP_KILL or same UID */
    if (capable(kill_data->task, CAP_KILL)) {
        return LSM_ALLOW;
    }
    
    if (kill_data->task->cred.euid == kill_data->target->cred.uid) {
        return LSM_ALLOW;
    }
    
    return LSM_DENY;
}

static int cap_hook_socket_bind(void* data) {
    lsm_socket_bind_data_t* bind_data = (lsm_socket_bind_data_t*)data;
    
    if (!bind_data || !bind_data->task) {
        return LSM_DENY;
    }
    
    /* Privileged ports (< 1024) require CAP_NET_BIND_SERVICE */
    if (bind_data->port < 1024) {
        if (!capable(bind_data->task, CAP_NET_BIND_SERVICE)) {
            return LSM_DENY;
        }
    }
    
    return LSM_ALLOW;
}

static int cap_hook_module_load(void* data) {
    lsm_module_load_data_t* load_data = (lsm_module_load_data_t*)data;
    (void)load_data;
    
    /* Simplified: only allow root to load modules in real implementation */
    return LSM_ALLOW;
}

static lsm_module_t capability_module = {
    .name = "capability",
    .id = 0,
    .enabled = 1,
    .init = NULL,
    .cleanup = NULL,
    .hooks = {0}
};

int capability_module_init(void) {
    /* Set up capability hooks */
    capability_module.hooks[LSM_HOOK_CAPABLE] = cap_hook_capable;
    capability_module.hooks[LSM_HOOK_TASK_KILL] = cap_hook_task_kill;
    capability_module.hooks[LSM_HOOK_SOCKET_BIND] = cap_hook_socket_bind;
    capability_module.hooks[LSM_HOOK_MODULE_LOAD] = cap_hook_module_load;
    
    lsm_strcpy(capability_module.name, "capability", 32);
    
    return lsm_register_module(&capability_module);
}

/* Stub implementations for SELinux and AppArmor compatibility */

int selinux_stub_init(void) {
    /* SELinux stub - would be replaced with full implementation */
    return LSM_ALLOW;
}

int apparmor_stub_init(void) {
    /* AppArmor stub - would be replaced with full implementation */
    return LSM_ALLOW;
}
