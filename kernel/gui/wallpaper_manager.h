/**
 * Aurora OS - Wallpaper Manager
 * 
 * Manages desktop wallpapers including static and dynamic wallpapers
 */

#ifndef WALLPAPER_MANAGER_H
#define WALLPAPER_MANAGER_H

#include <stdint.h>
#include "framebuffer.h"
#include "live_wallpaper.h"

/**
 * Wallpaper mode types
 */
typedef enum {
    WALLPAPER_MODE_NONE,           // No wallpaper (solid color)
    WALLPAPER_MODE_SOLID,          // Solid color background
    WALLPAPER_MODE_GRADIENT,       // Gradient background
    WALLPAPER_MODE_GRADIENT_H,     // Horizontal gradient
    WALLPAPER_MODE_GRADIENT_V,     // Vertical gradient
    WALLPAPER_MODE_GRADIENT_RADIAL,// Radial gradient
    WALLPAPER_MODE_LIVE,           // Live/animated wallpaper
    WALLPAPER_MODE_IMAGE,          // Static image (future)
    WALLPAPER_MODE_COUNT
} wallpaper_mode_t;

/**
 * Predefined gradient wallpaper styles
 */
typedef enum {
    GRADIENT_STYLE_BLUE_SKY,       // Blue sky gradient (default)
    GRADIENT_STYLE_OCEAN,          // Ocean blue-green gradient
    GRADIENT_STYLE_SUNSET,         // Orange-pink sunset gradient
    GRADIENT_STYLE_FOREST,         // Green forest gradient
    GRADIENT_STYLE_PURPLE_DREAM,   // Purple gradient
    GRADIENT_STYLE_NIGHT,          // Dark night gradient
    GRADIENT_STYLE_WARM,           // Warm brown-orange gradient
    GRADIENT_STYLE_COOL,           // Cool blue-cyan gradient
    GRADIENT_STYLE_GRAYSCALE,      // Gray gradient
    GRADIENT_STYLE_CUSTOM,         // Custom colors
    GRADIENT_STYLE_COUNT
} gradient_style_t;

/**
 * Wallpaper configuration
 */
typedef struct {
    wallpaper_mode_t mode;
    
    // Solid color settings
    color_t solid_color;
    
    // Gradient settings
    gradient_style_t gradient_style;
    color_t gradient_start;
    color_t gradient_end;
    color_t gradient_center;  // For radial gradients
    
    // Live wallpaper settings
    wallpaper_type_t live_type;
    uint8_t live_enabled;
    
    // Image settings (future)
    const char* image_path;
} wallpaper_config_t;

/**
 * Initialize the wallpaper manager
 * @return 0 on success, -1 on failure
 */
int wallpaper_manager_init(void);

/**
 * Shutdown the wallpaper manager
 */
void wallpaper_manager_shutdown(void);

/**
 * Get current wallpaper configuration
 * @return Pointer to current configuration
 */
wallpaper_config_t* wallpaper_manager_get_config(void);

/**
 * Apply wallpaper configuration
 * @param config Configuration to apply
 * @return 0 on success, -1 on failure
 */
int wallpaper_manager_apply_config(const wallpaper_config_t* config);

/**
 * Set wallpaper mode
 * @param mode Wallpaper mode to set
 * @return 0 on success, -1 on failure
 */
int wallpaper_manager_set_mode(wallpaper_mode_t mode);

/**
 * Set solid color wallpaper
 * @param color Color to use
 * @return 0 on success, -1 on failure
 */
int wallpaper_manager_set_solid_color(color_t color);

/**
 * Set gradient wallpaper with predefined style
 * @param style Gradient style
 * @param mode Gradient mode (horizontal, vertical, radial)
 * @return 0 on success, -1 on failure
 */
int wallpaper_manager_set_gradient(gradient_style_t style, wallpaper_mode_t mode);

/**
 * Set custom gradient wallpaper
 * @param start Start color
 * @param end End color
 * @param mode Gradient mode
 * @return 0 on success, -1 on failure
 */
int wallpaper_manager_set_custom_gradient(color_t start, color_t end, wallpaper_mode_t mode);

/**
 * Set live wallpaper
 * @param type Live wallpaper type
 * @return 0 on success, -1 on failure
 */
int wallpaper_manager_set_live(wallpaper_type_t type);

/**
 * Get gradient style name
 * @param style Gradient style
 * @return Style name string
 */
const char* wallpaper_manager_get_gradient_name(gradient_style_t style);

/**
 * Get gradient colors for a style
 * @param style Gradient style
 * @param start Output start color
 * @param end Output end color
 */
void wallpaper_manager_get_gradient_colors(gradient_style_t style, color_t* start, color_t* end);

/**
 * Draw the current wallpaper
 * @param width Screen width
 * @param height Screen height (excluding taskbar)
 */
void wallpaper_manager_draw(uint32_t width, uint32_t height);

/**
 * Update wallpaper (for animated wallpapers)
 * @param delta_time Time since last update in milliseconds
 * @param cursor_x Current cursor X position
 * @param cursor_y Current cursor Y position
 */
void wallpaper_manager_update(uint32_t delta_time, int32_t cursor_x, int32_t cursor_y);

/**
 * Show wallpaper selection window
 */
void wallpaper_manager_show_selector(void);

#endif // WALLPAPER_MANAGER_H
