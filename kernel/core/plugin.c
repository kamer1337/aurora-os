/**
 * Aurora OS - Plugin System Implementation
 * 
 * Manages plugin registration, initialization, and lifecycle
 */

#include "plugin.h"
#include "../drivers/vga.h"
#include "../drivers/serial.h"
#include "../memory/memory.h"
#include "../security/quantum_crypto.h"

/* Head of the plugin linked list */
static plugin_descriptor_t* plugin_list_head = NULL;

/* Plugin statistics */
static int plugin_count = 0;
static int plugins_initialized = 0;

/* Security: Track quantum crypto interference */
static int quantum_crypto_interference_detected = 0;

/**
 * Initialize the plugin system
 */
void plugin_system_init(void) {
    plugin_list_head = NULL;
    plugin_count = 0;
    plugins_initialized = 0;
    
    vga_write("Plugin system initialized\n");
    serial_write(SERIAL_COM1, "Plugin system ready - API version ");
    /* Simple version output */
    serial_write(SERIAL_COM1, "1.0\n");
}

/**
 * Compare plugin priorities (for sorting)
 * Returns: -1 if p1 < p2, 0 if equal, 1 if p1 > p2
 */
static int plugin_compare_priority(plugin_descriptor_t* p1, plugin_descriptor_t* p2) {
    if (p1->priority < p2->priority) return -1;
    if (p1->priority > p2->priority) return 1;
    return 0;
}

/**
 * Register a plugin with the system
 * Plugins are inserted in priority order (critical first)
 */
int plugin_register(plugin_descriptor_t* plugin) {
    if (!plugin) {
        return PLUGIN_ERROR;
    }
    
    /* Validate API version */
    if (plugin->api_version_major != PLUGIN_API_VERSION_MAJOR) {
        vga_write("ERROR: Plugin API version mismatch\n");
        return PLUGIN_ERROR_INVALID_VERSION;
    }
    
    /* Check if plugin already registered */
    plugin_descriptor_t* existing = plugin_find(plugin->name);
    if (existing) {
        vga_write("ERROR: Plugin already registered: ");
        vga_write(plugin->name);
        vga_write("\n");
        return PLUGIN_ERROR_ALREADY_REGISTERED;
    }
    
    /* SECURITY: Verify plugin signature using quantum cryptography */
    int verify_result = plugin_verify_signature(plugin);
    if (verify_result != PLUGIN_SUCCESS) {
        vga_write("ERROR: Plugin verification failed: ");
        vga_write(plugin->name);
        vga_write("\n");
        serial_write(SERIAL_COM1, "SECURITY: Rejecting unverified plugin\n");
        return verify_result;
    }
    
    /* SECURITY: Report interference flags */
    if (plugin->interference_flags != PLUGIN_INTERFERE_NONE) {
        plugin_report_interference(plugin->name);
    }
    
    /* Insert plugin in priority order */
    if (!plugin_list_head || plugin_compare_priority(plugin, plugin_list_head) < 0) {
        /* Insert at head */
        plugin->next = plugin_list_head;
        plugin_list_head = plugin;
    } else {
        /* Find insertion point */
        plugin_descriptor_t* current = plugin_list_head;
        while (current->next && plugin_compare_priority(plugin, current->next) >= 0) {
            current = current->next;
        }
        plugin->next = current->next;
        current->next = plugin;
    }
    
    plugin_count++;
    
    vga_write("Registered plugin: ");
    vga_write(plugin->name);
    vga_write("\n");
    
    return PLUGIN_SUCCESS;
}

/**
 * Unregister a plugin
 */
int plugin_unregister(const char* name) {
    if (!name || !plugin_list_head) {
        return PLUGIN_ERROR;
    }
    
    plugin_descriptor_t* current = plugin_list_head;
    plugin_descriptor_t* prev = NULL;
    
    while (current) {
        /* Simple string comparison */
        int match = 1;
        for (int i = 0; i < PLUGIN_NAME_MAX; i++) {
            if (current->name[i] != name[i]) {
                match = 0;
                break;
            }
            if (current->name[i] == '\0') break;
        }
        
        if (match) {
            /* Call cleanup if plugin was loaded */
            if (current->loaded && current->cleanup) {
                current->cleanup(current);
            }
            
            /* Remove from list */
            if (prev) {
                prev->next = current->next;
            } else {
                plugin_list_head = current->next;
            }
            
            plugin_count--;
            
            vga_write("Unregistered plugin: ");
            vga_write(name);
            vga_write("\n");
            
            return PLUGIN_SUCCESS;
        }
        
        prev = current;
        current = current->next;
    }
    
    return PLUGIN_ERROR_NOT_FOUND;
}

