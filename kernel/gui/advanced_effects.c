/**
 * Aurora OS - Advanced Visual Effects Implementation
 */

#include "advanced_effects.h"
#include "framebuffer.h"
#include "gui_effects.h"
#include <stddef.h>

#define MAX_TEXTURES 16
#define MAX_ADVANCED_PARTICLES 512
#define MAX_LIGHT_SOURCES 16

// Advanced particle structure
typedef struct {
    float x, y;
    float vx, vy;
    float ax, ay;  // Acceleration
    float life;
    float size;
    color_t color;
    particle_effect_t type;
    uint8_t active;
} advanced_particle_t;

// Global state
static struct {
    texture_t textures[MAX_TEXTURES];
    int texture_count;
    advanced_particle_t particles[MAX_ADVANCED_PARTICLES];
    int particle_count;
    light_source_t lights[MAX_LIGHT_SOURCES];
    int light_count;
    uint8_t initialized;
} fx_state = {0};

void advanced_effects_init(void) {
    fx_state.texture_count = 0;
    fx_state.particle_count = 0;
    fx_state.light_count = 0;
    fx_state.initialized = 1;
}

void advanced_effects_shutdown(void) {
    fx_state.initialized = 0;
}

// ============================================================================
// Texture Mapping
// ============================================================================

texture_t* create_texture(uint32_t width, uint32_t height, uint32_t* pixels, uint8_t wrap_mode) {
    if (fx_state.texture_count >= MAX_TEXTURES || !pixels) {
        return NULL;
    }
    
    texture_t* tex = &fx_state.textures[fx_state.texture_count++];
    tex->width = width;
    tex->height = height;
    tex->pixels = pixels;
    tex->wrap_mode = wrap_mode;
    
    return tex;
}

void draw_textured_rect(int32_t x, int32_t y, uint32_t width, uint32_t height,
                        texture_t* texture, float u_offset, float v_offset) {
    if (!texture || !texture->pixels) {
        return;
    }
    
    for (uint32_t py = 0; py < height; py++) {
        for (uint32_t px = 0; px < width; px++) {
            // Calculate texture coordinates
            float u = ((float)px / width) + u_offset;
            float v = ((float)py / height) + v_offset;
            
            // Handle wrapping
            uint32_t tex_x, tex_y;
            if (texture->wrap_mode == 1) {  // Repeat
                u = u - (int)u;
                v = v - (int)v;
                if (u < 0.0f) u += 1.0f;
                if (v < 0.0f) v += 1.0f;
            } else if (texture->wrap_mode == 2) {  // Mirror
                int iu = (int)u;
                int iv = (int)v;
                u = u - iu;
                v = v - iv;
                if (iu % 2) u = 1.0f - u;
                if (iv % 2) v = 1.0f - v;
            } else {  // Clamp
                if (u < 0.0f) u = 0.0f;
                if (u > 1.0f) u = 1.0f;
                if (v < 0.0f) v = 0.0f;
                if (v > 1.0f) v = 1.0f;
            }
            
            tex_x = (uint32_t)(u * texture->width) % texture->width;
            tex_y = (uint32_t)(v * texture->height) % texture->height;
            
            uint32_t pixel = texture->pixels[tex_y * texture->width + tex_x];
            
            // Extract RGBA
            color_t color;
            color.r = (pixel >> 24) & 0xFF;
            color.g = (pixel >> 16) & 0xFF;
            color.b = (pixel >> 8) & 0xFF;
            color.a = pixel & 0xFF;
            
            if (color.a > 0) {
                if (color.a == 255) {
                    framebuffer_draw_pixel(x + px, y + py, color);
                } else {
                    gui_draw_pixel_alpha(x + px, y + py, color);
                }
            }
        }
    }
}

void destroy_texture(texture_t* texture) {
    if (texture) {
        texture->pixels = NULL;
    }
}

// ============================================================================
// Advanced Particle Effects
// ============================================================================

