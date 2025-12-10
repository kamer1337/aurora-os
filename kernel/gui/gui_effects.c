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

void gui_draw_gradient_horizontal(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                   color_t color1, color_t color2) {
    for (uint32_t dx = 0; dx < width; dx++) {
        float t = (float)dx / (float)width;
        
        color_t line_color;
        line_color.r = (uint8_t)(color1.r + (color2.r - color1.r) * t);
        line_color.g = (uint8_t)(color1.g + (color2.g - color1.g) * t);
        line_color.b = (uint8_t)(color1.b + (color2.b - color1.b) * t);
        line_color.a = 255;
        
        framebuffer_draw_vline(x + dx, y, y + height - 1, line_color);
    }
}

void gui_draw_gradient_radial(int32_t x, int32_t y, uint32_t radius,
                               color_t color1, color_t color2) {
    int32_t r_squared = (int32_t)(radius * radius);
    
    for (int32_t dy = -(int32_t)radius; dy <= (int32_t)radius; dy++) {
        for (int32_t dx = -(int32_t)radius; dx <= (int32_t)radius; dx++) {
            int32_t dist_squared = dx * dx + dy * dy;
            
            if (dist_squared <= r_squared) {
                float dist = sqrt_custom((float)dist_squared);
                float t = dist / (float)radius;
                
                // Clamp t
                if (t > 1.0f) t = 1.0f;
                
                color_t pixel_color = gui_color_lerp(color1, color2, t);
                framebuffer_draw_pixel(x + dx, y + dy, pixel_color);
            }
        }
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

void gui_draw_rounded_rect_alpha(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                  uint32_t radius, color_t color) {
    if (radius > width / 2) radius = width / 2;
    if (radius > height / 2) radius = height / 2;
    
    // Draw main body with alpha
    gui_draw_rect_alpha(x + radius, y, width - 2 * radius, height, color);
    gui_draw_rect_alpha(x, y + radius, radius, height - 2 * radius, color);
    gui_draw_rect_alpha(x + width - radius, y + radius, radius, height - 2 * radius, color);
    
    // Draw rounded corners with alpha
    // Top-left corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = radius - cx;
            int32_t dy = radius - cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                gui_draw_pixel_alpha(x + cx, y + cy, color);
            }
        }
    }
    
    // Top-right corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = cx;
            int32_t dy = radius - cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                gui_draw_pixel_alpha(x + width - radius + cx, y + cy, color);
            }
        }
    }
    
    // Bottom-left corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = radius - cx;
            int32_t dy = cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                gui_draw_pixel_alpha(x + cx, y + height - radius + cy, color);
            }
        }
    }
    
    // Bottom-right corner
    for (uint32_t cy = 0; cy < radius; cy++) {
        for (uint32_t cx = 0; cx < radius; cx++) {
            int32_t dx = cx;
            int32_t dy = cy;
            if (dx * dx + dy * dy <= (int32_t)(radius * radius)) {
                gui_draw_pixel_alpha(x + width - radius + cx, y + height - radius + cy, color);
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
        
        case EASE_ELASTIC: {
            // Elastic easing: overshoots with oscillation
            if (t == 0.0f) return 0.0f;
            if (t == 1.0f) return 1.0f;
            
            float p = 0.3f;
            float s = p / 4.0f;
            float post = t - 1.0f;
            
            // Simplified elastic without pow: use quadratic approximation
            float amp = 1.0f - (post * post * 4.0f);  // Approximation of decay
            if (amp < 0.0f) amp = 0.0f;
            
            // Sine approximation using Taylor series (first 3 terms)
            float angle = (post * 13.0f);  // Frequency
            float sine = angle - (angle * angle * angle) / 6.0f;  // sin(x) ≈ x - x³/6
            
            return amp * sine + 1.0f;
        }
        
        case EASE_BACK: {
            // Back easing: overshoots slightly past target
            float c1 = 1.70158f;
            float c3 = c1 + 1.0f;
            
            return c3 * t * t * t - c1 * t * t;
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
    if (amount < 1) amount = 1;
    if (amount > 10) amount = 10;
    
    // Get framebuffer info
    framebuffer_info_t* fb_info = framebuffer_get_info();
    if (!fb_info || !fb_info->address) {
        return;
    }
    
    // Ensure coordinates are within bounds
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + (int32_t)width > (int32_t)fb_info->width) width = fb_info->width - x;
    if (y + (int32_t)height > (int32_t)fb_info->height) height = fb_info->height - y;
    
    // Use a 3x3 Gaussian kernel for blur
    // Kernel: [1 2 1]
    //         [2 4 2]
    //         [1 2 1]
    // Sum = 16
    
    // Allocate temporary buffer for the region (simplified - use stack for small regions)
    // For larger regions, we'd need dynamic allocation
    uint32_t max_size = 256 * 256;  // Maximum 256x256 region
    if (width * height > max_size) {
        // Fall back to simple overlay for large regions
        color_t blur_color = {200, 200, 200, 50};
        gui_draw_rect_alpha(x, y, width, height, blur_color);
        return;
    }
    
    // Apply blur multiple times based on amount
    for (uint32_t iter = 0; iter < amount; iter++) {
        // Apply 3x3 Gaussian kernel convolution
        for (uint32_t py = 1; py < height - 1; py++) {
            for (uint32_t px = 1; px < width - 1; px++) {
                uint32_t abs_x = x + px;
                uint32_t abs_y = y + py;
                
                // Sample 3x3 neighborhood
                uint32_t sum_r = 0, sum_g = 0, sum_b = 0;
                
                // Get pixel values and apply kernel weights
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        uint32_t sx = abs_x + kx;
                        uint32_t sy = abs_y + ky;
                        uint32_t offset = sy * (fb_info->pitch / 4) + sx;
                        uint32_t pixel = fb_info->address[offset];
                        
                        // Extract RGB
                        uint8_t r = (pixel >> 16) & 0xFF;
                        uint8_t g = (pixel >> 8) & 0xFF;
                        uint8_t b = pixel & 0xFF;
                        
                        // Apply kernel weight
                        uint32_t weight = 1;
                        if (kx == 0 && ky == 0) weight = 4;  // Center
                        else if (kx == 0 || ky == 0) weight = 2;  // Edges
                        // Corners = 1
                        
                        sum_r += r * weight;
                        sum_g += g * weight;
                        sum_b += b * weight;
                    }
                }
                
                // Average by dividing by kernel sum (16)
                color_t blurred;
                blurred.r = (uint8_t)(sum_r / 16);
                blurred.g = (uint8_t)(sum_g / 16);
                blurred.b = (uint8_t)(sum_b / 16);
                blurred.a = 255;
                
                // Write back (only for every other iteration to avoid artifacts)
                if (iter % 2 == 0 || iter == amount - 1) {
                    framebuffer_draw_pixel(abs_x, abs_y, blurred);
                }
            }
        }
    }
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

// ============================================================================
// Sprite System
// ============================================================================

sprite_t* gui_create_sprite(uint32_t width, uint32_t height, uint32_t* pixels) {
    if (!width || !height || !pixels) {
        return 0;
    }
    
    // For now, use a static sprite pool (in real implementation, use dynamic allocation)
    static sprite_t sprite_pool[16];
    static int sprite_count = 0;
    
    if (sprite_count >= 16) {
        return 0;  // Pool exhausted
    }
    
    sprite_t* sprite = &sprite_pool[sprite_count++];
    sprite->width = width;
    sprite->height = height;
    sprite->pixels = pixels;
    
    return sprite;
}

void gui_draw_sprite(sprite_t* sprite, int32_t x, int32_t y) {
    if (!sprite || !sprite->pixels) {
        return;
    }
    
    for (uint32_t py = 0; py < sprite->height; py++) {
        for (uint32_t px = 0; px < sprite->width; px++) {
            uint32_t pixel = sprite->pixels[py * sprite->width + px];
            
            // Extract RGBA
            color_t color;
            color.r = (pixel >> 24) & 0xFF;
            color.g = (pixel >> 16) & 0xFF;
            color.b = (pixel >> 8) & 0xFF;
            color.a = pixel & 0xFF;
            
            // Draw with alpha blending if alpha < 255
            if (color.a == 255) {
                framebuffer_draw_pixel(x + px, y + py, color);
            } else if (color.a > 0) {
                gui_draw_pixel_alpha(x + px, y + py, color);
            }
        }
    }
}

void gui_draw_sprite_alpha(sprite_t* sprite, int32_t x, int32_t y, uint8_t alpha) {
    if (!sprite || !sprite->pixels) {
        return;
    }
    
    for (uint32_t py = 0; py < sprite->height; py++) {
        for (uint32_t px = 0; px < sprite->width; px++) {
            uint32_t pixel = sprite->pixels[py * sprite->width + px];
            
            // Extract RGBA
            color_t color;
            color.r = (pixel >> 24) & 0xFF;
            color.g = (pixel >> 16) & 0xFF;
            color.b = (pixel >> 8) & 0xFF;
            color.a = ((pixel & 0xFF) * alpha) / 255;
            
            if (color.a > 0) {
                gui_draw_pixel_alpha(x + px, y + py, color);
            }
        }
    }
}

void gui_draw_sprite_scaled(sprite_t* sprite, int32_t x, int32_t y, float scale_x, float scale_y) {
    if (!sprite || !sprite->pixels || scale_x <= 0.0f || scale_y <= 0.0f) {
        return;
    }
    
    uint32_t scaled_width = (uint32_t)(sprite->width * scale_x);
    uint32_t scaled_height = (uint32_t)(sprite->height * scale_y);
    
    for (uint32_t py = 0; py < scaled_height; py++) {
        for (uint32_t px = 0; px < scaled_width; px++) {
            // Nearest neighbor sampling
            uint32_t src_x = (uint32_t)(px / scale_x);
            uint32_t src_y = (uint32_t)(py / scale_y);
            
            if (src_x < sprite->width && src_y < sprite->height) {
                uint32_t pixel = sprite->pixels[src_y * sprite->width + src_x];
                
                // Extract RGBA
                color_t color;
                color.r = (pixel >> 24) & 0xFF;
                color.g = (pixel >> 16) & 0xFF;
                color.b = (pixel >> 8) & 0xFF;
                color.a = pixel & 0xFF;
                
                if (color.a == 255) {
                    framebuffer_draw_pixel(x + px, y + py, color);
                } else if (color.a > 0) {
                    gui_draw_pixel_alpha(x + px, y + py, color);
                }
            }
        }
    }
}

void gui_destroy_sprite(sprite_t* sprite) {
    // In a real implementation with dynamic allocation, free memory here
    // For now with static pool, just mark as unused (simplified)
    if (sprite) {
        sprite->pixels = 0;
    }
}

// ============================================================================
// Layered Window Rendering - 5D Depth
// ============================================================================

void gui_draw_window_with_depth(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                 float depth_offset, color_t color, const char* title) {
    // Clamp depth to [0, 1]
    if (depth_offset < 0.0f) depth_offset = 0.0f;
    if (depth_offset > 1.0f) depth_offset = 1.0f;
    
    // Calculate depth effects
    float scale = 1.0f - (depth_offset * 0.3f);  // Scale from 1.0 to 0.7
    uint8_t alpha = (uint8_t)(255 - (depth_offset * 100));  // Alpha from 255 to 155
    
    // Apply perspective offset (windows further back appear lower and to the right)
    int32_t offset_x = (int32_t)(depth_offset * 30);
    int32_t offset_y = (int32_t)(depth_offset * 20);
    
    // Scaled dimensions
    uint32_t scaled_width = (uint32_t)(width * scale);
    uint32_t scaled_height = (uint32_t)(height * scale);
    
    // Adjusted position
    int32_t draw_x = x + offset_x;
    int32_t draw_y = y + offset_y;
    
    // Draw shadow with reduced intensity for background windows
    uint32_t shadow_offset = (uint32_t)(5 * scale);
    uint32_t shadow_blur = (uint32_t)(8 * scale);
    gui_draw_shadow(draw_x, draw_y, scaled_width, scaled_height, shadow_offset, shadow_blur);
    
    // Draw window with rounded corners and transparency
    color_t window_color = color;
    window_color.a = alpha;
    gui_draw_rounded_rect_alpha(draw_x, draw_y, scaled_width, scaled_height, 12, window_color);
    
    // Draw titlebar with gradient
    uint32_t title_height = (uint32_t)(24 * scale);
    color_t title_color1 = {70, 130, 220, alpha};
    color_t title_color2 = {40, 90, 180, alpha};
    
    // Draw titlebar background with rounded top corners
    gui_draw_gradient(draw_x + 1, draw_y + 1, scaled_width - 2, title_height, 
                     title_color1, title_color2);
    
    // Draw title text if provided (scale text is complex, so simplified)
    if (title && depth_offset < 0.5f) {  // Only draw text for front windows
        framebuffer_draw_string(draw_x + 8, draw_y + 8, title, 
                              COLOR_WHITE, title_color1);
    }
    
    // Draw subtle glow for foreground windows
    if (depth_offset < 0.3f) {
        color_t glow_color = {100, 150, 255, (uint8_t)(50 * (1.0f - depth_offset))};
        gui_draw_glow(draw_x, draw_y, scaled_width, scaled_height, glow_color, 3);
    }
}

// ============================================================================
// 5D Icon System with Depth
// ============================================================================

sprite_t* gui_create_icon(uint32_t size, color_t base_color, uint32_t icon_type) {
    if (size < 8 || size > 128) return 0;
    
    // Allocate pixel data
    uint32_t* pixels = (uint32_t*)kmalloc(size * size * sizeof(uint32_t));
    if (!pixels) return 0;
    
    // Generate icon based on type
    for (uint32_t y = 0; y < size; y++) {
        for (uint32_t x = 0; x < size; x++) {
            uint32_t idx = y * size + x;
            color_t pixel_color = base_color;
            
            // Calculate distance from center
            int32_t cx = (int32_t)(size / 2);
            int32_t cy = (int32_t)(size / 2);
            int32_t dx = (int32_t)x - cx;
            int32_t dy = (int32_t)y - cy;
            float dist = sqrt_custom((float)(dx * dx + dy * dy));
            float max_dist = (float)(size / 2);
            
            switch (icon_type % 10) {
                case 0:  // Circular icon with gradient
                    if (dist <= max_dist) {
                        float t = dist / max_dist;
                        pixel_color.r = (uint8_t)(base_color.r * (1.0f - t * 0.5f));
                        pixel_color.g = (uint8_t)(base_color.g * (1.0f - t * 0.5f));
                        pixel_color.b = (uint8_t)(base_color.b * (1.0f - t * 0.5f));
                        pixel_color.a = 255;
                    } else {
                        pixel_color.a = 0;  // Transparent outside circle
                    }
                    break;
                    
                case 1:  // Square icon with border
                    if (x < 4 || x >= size - 4 || y < 4 || y >= size - 4) {
                        pixel_color.r = (uint8_t)(base_color.r * 0.7f);
                        pixel_color.g = (uint8_t)(base_color.g * 0.7f);
                        pixel_color.b = (uint8_t)(base_color.b * 0.7f);
                    }
                    pixel_color.a = 255;
                    break;
                    
                case 2:  // Diamond shape
                    if (fabs_custom((float)dx) + fabs_custom((float)dy) <= max_dist) {
                        pixel_color.a = 255;
                    } else {
                        pixel_color.a = 0;
                    }
                    break;
                    
                case 3:  // Folder icon (simplified)
                    if (y < size / 3 && x >= size / 4 && x < 3 * size / 4) {
                        pixel_color.r = (uint8_t)(base_color.r * 1.2f);
                        if (pixel_color.r < base_color.r) pixel_color.r = 255;
                    } else if (y >= size / 3 && x >= size / 8 && x < 7 * size / 8) {
                        pixel_color.a = 255;
                    } else {
                        pixel_color.a = 0;
                    }
                    break;
                    
                case 4:  // Star shape (approximated)
                    {
                        float angle = 0.0f;
                        if (dx != 0 || dy != 0) {
                            // Simplified angle calculation
                            angle = (float)dy / (float)(dx + 1);
                        }
                        float star_radius = max_dist * (0.5f + 0.5f * (1.0f + angle * 0.3f));
                        if (dist <= star_radius) {
                            pixel_color.a = 255;
                        } else {
                            pixel_color.a = 0;
                        }
                    }
                    break;
                    
                default:  // Default: circular with radial gradient
                    if (dist <= max_dist) {
                        float t = dist / max_dist;
                        color_t edge_color = {
                            (uint8_t)(base_color.r * 0.6f),
                            (uint8_t)(base_color.g * 0.6f),
                            (uint8_t)(base_color.b * 0.6f),
                            255
                        };
                        pixel_color = gui_color_lerp(base_color, edge_color, t);
                    } else {
                        pixel_color.a = 0;
                    }
                    break;
            }
            
            // Pack RGBA into uint32_t (R G B A format as expected by sprite drawing)
            pixels[idx] = ((uint32_t)pixel_color.r << 24) | 
                         ((uint32_t)pixel_color.g << 16) | 
                         ((uint32_t)pixel_color.b << 8) | 
                         (uint32_t)pixel_color.a;
        }
    }
    
    return gui_create_sprite(size, size, pixels);
}

void gui_draw_icon_5d(sprite_t* sprite, int32_t x, int32_t y, 
                      icon_depth_level_t depth, uint8_t hover) {
    if (!sprite) return;
    
    // Depth scaling factors
    float scale = 1.0f;
    uint8_t base_alpha = 255;
    int32_t offset_x = 0;
    int32_t offset_y = 0;
    
    switch (depth) {
        case ICON_DEPTH_FOREGROUND:
            scale = 1.0f;
            base_alpha = 255;
            break;
        case ICON_DEPTH_NORMAL:
            scale = 0.9f;
            base_alpha = 230;
            offset_x = 5;
            offset_y = 3;
            break;
        case ICON_DEPTH_BACKGROUND:
            scale = 0.8f;
            base_alpha = 200;
            offset_x = 10;
            offset_y = 6;
            break;
        case ICON_DEPTH_FAR:
            scale = 0.7f;
            base_alpha = 170;
            offset_x = 15;
            offset_y = 9;
            break;
    }
    
    // Hover effects
    if (hover) {
        scale *= 1.1f;  // Slightly enlarge on hover
        
        // Draw glow effect
        color_t glow_color = {200, 220, 255, 150};
        uint32_t glow_size = (uint32_t)(sprite->width * scale * 0.2f);
        gui_draw_glow(x + offset_x - glow_size, y + offset_y - glow_size,
                     (uint32_t)(sprite->width * scale) + 2 * glow_size,
                     (uint32_t)(sprite->height * scale) + 2 * glow_size,
                     glow_color, 5);
        
        // Emit particles on hover
        if (depth == ICON_DEPTH_FOREGROUND) {
            static uint32_t hover_particle_timer = 0;
            hover_particle_timer++;
            if (hover_particle_timer % 10 == 0) {  // Emit every 10 frames
                color_t particle_color = {255, 220, 100, 255};
                gui_emit_particles(x + sprite->width / 2, y + sprite->height / 2, 3, particle_color);
            }
        }
    }
    
    // Draw shadow based on depth
    uint32_t shadow_offset = (uint32_t)(3 * scale);
    uint32_t shadow_blur = (uint32_t)(5 * scale);
    if (depth == ICON_DEPTH_FOREGROUND) {
        gui_draw_shadow(x + offset_x, y + offset_y,
                       (uint32_t)(sprite->width * scale),
                       (uint32_t)(sprite->height * scale),
                       shadow_offset, shadow_blur);
    }
    
    // Draw the icon sprite with scaling and alpha
    if (scale == 1.0f && base_alpha == 255) {
        gui_draw_sprite(sprite, x + offset_x, y + offset_y);
    } else if (scale == 1.0f) {
        gui_draw_sprite_alpha(sprite, x + offset_x, y + offset_y, base_alpha);
    } else {
        // Draw scaled with alpha
        gui_draw_sprite_scaled(sprite, x + offset_x, y + offset_y, scale, scale);
    }
}