/**
 * Find a plugin by name
 */
plugin_descriptor_t* plugin_find(const char* name) {
    if (!name) {
        return NULL;
    }
    
    plugin_descriptor_t* current = plugin_list_head;
    
    while (current) {
        /* Simple string comparison */
        int match = 1;
        for (int i = 0; i < PLUGIN_NAME_MAX; i++) {
            if (current->name[i] != name[i]) {
                match = 0;
                break;
            }
            if (current->name[i] == '\0') break;
        }
        
        if (match) {
            return current;
        }
        
        current = current->next;
    }
    
    return NULL;
}

/**
 * Initialize all registered plugins
 * Called during bootloader/kernel initialization
 */
int plugin_init_all(void) {
    vga_write("\n=== Initializing plugins ===\n");
    
    plugin_descriptor_t* current = plugin_list_head;
    int initialized = 0;
    int failed = 0;
    
    while (current) {
        vga_write("Initializing plugin: ");
        vga_write(current->name);
        vga_write(" ... ");
        
        int result = PLUGIN_SUCCESS;
        if (current->init) {
            result = current->init(current);
        }
        
        if (result == PLUGIN_SUCCESS) {
            current->loaded = 1;
            initialized++;
            vga_write("OK\n");
        } else {
            /* Check if failure is acceptable */
            if (current->priority == PLUGIN_PRIORITY_OPTIONAL) {
                vga_write("FAILED (optional)\n");
            } else {
                vga_write("FAILED (critical)\n");
                failed++;
            }
        }
        
        current = current->next;
    }
    
    plugins_initialized = 1;
    
    vga_write("Plugin initialization complete: ");
    /* Simple number output */
    vga_write_hex(initialized);
    vga_write(" loaded, ");
    vga_write_hex(failed);
    vga_write(" failed\n");
    
    return (failed > 0) ? PLUGIN_ERROR : PLUGIN_SUCCESS;
}

/**
 * Cleanup all plugins
 */
void plugin_cleanup_all(void) {
    vga_write("\n=== Cleaning up plugins ===\n");
    
    plugin_descriptor_t* current = plugin_list_head;
    
    while (current) {
        if (current->loaded && current->cleanup) {
            vga_write("Cleaning up plugin: ");
            vga_write(current->name);
            vga_write("\n");
            current->cleanup(current);
            current->loaded = 0;
        }
        current = current->next;
    }
}

/**
 * Call a plugin's optional function
 */
int plugin_call(const char* name, void* context, void* params) {
    plugin_descriptor_t* plugin = plugin_find(name);
    
    if (!plugin) {
        return PLUGIN_ERROR_NOT_FOUND;
    }
    
    /* SECURITY: Verify plugin is loaded and verified */
    if (!plugin->loaded) {
        return PLUGIN_ERROR;
    }
    
    if (!plugin->verified) {
        vga_write("SECURITY: Refusing to call unverified plugin: ");
        vga_write(name);
        vga_write("\n");
        return PLUGIN_ERROR_SECURITY_VIOLATION;
    }
    
    if (!plugin->function) {
        return PLUGIN_ERROR;
    }
    
    /* SECURITY: Log if plugin interferes with quantum crypto */
    if (plugin->interference_flags & PLUGIN_INTERFERE_QUANTUM_CRYPTO) {
        serial_write(SERIAL_COM1, "SECURITY: Calling plugin that may interfere with quantum crypto: ");
        serial_write(SERIAL_COM1, name);
        serial_write(SERIAL_COM1, "\n");
    }
    
    return plugin->function(context, params);
}

/**
 * List all registered plugins (for debugging)
 */
