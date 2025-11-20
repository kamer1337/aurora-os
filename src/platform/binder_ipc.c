/**
 * @file binder_ipc.c
 * @brief Binder IPC Implementation
 */

#include "../../include/platform/binder_ipc.h"
#include "../../include/platform/platform_util.h"

/* Global Binder state */
static binder_driver_t g_binder_driver;
static service_manager_t g_service_manager;
static bool g_binder_initialized = false;

#define BINDER_VERSION "1.0.0-aurora-binder"

int binder_init(void) {
    if (g_binder_initialized) {
        return 0;
    }
    
    /* Initialize driver */
    platform_memset(&g_binder_driver, 0, sizeof(binder_driver_t));
    g_binder_driver.process_count = 0;
    g_binder_driver.context_mgr = (binder_process_t*)0;
    g_binder_driver.initialized = true;
    
    /* Initialize service manager */
    platform_memset(&g_service_manager, 0, sizeof(service_manager_t));
    g_service_manager.service_count = 0;
    
    g_binder_initialized = true;
    
    return 0;
}

binder_process_t* binder_create_process(uint32_t pid) {
    if (!g_binder_initialized) {
        binder_init();
    }
    
    if (g_binder_driver.process_count >= 256) {
        return (binder_process_t*)0; /* Too many processes */
    }
    
    /* Allocate process state */
    binder_process_t* process = (binder_process_t*)platform_malloc(sizeof(binder_process_t));
    if (!process) {
        return (binder_process_t*)0;
    }
    
    /* Initialize process */
    platform_memset(process, 0, sizeof(binder_process_t));
    process->pid = pid;
    process->nodes = (binder_node_t*)0;
    process->next_handle = 1; /* Handle 0 is reserved for context manager */
    process->thread_count = 0;
    process->context_manager = false;
    
    /* Add to driver */
    g_binder_driver.processes[g_binder_driver.process_count++] = process;
    
    return process;
}

void binder_destroy_process(binder_process_t* process) {
    if (!process) {
        return;
    }
    
    /* Free all nodes */
    binder_node_t* node = process->nodes;
    while (node) {
        binder_node_t* next = node->next;
        platform_free(node);
        node = next;
    }
    
    /* Free all threads */
    for (uint32_t i = 0; i < process->thread_count; i++) {
        if (process->threads[i]) {
            platform_free(process->threads[i]);
        }
    }
    
    /* Remove from driver */
    for (uint32_t i = 0; i < g_binder_driver.process_count; i++) {
        if (g_binder_driver.processes[i] == process) {
            /* Shift remaining processes */
            for (uint32_t j = i; j < g_binder_driver.process_count - 1; j++) {
                g_binder_driver.processes[j] = g_binder_driver.processes[j + 1];
            }
            g_binder_driver.process_count--;
            break;
        }
    }
    
    platform_free(process);
}

binder_thread_t* binder_create_thread(binder_process_t* process, uint32_t tid) {
    if (!process || process->thread_count >= 64) {
        return (binder_thread_t*)0;
    }
    
    /* Allocate thread state */
    binder_thread_t* thread = (binder_thread_t*)platform_malloc(sizeof(binder_thread_t));
    if (!thread) {
        return (binder_thread_t*)0;
    }
    
    /* Initialize thread */
    platform_memset(thread, 0, sizeof(binder_thread_t));
    thread->pid = process->pid;
    thread->tid = tid;
    thread->looper_registered = false;
    thread->looper_entered = false;
    thread->transaction_depth = 0;
    
    /* Add to process */
    process->threads[process->thread_count++] = thread;
    
    return thread;
}

uint32_t binder_new_node(binder_process_t* process, void* ptr, void* cookie) {
    if (!process) {
        return 0;
    }
    
    /* Allocate node */
    binder_node_t* node = (binder_node_t*)platform_malloc(sizeof(binder_node_t));
    if (!node) {
        return 0;
    }
    
    /* Initialize node */
    platform_memset(node, 0, sizeof(binder_node_t));
    node->handle = process->next_handle++;
    node->ptr = ptr;
    node->cookie = cookie;
    node->refs = 1;
    node->weak_refs = 0;
    node->dead = false;
    node->next = process->nodes;
    
    /* Add to process node list */
    process->nodes = node;
    
    return node->handle;
}

binder_node_t* binder_get_node(binder_process_t* process, uint32_t handle) {
    if (!process) {
        return (binder_node_t*)0;
    }
    
    /* Search for node */
    binder_node_t* node = process->nodes;
    while (node) {
        if (node->handle == handle) {
            return node;
        }
        node = node->next;
    }
    
    return (binder_node_t*)0;
}

int binder_inc_ref(binder_node_t* node) {
    if (!node) {
        return -1;
    }
    
    node->refs++;
    return 0;
}

int binder_dec_ref(binder_node_t* node) {
    if (!node || node->refs == 0) {
        return -1;
    }
    
    node->refs--;
    
    /* Mark as dead if no references */
    if (node->refs == 0 && node->weak_refs == 0) {
        node->dead = true;
    }
    
    return 0;
}

int binder_transact(binder_process_t* process, binder_thread_t* thread,
                    binder_transaction_t* transaction) {
    if (!process || !thread || !transaction) {
        return -1;
    }
    
    /* Find target process and node */
    uint32_t target_handle = transaction->target_handle;
    
    /* Handle 0 is context manager */
    if (target_handle == 0) {
        if (!g_binder_driver.context_mgr) {
            return -1; /* No context manager */
        }
        
        /* Route to service manager */
        return 0;
    }
    
    /* Find target node */
    binder_node_t* target_node = binder_get_node(process, target_handle);
    if (!target_node || target_node->dead) {
        return -1; /* Invalid target */
    }
    
    /* In real implementation:
     * 1. Validate transaction
     * 2. Copy data to target process
     * 3. Wake up target thread
     * 4. Wait for reply (if not one-way)
     * 5. Copy reply back
     */
    
    return 0;
}

