/**
 * Aurora OS - Desktop Modules System
 * 
 * This module provides a modular desktop architecture with pluggable components.
 * Each desktop module is independent and can be enabled/disabled individually.
 */

#ifndef DESKTOP_MODULES_H
#define DESKTOP_MODULES_H

#include <stdint.h>
#include "gui.h"

/**
 * Desktop module types
 */
typedef enum {
    MODULE_QUICK_LAUNCH,
    MODULE_SYSTEM_TRAY,
    MODULE_WINDOW_SWITCHER,
    MODULE_DESKTOP_ICONS,
    MODULE_START_MENU,
    MODULE_TASKBAR,
    MODULE_COUNT
} desktop_module_type_t;

/**
 * Desktop module structure
 */
typedef struct desktop_module {
    desktop_module_type_t type;
    const char* name;
    uint8_t enabled;
    void (*init)(void);
    void (*update)(void);
    void (*draw)(void);
    void (*shutdown)(void);
} desktop_module_t;

/**
 * Initialize the desktop module system
 * @return 0 on success, -1 on failure
 */
int desktop_modules_init(void);

/**
 * Shutdown the desktop module system
 */
void desktop_modules_shutdown(void);

/**
 * Update all enabled desktop modules
 */
void desktop_modules_update(void);

/**
 * Draw all enabled desktop modules
 */
void desktop_modules_draw(void);

/**
 * Enable a desktop module
 * @param type Module type to enable
 * @return 0 on success, -1 on failure
 */
int desktop_module_enable(desktop_module_type_t type);

/**
 * Disable a desktop module
 * @param type Module type to disable
 * @return 0 on success, -1 on failure
 */
int desktop_module_disable(desktop_module_type_t type);

/**
 * Check if a module is enabled
 * @param type Module type to check
 * @return 1 if enabled, 0 if disabled
 */
int desktop_module_is_enabled(desktop_module_type_t type);

/**
 * Get module by type
 * @param type Module type
 * @return Pointer to module or NULL if not found
 */
desktop_module_t* desktop_module_get(desktop_module_type_t type);

/**
 * 3D Depth Navigation Functions for Desktop Icons
 */

/**
 * Navigate depth forward (bring icons closer)
 */
void desktop_icons_depth_forward(void);

/**
 * Navigate depth backward (push icons away)
 */
void desktop_icons_depth_backward(void);

/**
 * Reset depth navigation to default
 */
void desktop_icons_depth_reset(void);

/**
 * Get current depth offset
 * @return Current depth navigation offset
 */
float desktop_icons_get_depth_offset(void);

/**
 * Adjust depth of currently selected icon
 * @param delta Change in depth (-1.0 to 1.0)
 */
void desktop_icons_adjust_selected_depth(float delta);

#endif // DESKTOP_MODULES_H
