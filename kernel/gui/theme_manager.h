/**
 * Aurora OS - Theme Manager
 * 
 * Manages desktop color themes and appearance presets
 */

#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <stdint.h>
#include "framebuffer.h"

/**
 * Available theme types
 */
typedef enum {
    THEME_DEFAULT,      // Default Aurora theme (blue gradients)
    THEME_DARK,         // Dark theme with dark grays
    THEME_LIGHT,        // Light theme with bright colors
    THEME_OCEAN,        // Ocean-inspired blues and teals
    THEME_FOREST,       // Forest-inspired greens
    THEME_SUNSET,       // Warm sunset colors (orange/pink)
    THEME_PURPLE,       // Purple/violet theme
    THEME_CUSTOM,       // Custom user-defined theme
    THEME_COUNT
} theme_type_t;

/**
 * Theme structure containing all color settings
 */
typedef struct {
    theme_type_t type;
    const char* name;
    
    // Desktop colors
    color_t desktop_bg_start;      // Gradient start color
    color_t desktop_bg_end;        // Gradient end color
    
    // Taskbar colors
    color_t taskbar_bg;
    color_t taskbar_fg;
    color_t taskbar_highlight;
    
    // Window colors
    color_t window_title_active;
    color_t window_title_inactive;
    color_t window_border;
    color_t window_bg;
    
    // Widget colors
    color_t button_bg;
    color_t button_fg;
    color_t button_hover;
    color_t button_pressed;
    
    // Text colors
    color_t text_primary;
    color_t text_secondary;
    color_t text_disabled;
} theme_t;

/**
 * Initialize the theme manager
 * @return 0 on success, -1 on failure
 */
int theme_manager_init(void);

/**
 * Shutdown the theme manager
 */
void theme_manager_shutdown(void);

/**
 * Get a theme by type
 * @param type Theme type
 * @return Pointer to theme or NULL if not found
 */
const theme_t* theme_manager_get_theme(theme_type_t type);

/**
 * Get the current active theme
 * @return Pointer to current theme
 */
const theme_t* theme_manager_get_current(void);

/**
 * Set the active theme
 * @param type Theme type to activate
 * @return 0 on success, -1 on failure
 */
int theme_manager_set_theme(theme_type_t type);

/**
 * Get theme by index
 * @param index Theme index (0 to THEME_COUNT-1)
 * @return Pointer to theme or NULL if invalid index
 */
const theme_t* theme_manager_get_by_index(uint32_t index);

/**
 * Get theme count (excluding THEME_CUSTOM)
 * @return Number of predefined themes
 */
uint32_t theme_manager_get_count(void);

/**
 * Create a custom theme
 * @param custom_theme Theme data to use as custom theme
 * @return 0 on success, -1 on failure
 */
int theme_manager_set_custom(const theme_t* custom_theme);

/**
 * Show theme selection window
 */
void theme_manager_show_selector(void);

#endif // THEME_MANAGER_H