void emit_advanced_particles(int32_t x, int32_t y, particle_effect_t effect_type, uint32_t count) {
    for (uint32_t i = 0; i < count && fx_state.particle_count < MAX_ADVANCED_PARTICLES; i++) {
        // Find free slot
        int slot = -1;
        for (int j = 0; j < MAX_ADVANCED_PARTICLES; j++) {
            if (!fx_state.particles[j].active) {
                slot = j;
                break;
            }
        }
        
        if (slot == -1) break;
        
        advanced_particle_t* p = &fx_state.particles[slot];
        p->x = (float)x;
        p->y = (float)y;
        p->type = effect_type;
        p->active = 1;
        p->life = 1.0f;
        
        // Set properties based on effect type
        switch (effect_type) {
            case PARTICLE_FIRE:
                p->vx = ((float)(i % 20) - 10.0f) * 0.2f;
                p->vy = -2.0f - (float)(i % 10) * 0.3f;
                p->ax = 0.0f;
                p->ay = 0.1f;  // Slight upward drift
                p->size = 3.0f + (float)(i % 5);
                p->color = (color_t){255, 150 + (i % 100), 0, 255};
                break;
                
            case PARTICLE_SMOKE:
                p->vx = ((float)(i % 30) - 15.0f) * 0.1f;
                p->vy = -1.5f - (float)(i % 8) * 0.2f;
                p->ax = 0.0f;
                p->ay = -0.05f;  // Float upward
                p->size = 4.0f + (float)(i % 6);
                p->color = (color_t){80 + (i % 40), 80 + (i % 40), 80 + (i % 40), 200};
                break;
                
            case PARTICLE_WATER:
                p->vx = ((float)(i % 40) - 20.0f) * 0.3f;
                p->vy = -3.0f - (float)(i % 15) * 0.2f;
                p->ax = 0.0f;
                p->ay = 0.5f;  // Gravity
                p->size = 2.0f + (float)(i % 4);
                p->color = (color_t){100, 150, 255, 200};
                break;
                
            case PARTICLE_SPARKLE:
                p->vx = ((float)(i % 60) - 30.0f) * 0.2f;
                p->vy = ((float)(i % 60) - 30.0f) * 0.2f;
                p->ax = 0.0f;
                p->ay = 0.0f;
                p->size = 1.0f + (float)(i % 3);
                p->color = (color_t){255, 255, 200 + (i % 55), 255};
                break;
                
            case PARTICLE_EXPLOSION:
                float angle = ((float)i / count) * 6.28f;  // 2*PI
                float speed = 2.0f + (float)(i % 10) * 0.3f;
                p->vx = speed * ((angle * 100.0f) - (int)(angle * 100.0f)) * 0.01f;  // Approximation of cos
                p->vy = speed * ((angle * 100.0f + 157.0f) - (int)(angle * 100.0f + 157.0f)) * 0.01f;  // Approximation of sin
                p->ax = -p->vx * 0.1f;
                p->ay = -p->vy * 0.1f;
                p->size = 2.0f + (float)(i % 5);
                p->color = (color_t){255, 100 + (i % 155), 0, 255};
                break;
        }
        
        if (fx_state.particle_count <= slot) {
            fx_state.particle_count = slot + 1;
        }
    }
}

void update_advanced_particles(uint32_t delta_time) {
    float dt = (float)delta_time / 1000.0f;
    
    for (int i = 0; i < fx_state.particle_count; i++) {
        advanced_particle_t* p = &fx_state.particles[i];
        if (!p->active) continue;
        
        // Update velocity
        p->vx += p->ax * dt;
        p->vy += p->ay * dt;
        
        // Update position
        p->x += p->vx;
        p->y += p->vy;
        
        // Update life
        float life_decay = 0.3f;
        if (p->type == PARTICLE_SMOKE) life_decay = 0.15f;
        if (p->type == PARTICLE_SPARKLE) life_decay = 0.5f;
        
        p->life -= life_decay * dt;
        
        // Deactivate if dead
        if (p->life <= 0.0f) {
            p->active = 0;
        }
    }
}

void draw_advanced_particles(void) {
    for (int i = 0; i < fx_state.particle_count; i++) {
        advanced_particle_t* p = &fx_state.particles[i];
        if (!p->active) continue;
        
        // Apply life-based alpha
        color_t c = p->color;
        c.a = (uint8_t)(c.a * p->life);
        
        // Draw particle based on size
        int32_t px = (int32_t)p->x;
        int32_t py = (int32_t)p->y;
        int32_t size = (int32_t)p->size;
        
        for (int dy = -size; dy <= size; dy++) {
            for (int dx = -size; dx <= size; dx++) {
                if (dx * dx + dy * dy <= size * size) {
                    gui_draw_pixel_alpha(px + dx, py + dy, c);
                }
            }
        }
    }
}

// ============================================================================
// Motion Blur
// ============================================================================

void apply_motion_blur(int32_t x, int32_t y, uint32_t width, uint32_t height,
                       float velocity_x, float velocity_y, uint32_t intensity) {
    if (intensity < 1) intensity = 1;
    if (intensity > 10) intensity = 10;
    
    framebuffer_info_t* fb_info = framebuffer_get_info();
    if (!fb_info || !fb_info->address) return;
    
    // Simplified motion blur: sample along velocity direction
    int samples = intensity;
    float step_x = velocity_x / samples;
    float step_y = velocity_y / samples;
    
    // Process each pixel in the region
    for (uint32_t py = 0; py < height; py++) {
        for (uint32_t px = 0; px < width; px++) {
            uint32_t sum_r = 0, sum_g = 0, sum_b = 0;
            
            // Sample along motion vector
            for (int s = 0; s < samples; s++) {
                int32_t sample_x = x + px + (int32_t)(step_x * s);
                int32_t sample_y = y + py + (int32_t)(step_y * s);
                
                if (sample_x >= 0 && sample_x < (int32_t)fb_info->width &&
                    sample_y >= 0 && sample_y < (int32_t)fb_info->height) {
                    
                    uint32_t offset = sample_y * (fb_info->pitch / 4) + sample_x;
                    uint32_t pixel = fb_info->address[offset];
                    
                    sum_r += (pixel >> 16) & 0xFF;
                    sum_g += (pixel >> 8) & 0xFF;
                    sum_b += pixel & 0xFF;
                }
            }
            
            // Average and write back
            color_t blurred;
            blurred.r = (uint8_t)(sum_r / samples);
            blurred.g = (uint8_t)(sum_g / samples);
            blurred.b = (uint8_t)(sum_b / samples);
            blurred.a = 255;
            
            framebuffer_draw_pixel(x + px, y + py, blurred);
        }
    }
}

