/**
 * Aurora OS - Theme Plugin System
 * 
 * Plugin interface for extensible theme system allowing dynamic loading
 * of custom themes and 5D rendering effects.
 */

#ifndef THEME_PLUGIN_H
#define THEME_PLUGIN_H

#include <stdint.h>
#include "framebuffer.h"
#include "theme_manager.h"
#include "gui_effects.h"

/**
 * Plugin API version for compatibility checking
 */
#define THEME_PLUGIN_API_VERSION 1

/**
 * Plugin types
 */
typedef enum {
    PLUGIN_TYPE_THEME,          // Theme color plugin
    PLUGIN_TYPE_EFFECT,         // 5D rendering effect plugin
    PLUGIN_TYPE_COMBINED        // Both theme and effects
} plugin_type_t;

/**
 * Plugin status
 */
typedef enum {
    PLUGIN_STATUS_UNLOADED,
    PLUGIN_STATUS_LOADED,
    PLUGIN_STATUS_ACTIVE,
    PLUGIN_STATUS_ERROR
} plugin_status_t;

/**
 * Forward declarations
 */
typedef struct theme_plugin theme_plugin_t;
typedef struct plugin_effect plugin_effect_t;

/**
 * Plugin effect rendering callback
 * Custom 5D effects can be implemented via this callback
 * 
 * @param x X position
 * @param y Y position
 * @param width Width of effect region
 * @param height Height of effect region
 * @param params Plugin-specific parameters
 * @param user_data User data passed during registration
 */
typedef void (*plugin_effect_render_fn)(int32_t x, int32_t y, 
                                        uint32_t width, uint32_t height,
                                        void* params, void* user_data);

/**
 * Plugin initialization callback
 * Called when plugin is loaded
 * 
 * @param plugin Pointer to plugin structure
 * @return 0 on success, -1 on failure
 */
typedef int (*plugin_init_fn)(theme_plugin_t* plugin);

/**
 * Plugin shutdown callback
 * Called when plugin is unloaded
 * 
 * @param plugin Pointer to plugin structure
 */
typedef void (*plugin_shutdown_fn)(theme_plugin_t* plugin);

/**
 * Plugin theme provider callback
 * Returns a theme definition from the plugin
 * 
 * @param plugin Pointer to plugin structure
 * @return Pointer to theme or NULL
 */
typedef const theme_t* (*plugin_get_theme_fn)(theme_plugin_t* plugin);

/**
 * Plugin effect structure
 */
struct plugin_effect {
    const char* name;                   // Effect name (e.g., "neon_glow")
    const char* description;            // Effect description
    plugin_effect_render_fn render;     // Rendering callback
    void* user_data;                    // User data for callback
    plugin_effect_t* next;              // Next effect in list
};

/**
 * Theme plugin structure
 */
struct theme_plugin {
    // Plugin metadata
    const char* name;                   // Plugin name
    const char* author;                 // Plugin author
    const char* version;                // Plugin version string
    const char* description;            // Plugin description
    uint32_t api_version;               // API version this plugin uses
    plugin_type_t type;                 // Plugin type
    
    // Plugin status
    plugin_status_t status;             // Current status
    
    // Callbacks
    plugin_init_fn init;                // Init callback
    plugin_shutdown_fn shutdown;        // Shutdown callback
    plugin_get_theme_fn get_theme;      // Theme provider callback (can be NULL)
    
    // Plugin data
    void* private_data;                 // Plugin private data
    plugin_effect_t* effects;           // Linked list of effects (can be NULL)
    
    // Internal management
    theme_plugin_t* next;               // Next plugin in list
};

/**
 * Initialize the plugin system
 * @return 0 on success, -1 on failure
 */
int theme_plugin_system_init(void);

/**
 * Shutdown the plugin system
 */
void theme_plugin_system_shutdown(void);

/**
 * Register a theme plugin
 * @param plugin Pointer to plugin structure
 * @return 0 on success, -1 on failure
 */
int theme_plugin_register(theme_plugin_t* plugin);

/**
 * Unregister a theme plugin
 * @param plugin Pointer to plugin structure
 * @return 0 on success, -1 on failure
 */
int theme_plugin_unregister(theme_plugin_t* plugin);

/**
 * Load a plugin (calls init callback)
 * @param plugin Pointer to plugin structure
 * @return 0 on success, -1 on failure
 */
int theme_plugin_load(theme_plugin_t* plugin);

/**
 * Unload a plugin (calls shutdown callback)
 * @param plugin Pointer to plugin structure
 * @return 0 on success, -1 on failure
 */
int theme_plugin_unload(theme_plugin_t* plugin);

/**
 * Activate a plugin (make it the active theme/effect provider)
 * @param plugin Pointer to plugin structure
 * @return 0 on success, -1 on failure
 */
int theme_plugin_activate(theme_plugin_t* plugin);

/**
 * Get a plugin by name
 * @param name Plugin name
 * @return Pointer to plugin or NULL if not found
 */
theme_plugin_t* theme_plugin_get_by_name(const char* name);

/**
 * Get list of all registered plugins
 * @return Pointer to first plugin in list, or NULL
 */
theme_plugin_t* theme_plugin_get_list(void);

/**
 * Get active theme plugin
 * @return Pointer to active plugin or NULL
 */
theme_plugin_t* theme_plugin_get_active(void);

/**
 * Register a custom effect with a plugin
 * @param plugin Pointer to plugin
 * @param effect Pointer to effect structure
 * @return 0 on success, -1 on failure
 */
int theme_plugin_register_effect(theme_plugin_t* plugin, plugin_effect_t* effect);

/**
 * Unregister a custom effect from a plugin
 * @param plugin Pointer to plugin
 * @param effect_name Name of effect to unregister
 * @return 0 on success, -1 on failure
 */
int theme_plugin_unregister_effect(theme_plugin_t* plugin, const char* effect_name);

/**
 * Get a plugin effect by name
 * @param plugin Pointer to plugin
 * @param effect_name Name of effect
 * @return Pointer to effect or NULL if not found
 */
plugin_effect_t* theme_plugin_get_effect(theme_plugin_t* plugin, const char* effect_name);

/**
 * Render a plugin effect
 * @param plugin Pointer to plugin
 * @param effect_name Name of effect to render
 * @param x X position
 * @param y Y position
 * @param width Width of effect region
 * @param height Height of effect region
 * @param params Effect parameters (can be NULL)
 * @return 0 on success, -1 on failure
 */
int theme_plugin_render_effect(theme_plugin_t* plugin, const char* effect_name,
                                int32_t x, int32_t y, uint32_t width, uint32_t height,
                                void* params);

/**
 * Helper: Compare strings
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, non-zero otherwise
 */
int theme_plugin_strcmp(const char* s1, const char* s2);

#endif // THEME_PLUGIN_H
