/**
 * Aurora OS - Plugin System Implementation
 * 
 * Manages plugin registration, initialization, and lifecycle
 */

#include "plugin.h"
#include "../drivers/vga.h"
#include "../drivers/serial.h"
#include "../memory/memory.h"

/* Head of the plugin linked list */
static plugin_descriptor_t* plugin_list_head = NULL;

/* Plugin statistics */
static int plugin_count = 0;
static int plugins_initialized = 0;

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
    
    if (!plugin->loaded) {
        return PLUGIN_ERROR;
    }
    
    if (!plugin->function) {
        return PLUGIN_ERROR;
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
