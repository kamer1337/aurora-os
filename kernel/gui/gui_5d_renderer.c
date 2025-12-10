/**
 * Aurora OS - Enhanced 5D Renderer Implementation
 * 
 * Advanced 5D rendering with plugin support and new effects
 */

#include "gui_5d_renderer.h"
#include "theme_plugin.h"
#include "gui_effects.h"
#include "framebuffer.h"

// Global renderer context
static renderer_5d_context_t renderer_context;
static uint8_t renderer_initialized = 0;

// Math helpers
static inline float clamp_float(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

static inline int32_t clamp_int(int32_t value, int32_t min, int32_t max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int gui_5d_renderer_init(void) {
    if (renderer_initialized) {
        return 0;
    }
    
    // Initialize context with defaults
    renderer_context.mode = RENDER_MODE_STANDARD;
    renderer_context.active_plugin = NULL;
    renderer_context.enable_depth_fog = 1;
    renderer_context.enable_volumetric_lighting = 1;
    renderer_context.enable_motion_blur = 0;
    renderer_context.depth_of_field = 0.0f;
    
    renderer_initialized = 1;
    return 0;
}

void gui_5d_renderer_shutdown(void) {
    renderer_initialized = 0;
}

renderer_5d_context_t* gui_5d_renderer_get_context(void) {
    if (!renderer_initialized) {
        gui_5d_renderer_init();
    }
    return &renderer_context;
}

int gui_5d_renderer_set_mode(render_mode_t mode) {
    if (!renderer_initialized) {
        return -1;
    }
    
    renderer_context.mode = mode;
    return 0;
}

void gui_5d_renderer_enable_depth_fog(uint8_t enable) {
    if (renderer_initialized) {
        renderer_context.enable_depth_fog = enable;
    }
}

void gui_5d_renderer_enable_volumetric_lighting(uint8_t enable) {
    if (renderer_initialized) {
        renderer_context.enable_volumetric_lighting = enable;
    }
}

void gui_5d_renderer_enable_motion_blur(uint8_t enable) {
    if (renderer_initialized) {
        renderer_context.enable_motion_blur = enable;
    }
}

void gui_5d_renderer_set_depth_of_field(float strength) {
    if (renderer_initialized) {
        renderer_context.depth_of_field = clamp_float(strength, 0.0f, 1.0f);
    }
}

int gui_5d_renderer_render_layer(render_layer_5d_t* layer) {
    if (!renderer_initialized || !layer) {
        return -1;
    }
    
    // Calculate depth-based effects
    float depth = clamp_float(layer->depth, 0.0f, 1.0f);
    // Note: opacity is part of layer struct but not used in current rendering
    // It's available for future enhancements
    
    // Apply depth fog if enabled
    if (renderer_context.enable_depth_fog) {
        gui_5d_apply_depth_fog(layer->x, layer->y, layer->width, layer->height, depth);
    }
    
    // Apply depth-based shadow
    uint32_t shadow_offset = (uint32_t)(5.0f * (1.0f - depth));
    uint32_t shadow_blur = (uint32_t)(8.0f * (1.0f - depth));
    gui_draw_shadow(layer->x, layer->y, layer->width, layer->height, 
                   shadow_offset, shadow_blur);
    
    // Render plugin effects if in enhanced or plugin-only mode
    if (renderer_context.mode == RENDER_MODE_ENHANCED || 
        renderer_context.mode == RENDER_MODE_PLUGIN_ONLY) {
        gui_5d_render_plugin_effects(layer->x, layer->y, layer->width, layer->height, NULL);
    }
    
    return 0;
}

void gui_5d_apply_depth_fog(int32_t x, int32_t y, uint32_t width, uint32_t height, float depth) {
    // Apply fog based on depth (deeper = more fog)
    uint8_t fog_alpha = (uint8_t)(depth * 120.0f);
    
    if (fog_alpha > 0) {
        color_t fog_color = {180, 180, 200, fog_alpha};
        gui_draw_rect_alpha(x, y, width, height, fog_color);
    }
}

void gui_5d_apply_volumetric_light(int32_t x, int32_t y, uint32_t radius, 
                                    float intensity, color_t color) {
    if (!renderer_initialized || !renderer_context.enable_volumetric_lighting) {
        return;
    }
    
    // Create volumetric light effect with radial gradient
    intensity = clamp_float(intensity, 0.0f, 1.0f);
    
    // Inner bright core
    color_t core_color = color;
    core_color.a = (uint8_t)(255.0f * intensity);
    gui_draw_gradient_radial(x, y, radius / 4, core_color, 
                            (color_t){color.r, color.g, color.b, 0});
    
    // Outer glow
    color_t glow_color = color;
    glow_color.a = (uint8_t)(150.0f * intensity);
    gui_draw_gradient_radial(x, y, radius, glow_color,
                            (color_t){color.r, color.g, color.b, 0});
}

void gui_5d_draw_hologram(int32_t x, int32_t y, uint32_t width, uint32_t height,
                          color_t color, float time) {
    // Animated holographic effect with scan lines and glitch
    
    // Base translucent layer
    color_t base = color;
    base.a = 150;
    gui_draw_rect_alpha(x, y, width, height, base);
    
    // Scan line effect (animated)
    uint32_t scan_offset = ((uint32_t)(time * 100.0f)) % height;
    color_t scan_line = color;
    scan_line.a = 200;
    for (uint32_t i = 0; i < 3; i++) {
        uint32_t line_y = (scan_offset + i * 20) % height;
        gui_draw_rect_alpha(x, y + line_y, width, 2, scan_line);
    }
    
    // Edge glow
    color_t edge_glow = color;
    edge_glow.a = 180;
    gui_draw_glow(x, y, width, height, edge_glow, 3);
    
    // Random flicker effect
    if (((uint32_t)(time * 10.0f)) % 20 < 2) {
        color_t flicker = {255, 255, 255, 100};
        gui_draw_rect_alpha(x, y, width, height, flicker);
    }
}

void gui_5d_draw_chromatic_aberration(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                       float strength) {
    // Chromatic aberration - RGB channel separation
    strength = clamp_float(strength, 0.0f, 1.0f);
    int32_t offset = (int32_t)(strength * 3.0f);
    
    if (offset > 0) {
        // Red channel offset
        color_t red_tint = {255, 0, 0, 40};
        gui_draw_rect_alpha(x - offset, y, width, height, red_tint);
        
        // Blue channel offset
        color_t blue_tint = {0, 0, 255, 40};
        gui_draw_rect_alpha(x + offset, y, width, height, blue_tint);
    }
}

void gui_5d_draw_parallax(int32_t x, int32_t y, uint32_t width, uint32_t height,
                          float depth, int32_t offset_x, int32_t offset_y) {
    // Apply parallax offset based on depth
    depth = clamp_float(depth, 0.0f, 1.0f);
    
    // Deeper layers move less
    int32_t parallax_x = (int32_t)(offset_x * depth);
    int32_t parallax_y = (int32_t)(offset_y * depth);
    
    // Draw with offset and depth-based opacity
    uint8_t alpha = (uint8_t)(255.0f * (1.0f - depth * 0.3f));
    color_t tint = {255, 255, 255, alpha};
    
    // Simple representation - draw rect with parallax offset
    gui_draw_rect_alpha(x + parallax_x, y + parallax_y, width, height, tint);
}

void gui_5d_apply_lens_distortion(int32_t x, int32_t y, uint32_t radius, float strength) {
    // Lens distortion effect (simplified - just add radial gradient overlay)
    strength = clamp_float(strength, -1.0f, 1.0f);
    
    // Barrel distortion (positive) or pincushion (negative)
    if (strength > 0.0f) {
        // Barrel - darker edges
        color_t dark_edge = {0, 0, 0, (uint8_t)(strength * 80.0f)};
        gui_draw_gradient_radial(x, y, radius,
                                (color_t){0, 0, 0, 0}, dark_edge);
    } else if (strength < 0.0f) {
        // Pincushion - brighter center
        color_t bright_center = {255, 255, 255, (uint8_t)(-strength * 60.0f)};
        gui_draw_gradient_radial(x, y, radius,
                                bright_center, (color_t){255, 255, 255, 0});
    }
}

int gui_5d_render_plugin_effects(int32_t x, int32_t y, uint32_t width, uint32_t height,
                                  void* params) {
    if (!renderer_initialized) {
        return -1;
    }
    
    // Get active plugin from theme plugin system
    theme_plugin_t* plugin = theme_plugin_get_active();
    if (!plugin) {
        return 0;  // No active plugin, not an error
    }
    
    // Render all effects from the plugin
    plugin_effect_t* effect = plugin->effects;
    while (effect) {
        if (effect->render) {
            effect->render(x, y, width, height, params, effect->user_data);
        }
        effect = effect->next;
    }
    
    return 0;
}