void plugin_list_all(void) {
    vga_write("\n=== Registered Plugins ===\n");
    
    if (plugin_count == 0) {
        vga_write("No plugins registered\n");
        return;
    }
    
    plugin_descriptor_t* current = plugin_list_head;
    int index = 0;
    
    while (current) {
        vga_write_dec(index + 1);
        vga_write(". ");
        vga_write(current->name);
        vga_write(" (v");
        vga_write_dec(current->version_major);
        vga_write(".");
        vga_write_dec(current->version_minor);
        vga_write(") - ");
        
        /* Print type */
        switch (current->type) {
            case PLUGIN_TYPE_DRIVER:
                vga_write("Driver");
                break;
            case PLUGIN_TYPE_FILESYSTEM:
                vga_write("Filesystem");
                break;
            case PLUGIN_TYPE_BOOT_SETUP:
                vga_write("Boot Setup");
                break;
            case PLUGIN_TYPE_DIAGNOSTICS:
                vga_write("Diagnostics");
                break;
            case PLUGIN_TYPE_SECURITY:
                vga_write("Security");
                break;
            case PLUGIN_TYPE_NETWORK:
                vga_write("Network");
                break;
            default:
                vga_write("Other");
                break;
        }
        
        vga_write(" [");
        vga_write(current->loaded ? "loaded" : "not loaded");
        vga_write("]\n");
        
        index++;
        current = current->next;
    }
    
    vga_write("Total plugins: ");
    vga_write_dec(plugin_count);
    vga_write("\n");
}

/**
 * Get plugin count
 */
int plugin_get_count(void) {
    return plugin_count;
}

/**
 * Verify plugin signature using quantum cryptography
 * This protects against malicious/tampered plugins
 */
int plugin_verify_signature(plugin_descriptor_t* plugin) {
    if (!plugin) {
        return PLUGIN_ERROR;
    }
    
    /* Calculate hash of plugin metadata for verification */
    uint8_t plugin_data[256];
    uint8_t computed_hash[PLUGIN_SIGNATURE_SIZE];
    
    /* Serialize plugin metadata for hashing */
    int offset = 0;
    for (int i = 0; i < PLUGIN_NAME_MAX && plugin->name[i] != '\0'; i++) {
        plugin_data[offset++] = (uint8_t)plugin->name[i];
    }
    plugin_data[offset++] = (uint8_t)(plugin->version_major & 0xFF);
    plugin_data[offset++] = (uint8_t)((plugin->version_major >> 8) & 0xFF);
    plugin_data[offset++] = (uint8_t)(plugin->version_minor & 0xFF);
    plugin_data[offset++] = (uint8_t)((plugin->version_minor >> 8) & 0xFF);
    plugin_data[offset++] = (uint8_t)plugin->type;
    plugin_data[offset++] = (uint8_t)plugin->priority;
    
    /* Compute quantum hash */
    int result = quantum_hash(plugin_data, offset, computed_hash, PLUGIN_SIGNATURE_SIZE);
    if (result != QCRYPTO_SUCCESS) {
        vga_write("ERROR: Quantum hash computation failed for plugin: ");
        vga_write(plugin->name);
        vga_write("\n");
        serial_write(SERIAL_COM1, "SECURITY: Plugin verification failed - hash error\n");
        return PLUGIN_ERROR_VERIFICATION_FAILED;
    }
    
    /* Verify signature matches (for now, we auto-sign on first registration) */
    /* In production, plugins would come pre-signed by trusted authority */
    int signature_valid = 1;
    int has_signature = 0;
    for (int i = 0; i < PLUGIN_SIGNATURE_SIZE; i++) {
        if (plugin->signature[i] != 0) {
            has_signature = 1;
        }
        if (plugin->signature[i] != computed_hash[i]) {
            signature_valid = 0;
        }
    }
    
    /* If no signature, auto-sign (trusted local plugin) */
    if (!has_signature) {
        for (int i = 0; i < PLUGIN_SIGNATURE_SIZE; i++) {
            plugin->signature[i] = computed_hash[i];
        }
        plugin->verified = 1;
        vga_write("  Plugin auto-signed (trusted): ");
        vga_write(plugin->name);
        vga_write("\n");
        serial_write(SERIAL_COM1, "SECURITY: Plugin auto-signed as trusted\n");
        return PLUGIN_SUCCESS;
    }
    
    /* Verify existing signature */
    if (signature_valid) {
        plugin->verified = 1;
        vga_write("  Plugin signature verified: ");
        vga_write(plugin->name);
        vga_write("\n");
        serial_write(SERIAL_COM1, "SECURITY: Plugin signature valid\n");
        return PLUGIN_SUCCESS;
    } else {
        plugin->verified = 0;
        vga_write("ERROR: Invalid signature for plugin: ");
        vga_write(plugin->name);
        vga_write("\n");
        serial_write(SERIAL_COM1, "SECURITY ALERT: Plugin signature verification FAILED\n");
        return PLUGIN_ERROR_VERIFICATION_FAILED;
    }
}

