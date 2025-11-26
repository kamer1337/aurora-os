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
    
    // Live wallpaper settings
    uint8_t enable_live_wallpaper;
    
    // Widget window transparency (0-100, 0=fully transparent, 100=opaque)
    uint8_t widget_transparency;
    
    // Taskbar settings
    uint8_t taskbar_position;      // 0=bottom, 1=top, 2=left, 3=right
    uint8_t taskbar_auto_hide;     // Auto-hide taskbar
    uint32_t taskbar_height;       // Taskbar height in pixels
    
    // Desktop icon layout
    uint8_t icon_grid_auto;        // Auto-arrange icons
    uint8_t icon_label_position;   // 0=bottom, 1=right
    
    // Window behavior
    uint8_t double_click_titlebar; // 0=maximize, 1=shade, 2=nothing
    uint8_t focus_follows_mouse;   // Focus window on hover
    uint8_t raise_on_focus;        // Raise window when focused
    
    // Cursor settings
    uint8_t cursor_theme;          // 0=default, 1=large, 2=custom
    uint8_t cursor_blink_rate;     // Cursor blink rate (0-10)
    
    // Sound settings
    uint8_t enable_ui_sounds;      // Enable UI sound effects
    uint8_t sound_volume;          // UI sound volume (0-100)
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
 * Save configuration to VFS
 * Writes configuration to /etc/aurora/desktop.cfg with checksums
 * @return 0 on success, -1 on failure
 */
int desktop_config_save(void);

/**
 * Load configuration from VFS
 * Loads configuration from /etc/aurora/desktop.cfg with validation
 * @return 0 on success, -1 on failure
 */
int desktop_config_load(void);

/**
 * Show desktop settings window
 */
void desktop_config_show_settings(void);

/**
 * Show theme selection window
 */
void desktop_config_show_theme_selector(void);

/**
 * Show wallpaper selection window
 */
void desktop_config_show_wallpaper_selector(void);

#endif // DESKTOP_CONFIG_H
