/**
 * Aurora OS - Theme Plugin System Implementation
 * 
 * Implements the plugin system for extensible themes and 5D effects
 */

#include "theme_plugin.h"
#include "theme_manager.h"
#include "../memory/memory.h"

// Plugin list head
static theme_plugin_t* plugin_list_head = NULL;
static theme_plugin_t* active_plugin = NULL;
static uint8_t plugin_system_initialized = 0;

// Helper: Compare strings (simple implementation)
int theme_plugin_strcmp(const char* s1, const char* s2) {
    if (!s1 || !s2) {
        return (s1 == s2) ? 0 : 1;
    }
    
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    
    return *s1 - *s2;
}

int theme_plugin_system_init(void) {
    if (plugin_system_initialized) {
        return 0;
    }
    
    plugin_list_head = NULL;
    active_plugin = NULL;
    plugin_system_initialized = 1;
    
    return 0;
}

void theme_plugin_system_shutdown(void) {
    if (!plugin_system_initialized) {
        return;
    }
    
    // Unload all plugins
    theme_plugin_t* current = plugin_list_head;
    while (current) {
        theme_plugin_t* next = current->next;
        theme_plugin_unload(current);
        current = next;
    }
    
    plugin_list_head = NULL;
    active_plugin = NULL;
    plugin_system_initialized = 0;
}

int theme_plugin_register(theme_plugin_t* plugin) {
    if (!plugin_system_initialized) {
        theme_plugin_system_init();
    }
    
    if (!plugin) {
        return -1;
    }
    
    // Validate plugin
    if (!plugin->name || !plugin->version) {
        return -1;
    }
    
    // Check API version compatibility
    if (plugin->api_version != THEME_PLUGIN_API_VERSION) {
        return -1;
    }
    
    // Check if plugin already registered
    theme_plugin_t* existing = theme_plugin_get_by_name(plugin->name);
    if (existing) {
        return -1;  // Already registered
    }
    
    // Set initial status
    plugin->status = PLUGIN_STATUS_UNLOADED;
    
    // Add to list
    plugin->next = plugin_list_head;
    plugin_list_head = plugin;
    
    return 0;
}

int theme_plugin_unregister(theme_plugin_t* plugin) {
    if (!plugin_system_initialized || !plugin) {
        return -1;
    }
    
    // Unload if loaded
    if (plugin->status != PLUGIN_STATUS_UNLOADED) {
        theme_plugin_unload(plugin);
    }
    
    // Remove from list
    if (plugin_list_head == plugin) {
        plugin_list_head = plugin->next;
    } else {
        theme_plugin_t* current = plugin_list_head;
        while (current && current->next != plugin) {
            current = current->next;
        }
        if (current) {
            current->next = plugin->next;
        }
    }
    
    // If this was active plugin, clear it
    if (active_plugin == plugin) {
        active_plugin = NULL;
    }
    
    plugin->next = NULL;
    return 0;
}

int theme_plugin_load(theme_plugin_t* plugin) {
    if (!plugin_system_initialized || !plugin) {
        return -1;
    }
    
    if (plugin->status != PLUGIN_STATUS_UNLOADED) {
        return -1;  // Already loaded
    }
    
    // Call init callback if provided
    if (plugin->init) {
        int result = plugin->init(plugin);
        if (result != 0) {
            plugin->status = PLUGIN_STATUS_ERROR;
            return -1;
        }
    }
    
    plugin->status = PLUGIN_STATUS_LOADED;
    return 0;
}

int theme_plugin_unload(theme_plugin_t* plugin) {
    if (!plugin_system_initialized || !plugin) {
        return -1;
    }
    
    if (plugin->status == PLUGIN_STATUS_UNLOADED) {
        return 0;  // Already unloaded
    }
    
    // Deactivate if active
    if (active_plugin == plugin) {
        active_plugin = NULL;
    }
    
    // Call shutdown callback if provided
    if (plugin->shutdown) {
        plugin->shutdown(plugin);
    }
    
    plugin->status = PLUGIN_STATUS_UNLOADED;
    return 0;
}

