/**
 * Aurora OS - System Settings Application
 * 
 * Central configuration management for Aurora OS
 */

#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include <stdint.h>
#include "gui.h"

// Settings categories
typedef enum {
    SETTINGS_DISPLAY,
    SETTINGS_THEME,
    SETTINGS_USER,
    SETTINGS_NETWORK,
    SETTINGS_SECURITY,
    SETTINGS_ABOUT
} settings_category_t;

// Display settings
typedef struct {
    uint32_t resolution_width;
    uint32_t resolution_height;
    uint8_t color_depth;
    uint8_t refresh_rate;
} display_settings_t;

// Theme settings
typedef struct {
    char theme_name[32];
    color_t primary_color;
    color_t secondary_color;
    color_t accent_color;
    uint8_t enable_effects;
    uint8_t enable_animations;
    uint8_t enable_transparency;
} theme_settings_t;

// User settings
typedef struct {
    char username[32];
    char full_name[64];
    uint8_t auto_login;
    uint8_t show_desktop_icons;
    uint8_t show_taskbar;
} user_settings_t;

// System settings structure
typedef struct {
    display_settings_t display;
    theme_settings_t theme;
    user_settings_t user;
    uint8_t settings_changed;
} system_settings_t;

/**
 * Launch the system settings application
 * @return Pointer to created window or NULL on failure
 */
window_t* launch_settings_app(void);

/**
 * Get current system settings
 * @return Pointer to system settings structure
 */
system_settings_t* get_system_settings(void);

/**
 * Apply settings changes
 * @return 0 on success, -1 on failure
 */
int apply_settings(void);

/**
 * Reset settings to defaults
 */
void reset_settings_to_defaults(void);

/**
 * Save settings to persistent storage
 * @return 0 on success, -1 on failure
 */
int save_settings(void);

#endif // SETTINGS_APP_H
