/**
 * Aurora OS - GUI Advanced Effects
 * 
 * This module provides 3D depth, 4D (animation), and 5D (interactive) effects
 * for the GUI system to create a modern, visually rich interface.
 */

#ifndef GUI_EFFECTS_H
#define GUI_EFFECTS_H

#include <stdint.h>
#include "framebuffer.h"
#include "gui.h"

/**
 * 3D Depth Effects - Visual depth perception
 */

/**
 * Draw a shadow for a rectangle
 * @param x X position of rectangle
 * @param y Y position of rectangle
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param offset Shadow offset in pixels
 * @param blur Shadow blur amount (0-10)
 */
void gui_draw_shadow(int32_t x, int32_t y, uint32_t width, uint32_t height, 
                     uint32_t offset, uint32_t blur);

/**
 * Draw a gradient rectangle (vertical gradient)
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param color1 Start color (top)
 * @param color2 End color (bottom)
 */
void gui_draw_gradient(int32_t x, int32_t y, uint32_t width, uint32_t height,
                       color_t color1, color_t color2);

/**
 * Draw a rounded rectangle
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param radius Corner radius
 * @param color Fill color
 */
void gui_draw_rounded_rect(int32_t x, int32_t y, uint32_t width, uint32_t height,
                           uint32_t radius, color_t color);

/**
 * Alpha blend two colors
 * @param fg Foreground color
 * @param bg Background color
 * @param alpha Alpha value (0-255, 0=transparent, 255=opaque)
 * @return Blended color
 */
color_t gui_alpha_blend(color_t fg, color_t bg, uint8_t alpha);

/**
 * Draw a pixel with alpha blending
 * @param x X coordinate
 * @param y Y coordinate
 * @param color Color with alpha
 */
void gui_draw_pixel_alpha(uint32_t x, uint32_t y, color_t color);

/**
 * Draw a semi-transparent rectangle
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param color Color with alpha
 */
void gui_draw_rect_alpha(int32_t x, int32_t y, uint32_t width, uint32_t height,
                         color_t color);

/**
 * 4D Effects - Time-based animations
 */

/**
 * Animation easing functions
 */
typedef enum {
    EASE_LINEAR,
    EASE_IN_QUAD,
    EASE_OUT_QUAD,
    EASE_IN_OUT_QUAD,
    EASE_IN_CUBIC,
    EASE_OUT_CUBIC,
    EASE_BOUNCE
} ease_type_t;

/**
 * Calculate eased value for animation
 * @param t Time (0.0 to 1.0)
 * @param ease_type Easing function type
 * @return Eased value (0.0 to 1.0)
 */
float gui_ease(float t, ease_type_t ease_type);

/**
 * Interpolate between two values
 * @param start Start value
 * @param end End value
 * @param t Time (0.0 to 1.0)
 * @return Interpolated value
 */
int32_t gui_lerp(int32_t start, int32_t end, float t);

/**
 * Interpolate between two colors
 * @param color1 Start color
 * @param color2 End color
 * @param t Time (0.0 to 1.0)
 * @return Interpolated color
 */
color_t gui_color_lerp(color_t color1, color_t color2, float t);

/**
 * 5D Effects - Interactive and advanced visual effects
 */

/**
 * Apply blur effect to a rectangular region
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param amount Blur amount (1-10)
 */
void gui_apply_blur(int32_t x, int32_t y, uint32_t width, uint32_t height,
                    uint32_t amount);

/**
 * Draw a glow effect around a rectangle
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param color Glow color
 * @param intensity Glow intensity (1-10)
 */
void gui_draw_glow(int32_t x, int32_t y, uint32_t width, uint32_t height,
                   color_t color, uint32_t intensity);

/**
 * Draw a 3D raised button effect
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param color Base color
 * @param pressed Whether button is pressed
 */
void gui_draw_3d_button(int32_t x, int32_t y, uint32_t width, uint32_t height,
                        color_t color, uint8_t pressed);

/**
 * Draw a glass/frosted effect
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param opacity Opacity (0-255)
 */
void gui_draw_glass_effect(int32_t x, int32_t y, uint32_t width, uint32_t height,
                           uint8_t opacity);

/**
 * Particle system for effects
 */
typedef struct {
    float x, y;
    float vx, vy;
    float life;
    color_t color;
} particle_t;

/**
 * Create a particle effect at position
 * @param x X position
 * @param y Y position
 * @param count Number of particles
 * @param color Particle color
 */
void gui_emit_particles(int32_t x, int32_t y, uint32_t count, color_t color);

/**
 * Update all particles
 * @param delta_time Time since last update (in ticks)
 */
void gui_update_particles(uint32_t delta_time);

/**
 * Draw all active particles
 */
void gui_draw_particles(void);

#endif // GUI_EFFECTS_H
