/**
 * Aurora OS - Live Wallpaper System
 * 
 * Provides 4D/5D spatial depth live wallpaper with interactive depth perception
 * and nature scene models for an immersive desktop experience.
 */

#ifndef LIVE_WALLPAPER_H
#define LIVE_WALLPAPER_H

#include <stdint.h>
#include "framebuffer.h"
#include "gui_effects.h"

/**
 * Wallpaper types
 */
typedef enum {
    WALLPAPER_NONE,           // No wallpaper (gradient only)
    WALLPAPER_NATURE_FOREST,  // Forest scene with trees and mountains
    WALLPAPER_NATURE_OCEAN,   // Ocean scene with waves and sky
    WALLPAPER_NATURE_MOUNTAIN,// Mountain landscape
    WALLPAPER_NATURE_MEADOW   // Meadow with grass and flowers
} wallpaper_type_t;

/**
 * Depth layer structure for parallax effect
 */
typedef struct {
    float depth_factor;       // How much this layer moves (0.0 = static, 1.0 = full parallax)
    float offset_x;           // Current horizontal offset
    float offset_y;           // Current vertical offset
    uint8_t visible;          // Whether this layer is visible
} depth_layer_t;

/**
 * Nature element types
 */
typedef enum {
    ELEMENT_TREE,
    ELEMENT_MOUNTAIN,
    ELEMENT_CLOUD,
    ELEMENT_GRASS,
    ELEMENT_FLOWER,
    ELEMENT_BIRD,
    ELEMENT_LEAF
} element_type_t;

/**
 * Nature scene element
 */
typedef struct {
    element_type_t type;
    float x, y;               // Position
    float depth;              // Depth level (0.0 = background, 1.0 = foreground)
    float scale;              // Size scale
    color_t color;            // Element color
    float animation_offset;   // For animated elements
    uint8_t visible;
} nature_element_t;

/**
 * Live wallpaper configuration
 */
typedef struct {
    wallpaper_type_t type;
    uint8_t enabled;
    uint8_t parallax_enabled;
    uint8_t particles_enabled;
    uint8_t animation_enabled;
    float parallax_intensity;  // 0.0-1.0
    float animation_speed;     // Animation speed multiplier
    uint32_t particle_count;   // Number of ambient particles
} live_wallpaper_config_t;

/**
 * Initialize the live wallpaper system
 * @return 0 on success, -1 on failure
 */
int live_wallpaper_init(void);

/**
 * Shutdown the live wallpaper system
 */
void live_wallpaper_shutdown(void);

/**
 * Set the wallpaper type
 * @param type Wallpaper type to set
 * @return 0 on success, -1 on failure
 */
int live_wallpaper_set_type(wallpaper_type_t type);

/**
 * Get the current wallpaper type
 * @return Current wallpaper type
 */
wallpaper_type_t live_wallpaper_get_type(void);

/**
 * Enable or disable the live wallpaper
 * @param enabled 1 to enable, 0 to disable
 */
void live_wallpaper_set_enabled(uint8_t enabled);

/**
 * Check if live wallpaper is enabled
 * @return 1 if enabled, 0 if disabled
 */
uint8_t live_wallpaper_is_enabled(void);

/**
 * Get current configuration
 * @return Pointer to current configuration
 */
live_wallpaper_config_t* live_wallpaper_get_config(void);

/**
 * Apply configuration
 * @param config Configuration to apply
 */
void live_wallpaper_apply_config(const live_wallpaper_config_t* config);

/**
 * Update the live wallpaper (animations, particles, etc.)
 * @param delta_time Time since last update in milliseconds
 * @param cursor_x Current cursor X position
 * @param cursor_y Current cursor Y position
 */
void live_wallpaper_update(uint32_t delta_time, int32_t cursor_x, int32_t cursor_y);

/**
 * Draw the live wallpaper
 * @param width Screen width
 * @param height Screen height (excluding taskbar)
 */
void live_wallpaper_draw(uint32_t width, uint32_t height);

/**
 * Draw a nature element
 * @param element Element to draw
 */
void live_wallpaper_draw_element(const nature_element_t* element);

#endif // LIVE_WALLPAPER_H