int binder_reply(binder_thread_t* thread, parcel_t* reply) {
    if (!thread || !reply) {
        return -1;
    }
    
    if (thread->transaction_depth == 0) {
        return -1; /* No pending transaction */
    }
    
    /* Pop transaction from stack */
    thread->transaction_depth--;
    
    /* In real implementation:
     * 1. Find waiting thread
     * 2. Copy reply data
     * 3. Wake up waiting thread
     */
    
    return 0;
}

void parcel_init(parcel_t* parcel) {
    if (!parcel) {
        return;
    }
    
    platform_memset(parcel, 0, sizeof(parcel_t));
    parcel->data_pos = 0;
    parcel->data_size = 0;
    parcel->objects_count = 0;
}

int parcel_write_data(parcel_t* parcel, const void* data, uint32_t size) {
    if (!parcel || !data || size == 0) {
        return -1;
    }
    
    if (parcel->data_size + size > PARCEL_MAX_SIZE) {
        return -1; /* Not enough space */
    }
    
    platform_memcpy(parcel->data + parcel->data_size, data, size);
    parcel->data_size += size;
    
    return 0;
}

int parcel_read_data(parcel_t* parcel, void* data, uint32_t size) {
    if (!parcel || !data || size == 0) {
        return -1;
    }
    
    if (parcel->data_pos + size > parcel->data_size) {
        return -1; /* Not enough data */
    }
    
    platform_memcpy(data, parcel->data + parcel->data_pos, size);
    parcel->data_pos += size;
    
    return 0;
}

int parcel_write_int32(parcel_t* parcel, int32_t value) {
    return parcel_write_data(parcel, &value, sizeof(int32_t));
}

int parcel_read_int32(parcel_t* parcel, int32_t* value) {
    return parcel_read_data(parcel, value, sizeof(int32_t));
}

int parcel_write_string(parcel_t* parcel, const char* str) {
    if (!parcel || !str) {
        return -1;
    }
    
    uint32_t len = platform_strlen(str);
    
    /* Write length */
    if (parcel_write_int32(parcel, (int32_t)len) != 0) {
        return -1;
    }
    
    /* Write string data */
    if (parcel_write_data(parcel, str, len + 1) != 0) {
        return -1;
    }
    
    return 0;
}

int parcel_read_string(parcel_t* parcel, char* str, uint32_t max_size) {
    if (!parcel || !str || max_size == 0) {
        return -1;
    }
    
    /* Read length */
    int32_t len;
    if (parcel_read_int32(parcel, &len) != 0) {
        return -1;
    }
    
    if (len < 0 || (uint32_t)len >= max_size) {
        return -1; /* Invalid length */
    }
    
    /* Read string data */
    if (parcel_read_data(parcel, str, (uint32_t)len + 1) != 0) {
        return -1;
    }
    
    return 0;
}

int parcel_write_binder(parcel_t* parcel, binder_object_t* obj) {
    if (!parcel || !obj) {
        return -1;
    }
    
    /* Record offset */
    if (parcel->objects_count >= 64) {
        return -1; /* Too many objects */
    }
    
    parcel->objects_offsets[parcel->objects_count++] = parcel->data_size;
    
    /* Write binder object */
    return parcel_write_data(parcel, obj, sizeof(binder_object_t));
}

int service_manager_init(void) {
    if (!g_binder_initialized) {
        binder_init();
    }
    
    /* Create context manager process */
    if (!g_binder_driver.context_mgr) {
        g_binder_driver.context_mgr = binder_create_process(0); /* PID 0 for context mgr */
        if (g_binder_driver.context_mgr) {
            g_binder_driver.context_mgr->context_manager = true;
        }
    }
    
    return 0;
}

int service_manager_add_service(const char* name, uint32_t handle) {
    if (!name || handle == 0) {
        return -1;
    }
    
    if (g_service_manager.service_count >= 256) {
        return -1; /* Too many services */
    }
    
    /* Check if service already exists */
    for (uint32_t i = 0; i < g_service_manager.service_count; i++) {
        if (platform_strcmp(g_service_manager.services[i].name, name) == 0) {
            /* Update existing service */
            g_service_manager.services[i].handle = handle;
            return 0;
        }
    }
    
    /* Add new service */
    service_entry_t* entry = &g_service_manager.services[g_service_manager.service_count++];
    platform_strncpy(entry->name, name, sizeof(entry->name));
    entry->handle = handle;
    entry->allow_isolated = false;
    
    return 0;
}

uint32_t service_manager_get_service(const char* name) {
    if (!name) {
        return 0;
    }
    
    /* Search for service */
    for (uint32_t i = 0; i < g_service_manager.service_count; i++) {
        if (platform_strcmp(g_service_manager.services[i].name, name) == 0) {
            return g_service_manager.services[i].handle;
        }
    }
    
    return 0; /* Not found */
}

bool service_manager_check_service(const char* name) {
    return service_manager_get_service(name) != 0;
}

uint32_t service_manager_list_services(char names[][128], uint32_t max_count) {
    if (!names || max_count == 0) {
        return 0;
    }
    
    uint32_t count = 0;
    for (uint32_t i = 0; i < g_service_manager.service_count && count < max_count; i++) {
        platform_strncpy(names[count], g_service_manager.services[i].name, 128);
        count++;
    }
    
    return count;
}

binder_driver_t* binder_get_driver(void) {
    return &g_binder_driver;
}

service_manager_t* binder_get_service_manager(void) {
    return &g_service_manager;
}

const char* binder_get_version(void) {
    return BINDER_VERSION;
}
