/**
 * Aurora OS - Desktop Configuration System
 * 
 * Manages desktop appearance and behavior settings
 */

#ifndef DESKTOP_CONFIG_H
#define DESKTOP_CONFIG_H

#include <stdint.h>
#include "framebuffer.h"
#include "font_manager.h"

/**
 * Desktop configuration structure
 */
typedef struct {
    // Font settings
    font_type_t default_font;
    
    // Color scheme
    color_t desktop_bg_start;      // Gradient start color
    color_t desktop_bg_end;        // Gradient end color
    color_t taskbar_bg;
    color_t taskbar_fg;
    color_t window_title_active;
    color_t window_title_inactive;
    color_t window_border;
    
    // Desktop behavior
    uint8_t show_desktop_icons;
    uint8_t show_taskbar;
    uint8_t show_system_tray;
    uint8_t enable_animations;
    uint8_t enable_shadows;
    uint8_t enable_transparency;
    
    // Icon settings
    uint32_t icon_size;
    uint32_t icon_spacing;
    
    // Window settings
    uint8_t window_snap_enabled;
    uint8_t window_animations;
    uint32_t animation_speed;
} desktop_config_t;

/**
 * Initialize desktop configuration with defaults
 * @return 0 on success, -1 on failure
 */
int desktop_config_init(void);

/**
 * Get current desktop configuration
 * @return Pointer to configuration structure
 */
desktop_config_t* desktop_config_get(void);

/**
 * Apply desktop configuration
 * @param config Configuration to apply
 * @return 0 on success, -1 on failure
 */
int desktop_config_apply(const desktop_config_t* config);

/**
 * Reset configuration to defaults
 */
void desktop_config_reset(void);

/**
 * Save configuration (stub for future persistence)
 * @return 0 on success, -1 on failure
 */
int desktop_config_save(void);

/**
 * Load configuration (stub for future persistence)
 * @return 0 on success, -1 on failure
 */
int desktop_config_load(void);

/**
 * Show desktop settings window
 */
void desktop_config_show_settings(void);

#endif // DESKTOP_CONFIG_H
