/**
 * Aurora OS - GUI Advanced Effects Implementation
 * 
 * Implements 3D depth, 4D (animation), and 5D (interactive) effects
 */

#include "gui_effects.h"
#include "framebuffer.h"
#include "../memory/memory.h"

// Math helper functions
static float fabs_custom(float x) {
    return (x < 0.0f) ? -x : x;
}

static float sqrt_custom(float x) {
    if (x <= 0.0f) return 0.0f;
    float result = x;
    float prev;
    // Newton's method for square root
    for (int i = 0; i < 10; i++) {
        prev = result;
        result = (result + x / result) / 2.0f;
        if (fabs_custom(result - prev) < 0.0001f) break;
    }
    return result;
}

// ============================================================================
// 3D Depth Effects
// ============================================================================

color_t gui_alpha_blend(color_t fg, color_t bg, uint8_t alpha) {
    color_t result;
    uint32_t inv_alpha = 255 - alpha;
    
    result.r = (fg.r * alpha + bg.r * inv_alpha) / 255;
    result.g = (fg.g * alpha + bg.g * inv_alpha) / 255;
    result.b = (fg.b * alpha + bg.b * inv_alpha) / 255;
    result.a = 255;
    
    return result;
}

void gui_draw_pixel_alpha(uint32_t x, uint32_t y, color_t color) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb || x >= fb->width || y >= fb->height) return;
    
    if (color.a == 255) {
        // Fully opaque, just draw directly
        framebuffer_draw_pixel(x, y, color);
    } else if (color.a > 0) {
        // Get background pixel and blend
        // For simplicity, we'll assume a background color
        // In a real implementation, we'd read from framebuffer
        color_t bg = {0, 0, 0, 255};  // Assume black background
        color_t blended = gui_alpha_blend(color, bg, color.a);
        framebuffer_draw_pixel(x, y, blended);
    }
}

void gui_draw_rect_alpha(int32_t x, int32_t y, uint32_t width, uint32_t height,
                         color_t color) {
    for (uint32_t dy = 0; dy < height; dy++) {
        for (uint32_t dx = 0; dx < width; dx++) {
            gui_draw_pixel_alpha(x + dx, y + dy, color);
        }
    }
}

void gui_draw_shadow(int32_t x, int32_t y, uint32_t width, uint32_t height,
                     uint32_t offset, uint32_t blur) {
    // Clamp blur to reasonable values
    if (blur > 20) blur = 20;
    if (blur < 1) blur = 1;
    
    // Shadow color (black with varying alpha)
    color_t shadow_base = {0, 0, 0, 255};
    
    int32_t shadow_x = x + offset;
    int32_t shadow_y = y + offset;
    uint32_t shadow_width = width;
    uint32_t shadow_height = height;
    
    // Draw shadow with gradient alpha (simulated blur)
    for (uint32_t b = 0; b < blur; b++) {
        uint8_t alpha = (255 * (blur - b)) / (blur * 3);  // Fade out
        if (alpha < 10) alpha = 10;
        
        color_t shadow_color = shadow_base;
        shadow_color.a = alpha;
        
        // Draw shadow layer
        for (uint32_t dy = 0; dy < shadow_height; dy++) {
            for (uint32_t dx = 0; dx < shadow_width; dx++) {
                int32_t px = shadow_x + dx - b;
                int32_t py = shadow_y + dy - b;
                
                // Fade at edges
                float edge_factor = 1.0f;
                if (dx < blur) edge_factor *= (float)dx / blur;
                if (dy < blur) edge_factor *= (float)dy / blur;
                if (dx >= shadow_width - blur) edge_factor *= (float)(shadow_width - dx) / blur;
                if (dy >= shadow_height - blur) edge_factor *= (float)(shadow_height - dy) / blur;
                
                color_t edge_shadow = shadow_color;
                edge_shadow.a = (uint8_t)(shadow_color.a * edge_factor);
                
                if (edge_shadow.a > 0) {
                    framebuffer_draw_pixel(px, py, edge_shadow);
                }
            }
        }
    }
}

void gui_draw_gradient(int32_t x, int32_t y, uint32_t width, uint32_t height,
                       color_t color1, color_t color2) {
    for (uint32_t dy = 0; dy < height; dy++) {
        float t = (float)dy / (float)height;
        
        color_t line_color;
        line_color.r = (uint8_t)(color1.r + (color2.r - color1.r) * t);
        line_color.g = (uint8_t)(color1.g + (color2.g - color1.g) * t);
        line_color.b = (uint8_t)(color1.b + (color2.b - color1.b) * t);
        line_color.a = 255;
        
        framebuffer_draw_hline(x, x + width - 1, y + dy, line_color);
    }
}

