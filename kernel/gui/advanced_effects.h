/**
 * Aurora OS - Advanced Visual Effects
 * 
 * Provides advanced rendering effects including texture mapping, 
 * advanced particles, motion blur, reflections, and dynamic lighting
 */

#ifndef ADVANCED_EFFECTS_H
#define ADVANCED_EFFECTS_H

#include <stdint.h>
#include "framebuffer.h"
#include "gui_effects.h"

/**
 * Texture structure for background mapping
 */
typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t* pixels;  // RGBA pixel data
    uint8_t wrap_mode;  // 0=clamp, 1=repeat, 2=mirror
} texture_t;

/**
 * Particle effect types
 */
typedef enum {
    PARTICLE_FIRE,
    PARTICLE_SMOKE,
    PARTICLE_WATER,
    PARTICLE_SPARKLE,
    PARTICLE_EXPLOSION
} particle_effect_t;

/**
 * Light source structure
 */
typedef struct {
    int32_t x;
    int32_t y;
    color_t color;
    uint32_t radius;
    uint8_t intensity;  // 0-255
} light_source_t;

/**
 * Create a texture from pixel data
 * @param width Texture width
 * @param height Texture height
 * @param pixels Pixel data (RGBA)
 * @param wrap_mode Wrap mode (0=clamp, 1=repeat, 2=mirror)
 * @return Pointer to texture or NULL on failure
 */
texture_t* create_texture(uint32_t width, uint32_t height, uint32_t* pixels, uint8_t wrap_mode);

/**
 * Draw a textured rectangle
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param texture Texture to apply
 * @param u_offset Horizontal texture offset (0.0-1.0)
 * @param v_offset Vertical texture offset (0.0-1.0)
 */
void draw_textured_rect(int32_t x, int32_t y, uint32_t width, uint32_t height, 
                        texture_t* texture, float u_offset, float v_offset);

/**
 * Destroy a texture
 * @param texture Texture to destroy
 */
void destroy_texture(texture_t* texture);

/**
 * Emit advanced particle effect
 * @param x X position
 * @param y Y position
 * @param effect_type Type of particle effect
 * @param count Number of particles to emit
 */
void emit_advanced_particles(int32_t x, int32_t y, particle_effect_t effect_type, uint32_t count);

/**
 * Update advanced particle systems
 * @param delta_time Time since last update in milliseconds
 */
void update_advanced_particles(uint32_t delta_time);

/**
 * Draw all advanced particles
 */
void draw_advanced_particles(void);

/**
 * Apply motion blur to a rectangular region
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param velocity_x Horizontal velocity for blur direction
 * @param velocity_y Vertical velocity for blur direction
 * @param intensity Blur intensity (1-10)
 */
void apply_motion_blur(int32_t x, int32_t y, uint32_t width, uint32_t height,
                       float velocity_x, float velocity_y, uint32_t intensity);

/**
 * Apply screen-space reflection effect
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @param reflection_y Y position of reflection plane
 * @param intensity Reflection intensity (0-255)
 */
void apply_screen_space_reflection(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                   int32_t reflection_y, uint8_t intensity);

/**
 * Add a dynamic light source
 * @param x X position
 * @param y Y position
 * @param color Light color
 * @param radius Light radius
 * @param intensity Light intensity (0-255)
 * @return Light source ID or -1 on failure
 */
int add_light_source(int32_t x, int32_t y, color_t color, uint32_t radius, uint8_t intensity);

/**
 * Remove a light source
 * @param light_id Light source ID
 */
void remove_light_source(int light_id);

/**
 * Update light source position
 * @param light_id Light source ID
 * @param x New X position
 * @param y New Y position
 */
void update_light_position(int light_id, int32_t x, int32_t y);

/**
 * Apply dynamic lighting to a region
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 */
void apply_dynamic_lighting(int32_t x, int32_t y, uint32_t width, uint32_t height);

/**
 * Clear all light sources
 */
void clear_light_sources(void);

/**
 * Initialize advanced effects system
 */
void advanced_effects_init(void);

/**
 * Shutdown advanced effects system
 */
void advanced_effects_shutdown(void);

#endif // ADVANCED_EFFECTS_H