/**
 * Check if plugin has required permission
 * Protects against unauthorized access to system resources
 */
int plugin_check_permission(plugin_descriptor_t* plugin, plugin_permissions_t required_perm) {
    if (!plugin) {
        return PLUGIN_ERROR;
    }
    
    if (!plugin->verified) {
        vga_write("SECURITY: Unverified plugin attempted operation: ");
        vga_write(plugin->name);
        vga_write("\n");
        serial_write(SERIAL_COM1, "SECURITY ALERT: Unverified plugin operation blocked\n");
        return PLUGIN_ERROR_SECURITY_VIOLATION;
    }
    
    if ((plugin->permissions & required_perm) != required_perm) {
        vga_write("SECURITY: Plugin lacks permission: ");
        vga_write(plugin->name);
        vga_write("\n");
        serial_write(SERIAL_COM1, "SECURITY ALERT: Insufficient plugin permissions\n");
        return PLUGIN_ERROR_PERMISSION_DENIED;
    }
    
    return PLUGIN_SUCCESS;
}

/**
 * Report plugin interference with system functions
 * Alerts about plugins that may affect critical operations
 */
void plugin_report_interference(const char* plugin_name) {
    plugin_descriptor_t* plugin = plugin_find(plugin_name);
    if (!plugin) {
        return;
    }
    
    if (plugin->interference_flags == PLUGIN_INTERFERE_NONE) {
        vga_write("  Plugin has no interference flags: ");
        vga_write(plugin_name);
        vga_write("\n");
        return;
    }
    
    vga_write("\n=== SECURITY: Plugin Interference Report ===\n");
    vga_write("Plugin: ");
    vga_write(plugin_name);
    vga_write("\n");
    vga_write("May interfere with:\n");
    
    if (plugin->interference_flags & PLUGIN_INTERFERE_QUANTUM_CRYPTO) {
        vga_write("  - QUANTUM CRYPTOGRAPHY (Crystal-Kyber verification)\n");
        serial_write(SERIAL_COM1, "SECURITY: Plugin may interfere with quantum crypto verification\n");
        quantum_crypto_interference_detected = 1;
    }
    if (plugin->interference_flags & PLUGIN_INTERFERE_MEMORY) {
        vga_write("  - Memory Management\n");
    }
    if (plugin->interference_flags & PLUGIN_INTERFERE_PROCESS) {
        vga_write("  - Process Scheduling\n");
    }
    if (plugin->interference_flags & PLUGIN_INTERFERE_FILESYSTEM) {
        vga_write("  - Filesystem Operations\n");
    }
    if (plugin->interference_flags & PLUGIN_INTERFERE_NETWORK) {
        vga_write("  - Network Stack\n");
    }
    if (plugin->interference_flags & PLUGIN_INTERFERE_SECURITY) {
        vga_write("  - Security Subsystem\n");
    }
    
    vga_write("Verification status: ");
    vga_write(plugin->verified ? "VERIFIED" : "UNVERIFIED");
    vga_write("\n");
    vga_write("Permissions: 0x");
    vga_write_hex(plugin->permissions);
    vga_write("\n");
    vga_write("===========================================\n\n");
}

/**
 * List all plugins and their interference flags
 * Security audit function
 */
void plugin_list_interference_flags(void) {
    vga_write("\n=== SECURITY: Plugin Interference Audit ===\n");
    
    if (plugin_count == 0) {
        vga_write("No plugins registered\n");
        return;
    }
    
    plugin_descriptor_t* current = plugin_list_head;
    int interference_count = 0;
    
    while (current) {
        if (current->interference_flags != PLUGIN_INTERFERE_NONE) {
            plugin_report_interference(current->name);
            interference_count++;
        }
        current = current->next;
    }
    
    if (interference_count == 0) {
        vga_write("No plugins have interference flags set\n");
    } else {
        vga_write("Total plugins with interference: ");
        vga_write_dec(interference_count);
        vga_write("\n");
    }
    
    if (quantum_crypto_interference_detected) {
        vga_write("\nWARNING: Quantum cryptography interference detected!\n");
        vga_write("Crystal-Kyber verification may be affected by plugins.\n");
        serial_write(SERIAL_COM1, "SECURITY WARNING: Quantum crypto interference active\n");
    }
    
    vga_write("===========================================\n\n");
}