void gui_draw_rounded_rect(int32_t x, int32_t y, uint32_t width, uint32_t height,
                           uint32_t radius, color_t color) {
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;
    
    // Draw main body
    framebuffer_draw_rect(x + radius, y, width - 2 * radius, height, color);
    framebuffer_draw_rect(x, y + radius, radius, height - 2 * radius, color);
    framebuffer_draw_rect(x + width - radius, y + radius, radius, height - 2 * radius, color);
    
    // Draw rounded corners (simplified - use circles)
    // Top-left corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = radius - cx;
            int32_t dy = radius - cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                framebuffer_draw_pixel(x + cx, y + cy, color);
            }
        }
    }
    
    // Top-right corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = cx;
            int32_t dy = radius - cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                framebuffer_draw_pixel(x + width - radius + cx, y + cy, color);
            }
        }
    }
    
    // Bottom-left corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = radius - cx;
            int32_t dy = cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                framebuffer_draw_pixel(x + cx, y + height - radius + cy, color);
            }
        }
    }
    
    // Bottom-right corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = cx;
            int32_t dy = cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                framebuffer_draw_pixel(x + width - radius + cx, y + height - radius + cy, color);
            }
        }
    }
}

// ============================================================================
// 4D Effects - Animations
// ============================================================================

float gui_ease(float t, ease_type_t ease_type) {
    // Clamp t to [0, 1]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    switch (ease_type) {
        case EASE_LINEAR:
            return t;
            
        case EASE_IN_QUAD:
            return t * t;
            
        case EASE_OUT_QUAD:
            return t * (2.0f - t);
            
        case EASE_IN_OUT_QUAD:
            if (t < 0.5f) {
                return 2.0f * t * t;
            } else {
                return -1.0f + (4.0f - 2.0f * t) * t;
            }
            
        case EASE_IN_CUBIC:
            return t * t * t;
            
        case EASE_OUT_CUBIC: {
            float f = t - 1.0f;
            return f * f * f + 1.0f;
        }
            
        case EASE_BOUNCE: {
            if (t < 0.5f) {
                return 0.5f * (1.0f - (1.0f - 2.0f * t) * (1.0f - 2.0f * t));
            } else {
                return 0.5f + 0.5f * (2.0f * t - 1.0f) * (2.0f * t - 1.0f);
            }
        }
            
        default:
            return t;
    }
}

int32_t gui_lerp(int32_t start, int32_t end, float t) {
    return start + (int32_t)((end - start) * t);
}

color_t gui_color_lerp(color_t color1, color_t color2, float t) {
    color_t result;
    result.r = (uint8_t)(color1.r + (color2.r - color1.r) * t);
    result.g = (uint8_t)(color1.g + (color2.g - color1.g) * t);
    result.b = (uint8_t)(color1.b + (color2.b - color1.b) * t);
    result.a = (uint8_t)(color1.a + (color2.a - color1.a) * t);
    return result;
}

// ============================================================================
// 5D Effects - Interactive and Advanced
// ============================================================================

void gui_apply_blur(int32_t x, int32_t y, uint32_t width, uint32_t height,
                    uint32_t amount) {
    // Simple box blur implementation
    // Note: In a real implementation, we'd read from and write to framebuffer
    // For now, this is a placeholder showing the concept
    if (amount < 1) amount = 1;
    if (amount > 10) amount = 10;
    
    // Draw semi-transparent overlay to simulate blur
    color_t blur_color = {200, 200, 200, 50};
    gui_draw_rect_alpha(x, y, width, height, blur_color);
}

void gui_draw_glow(int32_t x, int32_t y, uint32_t width, uint32_t height,
                   color_t color, uint32_t intensity) {
    if (intensity < 1) intensity = 1;
    if (intensity > 10) intensity = 10;
    
    // Draw multiple layers with decreasing alpha
    for (uint32_t i = 0; i < intensity; i++) {
        int32_t offset = i * 2;
        uint8_t alpha = (255 * (intensity - i)) / (intensity * 2);
        
        color_t glow_color = color;
        glow_color.a = alpha;
        
        gui_draw_rect_alpha(x - offset, y - offset, 
                          width + 2 * offset, height + 2 * offset,
                          glow_color);
    }
}

