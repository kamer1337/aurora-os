/**
 * Aurora OS - Plugin System API
 * 
 * Defines the plugin interface for external modules that can be loaded
 * during bootloader/kernel initialization to provide optional functionality.
 */

#ifndef AURORA_PLUGIN_H
#define AURORA_PLUGIN_H

#include <stdint.h>
#include <stddef.h>

/* Plugin system version */
#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0

/* Plugin status codes */
#define PLUGIN_SUCCESS 0
#define PLUGIN_ERROR -1
#define PLUGIN_ERROR_INVALID_VERSION -2
#define PLUGIN_ERROR_INIT_FAILED -3
#define PLUGIN_ERROR_NOT_FOUND -4
#define PLUGIN_ERROR_ALREADY_REGISTERED -5
#define PLUGIN_ERROR_VERIFICATION_FAILED -6
#define PLUGIN_ERROR_SECURITY_VIOLATION -7
#define PLUGIN_ERROR_PERMISSION_DENIED -8

/* Maximum plugin name length */
#define PLUGIN_NAME_MAX 64

/* Plugin signature size (for quantum crypto verification) */
#define PLUGIN_SIGNATURE_SIZE 32

/* Plugin types */
typedef enum {
    PLUGIN_TYPE_DRIVER,       /* Hardware driver plugin */
    PLUGIN_TYPE_FILESYSTEM,   /* Filesystem plugin */
    PLUGIN_TYPE_BOOT_SETUP,   /* Boot-time setup plugin */
    PLUGIN_TYPE_DIAGNOSTICS,  /* Diagnostic/testing plugin */
    PLUGIN_TYPE_SECURITY,     /* Security extension plugin */
    PLUGIN_TYPE_NETWORK,      /* Network protocol plugin */
    PLUGIN_TYPE_ML_OPTIMIZATION,  /* Machine learning optimization plugin */
    PLUGIN_TYPE_QUANTUM_COMPUTE,  /* Quantum computing plugin */
    PLUGIN_TYPE_SYSTEM_OPTIMIZATION, /* System optimization plugin */
    PLUGIN_TYPE_OTHER         /* Other/custom plugin type */
} plugin_type_t;

/* Plugin priority (for initialization order) */
typedef enum {
    PLUGIN_PRIORITY_CRITICAL = 0,  /* Must load first */
    PLUGIN_PRIORITY_HIGH = 1,
    PLUGIN_PRIORITY_NORMAL = 2,
    PLUGIN_PRIORITY_LOW = 3,
    PLUGIN_PRIORITY_OPTIONAL = 4    /* Can fail without affecting boot */
} plugin_priority_t;

/* Plugin security permissions */
typedef enum {
    PLUGIN_PERM_NONE = 0x00,
    PLUGIN_PERM_MEMORY = 0x01,       /* Can allocate/free memory */
    PLUGIN_PERM_IO = 0x02,           /* Can perform I/O operations */
    PLUGIN_PERM_CRYPTO = 0x04,       /* Can access crypto functions */
    PLUGIN_PERM_KERNEL = 0x08,       /* Can access kernel internals */
    PLUGIN_PERM_ALL = 0xFF           /* All permissions */
} plugin_permissions_t;

/* Functions that plugin may interfere with */
typedef enum {
    PLUGIN_INTERFERE_NONE = 0x00,
    PLUGIN_INTERFERE_QUANTUM_CRYPTO = 0x01,  /* May affect quantum crypto verification */
    PLUGIN_INTERFERE_MEMORY = 0x02,          /* May affect memory management */
    PLUGIN_INTERFERE_PROCESS = 0x04,         /* May affect process scheduling */
    PLUGIN_INTERFERE_FILESYSTEM = 0x08,      /* May affect filesystem operations */
    PLUGIN_INTERFERE_NETWORK = 0x10,         /* May affect networking */
    PLUGIN_INTERFERE_SECURITY = 0x20         /* May affect security subsystem */
} plugin_interference_t;

/* Forward declaration */
struct plugin_descriptor;

/* Plugin initialization callback */
typedef int (*plugin_init_func_t)(struct plugin_descriptor* plugin);

/* Plugin cleanup callback */
typedef void (*plugin_cleanup_func_t)(struct plugin_descriptor* plugin);

/* Plugin optional function callback */
typedef int (*plugin_function_t)(void* context, void* params);

/* Plugin configuration callback */
typedef int (*plugin_config_func_t)(struct plugin_descriptor* plugin, const char* key, const char* value);

/**
 * Plugin configuration structure
 * Allows plugins to be configured at runtime
 */