// ============================================================================
// Screen-Space Reflections
// ============================================================================

void apply_screen_space_reflection(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                   int32_t reflection_y, uint8_t intensity) {
    framebuffer_info_t* fb_info = framebuffer_get_info();
    if (!fb_info || !fb_info->address) return;
    
    // Draw reflection below the object
    for (uint32_t py = 0; py < height; py++) {
        for (uint32_t px = 0; px < width; px++) {
            int32_t src_y = y + py;
            int32_t dst_y = reflection_y + (reflection_y - src_y);
            
            if (src_y >= 0 && src_y < (int32_t)fb_info->height &&
                dst_y >= 0 && dst_y < (int32_t)fb_info->height &&
                x + (int32_t)px >= 0 && x + (int32_t)px < (int32_t)fb_info->width) {
                
                uint32_t src_offset = src_y * (fb_info->pitch / 4) + x + px;
                uint32_t pixel = fb_info->address[src_offset];
                
                // Extract RGB and apply fade based on distance
                float fade = 1.0f - ((float)py / height);
                fade *= ((float)intensity / 255.0f);
                
                color_t reflected;
                reflected.r = (uint8_t)(((pixel >> 16) & 0xFF) * fade);
                reflected.g = (uint8_t)(((pixel >> 8) & 0xFF) * fade);
                reflected.b = (uint8_t)((pixel & 0xFF) * fade);
                reflected.a = (uint8_t)(intensity * fade);
                
                gui_draw_pixel_alpha(x + px, dst_y, reflected);
            }
        }
    }
}

// ============================================================================
// Dynamic Lighting
// ============================================================================

int add_light_source(int32_t x, int32_t y, color_t color, uint32_t radius, uint8_t intensity) {
    if (fx_state.light_count >= MAX_LIGHT_SOURCES) {
        return -1;
    }
    
    int id = fx_state.light_count++;
    fx_state.lights[id].x = x;
    fx_state.lights[id].y = y;
    fx_state.lights[id].color = color;
    fx_state.lights[id].radius = radius;
    fx_state.lights[id].intensity = intensity;
    
    return id;
}

void remove_light_source(int light_id) {
    if (light_id < 0 || light_id >= fx_state.light_count) {
        return;
    }
    
    // Shift remaining lights
    for (int i = light_id; i < fx_state.light_count - 1; i++) {
        fx_state.lights[i] = fx_state.lights[i + 1];
    }
    fx_state.light_count--;
}

void update_light_position(int light_id, int32_t x, int32_t y) {
    if (light_id < 0 || light_id >= fx_state.light_count) {
        return;
    }
    
    fx_state.lights[light_id].x = x;
    fx_state.lights[light_id].y = y;
}

void apply_dynamic_lighting(int32_t x, int32_t y, uint32_t width, uint32_t height) {
    framebuffer_info_t* fb_info = framebuffer_get_info();
    if (!fb_info || !fb_info->address) return;
    
    // Apply lighting from each light source
    for (int l = 0; l < fx_state.light_count; l++) {
        light_source_t* light = &fx_state.lights[l];
        
        for (uint32_t py = 0; py < height; py++) {
            for (uint32_t px = 0; px < width; px++) {
                int32_t pixel_x = x + px;
                int32_t pixel_y = y + py;
                
                // Calculate distance to light
                int32_t dx = pixel_x - light->x;
                int32_t dy = pixel_y - light->y;
                int32_t dist_sq = dx * dx + dy * dy;
                int32_t radius_sq = light->radius * light->radius;
                
                if (dist_sq < radius_sq) {
                    // Calculate light intensity (inverse square falloff)
                    float falloff = 1.0f - ((float)dist_sq / radius_sq);
                    falloff = falloff * falloff;  // Quadratic falloff
                    
                    uint8_t light_alpha = (uint8_t)(light->intensity * falloff);
                    
                    // Apply light color
                    color_t light_color = light->color;
                    light_color.a = light_alpha;
                    
                    gui_draw_pixel_alpha(pixel_x, pixel_y, light_color);
                }
            }
        }
    }
}

void clear_light_sources(void) {
    fx_state.light_count = 0;
}