void gui_draw_3d_button(int32_t x, int32_t y, uint32_t width, uint32_t height,
                        color_t color, uint8_t pressed) {
    if (pressed) {
        // Pressed state - darker, inset look
        color_t dark = color;
        dark.r = (uint8_t)(color.r * 0.7f);
        dark.g = (uint8_t)(color.g * 0.7f);
        dark.b = (uint8_t)(color.b * 0.7f);
        
        // Draw main button
        framebuffer_draw_rect(x + 2, y + 2, width - 4, height - 4, dark);
        
        // Draw inner shadow (dark borders)
        color_t shadow = {0, 0, 0, 100};
        framebuffer_draw_hline(x + 1, x + width - 2, y + 1, shadow);
        framebuffer_draw_vline(x + 1, y + 1, y + height - 2, shadow);
    } else {
        // Normal state - lighter, raised look
        // Draw main button
        framebuffer_draw_rect(x, y, width, height, color);
        
        // Draw highlight (top and left)
        color_t highlight = color;
        highlight.r = (uint8_t)(color.r + (255 - color.r) * 0.3f);
        highlight.g = (uint8_t)(color.g + (255 - color.g) * 0.3f);
        highlight.b = (uint8_t)(color.b + (255 - color.b) * 0.3f);
        
        framebuffer_draw_hline(x, x + width - 1, y, highlight);
        framebuffer_draw_vline(x, y, y + height - 1, highlight);
        
        // Draw shadow (bottom and right)
        color_t shadow = color;
        shadow.r = (uint8_t)(color.r * 0.7f);
        shadow.g = (uint8_t)(color.g * 0.7f);
        shadow.b = (uint8_t)(color.b * 0.7f);
        
        framebuffer_draw_hline(x + 1, x + width, y + height - 1, shadow);
        framebuffer_draw_vline(x + width - 1, y + 1, y + height, shadow);
        
        // Draw outer shadow for depth
        gui_draw_shadow(x, y, width, height, 3, 3);
    }
}

void gui_draw_glass_effect(int32_t x, int32_t y, uint32_t width, uint32_t height,
                           uint8_t opacity) {
    // Draw semi-transparent white overlay
    color_t glass_color = {255, 255, 255, opacity};
    gui_draw_rect_alpha(x, y, width, height, glass_color);
    
    // Add subtle gradient for depth
    color_t top_color = {255, 255, 255, opacity};
    color_t bottom_color = {200, 200, 200, opacity / 2};
    gui_draw_gradient(x, y, width, height, top_color, bottom_color);
    
    // Add highlight at top
    color_t highlight = {255, 255, 255, opacity + 50};
    if (highlight.a > 255) highlight.a = 255;
    framebuffer_draw_hline(x, x + width - 1, y, highlight);
    framebuffer_draw_hline(x, x + width - 1, y + 1, highlight);
}

// ============================================================================
// Particle System
// ============================================================================

#define MAX_PARTICLES 256

static particle_t particles[MAX_PARTICLES];
static uint32_t particle_count = 0;

void gui_emit_particles(int32_t x, int32_t y, uint32_t count, color_t color) {
    for (uint32_t i = 0; i < count && particle_count < MAX_PARTICLES; i++) {
        particle_t* p = &particles[particle_count++];
        
        p->x = (float)x;
        p->y = (float)y;
        
        // Random velocity (simplified - use simple pseudo-random)
        float angle = (float)(i * 360) / count;
        float speed = 2.0f + (float)(i % 3);
        
        p->vx = speed * (angle / 360.0f - 0.5f) * 2.0f;
        p->vy = -speed * (1.0f - angle / 360.0f);
        
        p->life = 1.0f;
        p->color = color;
    }
}

void gui_update_particles(uint32_t delta_time) {
    (void)delta_time;  // Not used in simple implementation
    
    // Update all particles
    uint32_t active = 0;
    for (uint32_t i = 0; i < particle_count; i++) {
        particle_t* p = &particles[i];
        
        // Update position
        p->x += p->vx;
        p->y += p->vy;
        
        // Apply gravity
        p->vy += 0.1f;
        
        // Update life
        p->life -= 0.02f;
        
        // Keep active particles
        if (p->life > 0.0f) {
            if (i != active) {
                particles[active] = *p;
            }
            active++;
        }
    }
    
    particle_count = active;
}

void gui_draw_particles(void) {
    for (uint32_t i = 0; i < particle_count; i++) {
        particle_t* p = &particles[i];
        
        color_t c = p->color;
        c.a = (uint8_t)(255 * p->life);
        
        // Draw particle as small circle (3x3 pixels)
        int32_t px = (int32_t)p->x;
        int32_t py = (int32_t)p->y;
        
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx * dx + dy * dy <= 1) {
                    gui_draw_pixel_alpha(px + dx, py + dy, c);
                }
            }
        }
    }
}
