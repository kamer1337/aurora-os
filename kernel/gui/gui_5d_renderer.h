/**
 * Aurora OS - Enhanced 5D Renderer with Plugin Support
 * 
 * Extended 5D rendering capabilities with plugin-based effects
 */

#ifndef GUI_5D_RENDERER_H
#define GUI_5D_RENDERER_H

#include <stdint.h>
#include "framebuffer.h"
#include "gui_effects.h"
#include "theme_plugin.h"

/**
 * 5D Rendering pipeline modes
 */
typedef enum {
    RENDER_MODE_STANDARD,       // Standard 5D rendering
    RENDER_MODE_ENHANCED,       // Enhanced with plugin effects
    RENDER_MODE_PLUGIN_ONLY     // Only plugin effects
} render_mode_t;

/**
 * 5D Layer structure for depth-based rendering
 */
typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
    float depth;                // 0.0 = front, 1.0 = back
    float opacity;              // 0.0 = transparent, 1.0 = opaque
    color_t tint;
    void* content;              // Layer content (window, widget, etc.)
} render_layer_5d_t;

/**
 * 5D Renderer context
 */
typedef struct {
    render_mode_t mode;
    theme_plugin_t* active_plugin;
    uint8_t enable_depth_fog;
    uint8_t enable_volumetric_lighting;
    uint8_t enable_motion_blur;
    float depth_of_field;
} renderer_5d_context_t;

/**
 * Initialize the 5D renderer
 * @return 0 on success, -1 on failure
 */
int gui_5d_renderer_init(void);

/**
 * Shutdown the 5D renderer
 */
void gui_5d_renderer_shutdown(void);

/**
 * Get the 5D renderer context
 * @return Pointer to context
 */
renderer_5d_context_t* gui_5d_renderer_get_context(void);

/**
 * Set the rendering mode
 * @param mode Rendering mode
 * @return 0 on success, -1 on failure
 */
int gui_5d_renderer_set_mode(render_mode_t mode);

/**
 * Enable/disable depth fog effect
 * @param enable 1 to enable, 0 to disable
 */
void gui_5d_renderer_enable_depth_fog(uint8_t enable);

/**
 * Enable/disable volumetric lighting
 * @param enable 1 to enable, 0 to disable
 */
void gui_5d_renderer_enable_volumetric_lighting(uint8_t enable);

/**
 * Enable/disable motion blur
 * @param enable 1 to enable, 0 to disable
 */
void gui_5d_renderer_enable_motion_blur(uint8_t enable);

/**
 * Set depth of field effect strength
 * @param strength Strength (0.0-1.0)
 */
void gui_5d_renderer_set_depth_of_field(float strength);

/**
 * Render a 5D layer with all active effects
 * @param layer Pointer to layer structure
 * @return 0 on success, -1 on failure
 */
int gui_5d_renderer_render_layer(render_layer_5d_t* layer);

/**
 * Apply depth fog to a region
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param depth Depth value (0.0-1.0)
 */
void gui_5d_apply_depth_fog(int32_t x, int32_t y, uint32_t width, uint32_t height, float depth);

/**
 * Apply volumetric lighting effect
 * @param x Light source X
 * @param y Light source Y
 * @param radius Light radius
 * @param intensity Light intensity (0.0-1.0)
 * @param color Light color
 */
void gui_5d_apply_volumetric_light(int32_t x, int32_t y, uint32_t radius, 
                                    float intensity, color_t color);

/**
 * Draw a holographic effect
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param color Base color
 * @param time Animation time value
 */
void gui_5d_draw_hologram(int32_t x, int32_t y, uint32_t width, uint32_t height,
                          color_t color, float time);

/**
 * Draw a chromatic aberration effect
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param strength Effect strength (0.0-1.0)
 */
void gui_5d_draw_chromatic_aberration(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                       float strength);

/**
 * Draw a depth-based parallax effect
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param depth Depth level
 * @param offset_x X parallax offset
 * @param offset_y Y parallax offset
 */
void gui_5d_draw_parallax(int32_t x, int32_t y, uint32_t width, uint32_t height,
                          float depth, int32_t offset_x, int32_t offset_y);

/**
 * Apply a lens distortion effect
 * @param x Center X
 * @param y Center Y
 * @param radius Effect radius
 * @param strength Distortion strength (-1.0 to 1.0)
 */
void gui_5d_apply_lens_distortion(int32_t x, int32_t y, uint32_t radius, float strength);

/**
 * Render plugin effects for current context
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param params Plugin-specific parameters
 * @return 0 on success, -1 on failure
 */
int gui_5d_render_plugin_effects(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                  void* params);

#endif // GUI_5D_RENDERER_H