int theme_plugin_activate(theme_plugin_t* plugin) {
    if (!plugin_system_initialized || !plugin) {
        return -1;
    }
    
    // Load plugin if not loaded
    if (plugin->status == PLUGIN_STATUS_UNLOADED) {
        int result = theme_plugin_load(plugin);
        if (result != 0) {
            return -1;
        }
    }
    
    if (plugin->status != PLUGIN_STATUS_LOADED) {
        return -1;  // Plugin in error state
    }
    
    // Deactivate current active plugin
    if (active_plugin && active_plugin != plugin) {
        active_plugin->status = PLUGIN_STATUS_LOADED;
    }
    
    // Activate new plugin
    active_plugin = plugin;
    plugin->status = PLUGIN_STATUS_ACTIVE;
    
    // If plugin provides a theme, apply it
    if (plugin->type == PLUGIN_TYPE_THEME || plugin->type == PLUGIN_TYPE_COMBINED) {
        if (plugin->get_theme) {
            const theme_t* theme = plugin->get_theme(plugin);
            if (theme) {
                // Apply theme via theme manager
                theme_manager_set_custom(theme);
                theme_manager_set_theme(THEME_CUSTOM);
            }
        }
    }
    
    return 0;
}

theme_plugin_t* theme_plugin_get_by_name(const char* name) {
    if (!plugin_system_initialized || !name) {
        return NULL;
    }
    
    theme_plugin_t* current = plugin_list_head;
    while (current) {
        if (theme_plugin_strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

theme_plugin_t* theme_plugin_get_list(void) {
    if (!plugin_system_initialized) {
        theme_plugin_system_init();
    }
    
    return plugin_list_head;
}

theme_plugin_t* theme_plugin_get_active(void) {
    if (!plugin_system_initialized) {
        return NULL;
    }
    
    return active_plugin;
}

int theme_plugin_register_effect(theme_plugin_t* plugin, plugin_effect_t* effect) {
    if (!plugin || !effect) {
        return -1;
    }
    
    // Validate effect
    if (!effect->name || !effect->render) {
        return -1;
    }
    
    // Check if effect already exists
    plugin_effect_t* existing = theme_plugin_get_effect(plugin, effect->name);
    if (existing) {
        return -1;  // Already registered
    }
    
    // Add to effects list
    effect->next = plugin->effects;
    plugin->effects = effect;
    
    return 0;
}

int theme_plugin_unregister_effect(theme_plugin_t* plugin, const char* effect_name) {
    if (!plugin || !effect_name) {
        return -1;
    }
    
    // Find and remove effect
    if (plugin->effects && theme_plugin_strcmp(plugin->effects->name, effect_name) == 0) {
        plugin->effects = plugin->effects->next;
        return 0;
    }
    
    plugin_effect_t* current = plugin->effects;
    while (current && current->next) {
        if (theme_plugin_strcmp(current->next->name, effect_name) == 0) {
            current->next = current->next->next;
            return 0;
        }
        current = current->next;
    }
    
    return -1;  // Not found
}

plugin_effect_t* theme_plugin_get_effect(theme_plugin_t* plugin, const char* effect_name) {
    if (!plugin || !effect_name) {
        return NULL;
    }
    
    plugin_effect_t* current = plugin->effects;
    while (current) {
        if (theme_plugin_strcmp(current->name, effect_name) == 0) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

int theme_plugin_render_effect(theme_plugin_t* plugin, const char* effect_name,
                                int32_t x, int32_t y, uint32_t width, uint32_t height,
                                void* params) {
    if (!plugin || !effect_name) {
        return -1;
    }
    
    // Get the effect
    plugin_effect_t* effect = theme_plugin_get_effect(plugin, effect_name);
    if (!effect) {
        return -1;
    }
    
    // Render the effect
    effect->render(x, y, width, height, params, effect->user_data);
    
    return 0;
}