typedef struct plugin_config {
    char key[64];
    char value[128];
    struct plugin_config* next;
} plugin_config_t;

/**
 * Plugin descriptor structure
 * Each plugin must provide this structure
 */
typedef struct plugin_descriptor {
    /* Plugin metadata */
    char name[PLUGIN_NAME_MAX];
    uint32_t version_major;
    uint32_t version_minor;
    plugin_type_t type;
    plugin_priority_t priority;
    
    /* API version this plugin was built against */
    uint32_t api_version_major;
    uint32_t api_version_minor;
    
    /* Security fields */
    uint8_t signature[PLUGIN_SIGNATURE_SIZE];  /* Quantum crypto signature */
    uint32_t permissions;                       /* Plugin permissions bitmask */
    uint32_t interference_flags;                /* Functions this plugin may interfere with */
    uint8_t verified;                           /* 1 if signature verified, 0 otherwise */
    
    /* Plugin lifecycle callbacks */
    plugin_init_func_t init;
    plugin_cleanup_func_t cleanup;
    
    /* Optional function provided by plugin */
    plugin_function_t function;
    
    /* Configuration callback */
    plugin_config_func_t config;
    
    /* Plugin-specific data */
    void* private_data;
    
    /* Plugin configuration */
    plugin_config_t* config_list;
    
    /* Internal use by plugin manager */
    struct plugin_descriptor* next;
    int loaded;
    int enabled;  /* 1 if plugin should be loaded, 0 if disabled */
} plugin_descriptor_t;

/**
 * Plugin manager API functions
 */

/* Initialize the plugin system */
void plugin_system_init(void);

/* Register a plugin with the system */
int plugin_register(plugin_descriptor_t* plugin);

/* Unregister a plugin */
int plugin_unregister(const char* name);

/* Find a plugin by name */
plugin_descriptor_t* plugin_find(const char* name);

/* Initialize all registered plugins (called during boot) */
int plugin_init_all(void);

/* Cleanup all plugins */
void plugin_cleanup_all(void);

/* Call a plugin's optional function */
int plugin_call(const char* name, void* context, void* params);

/* List all registered plugins (for debugging) */
void plugin_list_all(void);

/* Get plugin count */
int plugin_get_count(void);

/* Plugin configuration functions */
int plugin_set_config(const char* name, const char* key, const char* value);
const char* plugin_get_config(const char* name, const char* key);
void plugin_clear_config(const char* name);

/* Plugin enable/disable functions */
int plugin_enable(const char* name);
int plugin_disable(const char* name);
int plugin_is_enabled(const char* name);

/* Security functions */
int plugin_verify_signature(plugin_descriptor_t* plugin);
int plugin_check_permission(plugin_descriptor_t* plugin, plugin_permissions_t required_perm);
void plugin_report_interference(const char* plugin_name);
void plugin_list_interference_flags(void);

/**
 * Helper macro for plugin definition
 * Usage in plugin code:
 * DEFINE_PLUGIN(my_plugin, "My Plugin", 1, 0, PLUGIN_TYPE_DRIVER, 
 *               PLUGIN_PRIORITY_NORMAL, my_init, my_cleanup, my_function);
 */
#define DEFINE_PLUGIN(var_name, plugin_name_str, major, minor, plugin_type, plugin_priority, init_fn, cleanup_fn, func_fn) \
    plugin_descriptor_t var_name = { \
        plugin_name_str, \
        major, \
        minor, \
        plugin_type, \
        plugin_priority, \
        PLUGIN_API_VERSION_MAJOR, \
        PLUGIN_API_VERSION_MINOR, \
        {0}, \
        PLUGIN_PERM_NONE, \
        PLUGIN_INTERFERE_NONE, \
        0, \
        init_fn, \
        cleanup_fn, \
        func_fn, \
        NULL, \
        NULL, \
        NULL, \
        NULL, \
        0, \
        1 \
    }

/**
 * Helper macro for secure plugin definition with permissions
 */
#define DEFINE_SECURE_PLUGIN(var_name, plugin_name_str, major, minor, plugin_type, plugin_priority, perms, interference, init_fn, cleanup_fn, func_fn) \
    plugin_descriptor_t var_name = { \
        plugin_name_str, \
        major, \
        minor, \
        plugin_type, \
        plugin_priority, \
        PLUGIN_API_VERSION_MAJOR, \
        PLUGIN_API_VERSION_MINOR, \
        {0}, \
        perms, \
        interference, \
        0, \
        init_fn, \
        cleanup_fn, \
        func_fn, \
        NULL, \
        NULL, \
        NULL, \
        NULL, \
        0, \
        1 \
    }

#endif /* AURORA_PLUGIN_H */
