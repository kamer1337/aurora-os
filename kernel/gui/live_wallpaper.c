/**
 * Aurora OS - Live Wallpaper System Implementation
 * 
 * Provides 4D/5D spatial depth live wallpaper with interactive depth perception
 * and nature scene models for an immersive desktop experience.
 */

#include "live_wallpaper.h"
#include "advanced_effects.h"
#include "gui_effects.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include <stddef.h>

// Maximum number of nature elements
#define MAX_NATURE_ELEMENTS 50
#define MAX_DEPTH_LAYERS 5

// Global state
static live_wallpaper_config_t config;
static nature_element_t elements[MAX_NATURE_ELEMENTS];
static uint32_t element_count = 0;
static depth_layer_t layers[MAX_DEPTH_LAYERS];
static uint32_t animation_time = 0;
static uint8_t initialized = 0;

// Helper math functions
static float fabs_custom(float x) {
    return x < 0.0f ? -x : x;
}

static float sin_approx(float x) {
    // Simple sine approximation using polynomial
    // Normalize x to [-PI, PI] range
    while (x > 3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    
    // Taylor series approximation
    float x2 = x * x;
    float x3 = x2 * x;
    float x5 = x3 * x2;
    return x - (x3 / 6.0f) + (x5 / 120.0f);
}

static float cos_approx(float x) {
    return sin_approx(x + 1.5708f); // cos(x) = sin(x + PI/2)
}

// Helper to create a color with alpha
static color_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    color_t c = {r, g, b, a};
    return c;
}

/**
 * Initialize nature elements for a specific scene type
 */
static void init_nature_scene(wallpaper_type_t type) {
    element_count = 0;
    
    switch (type) {
        case WALLPAPER_NATURE_FOREST:
            // Background mountains (depth 0.1-0.3)
            for (int i = 0; i < 5; i++) {
                if (element_count >= MAX_NATURE_ELEMENTS) break;
                elements[element_count].type = ELEMENT_MOUNTAIN;
                elements[element_count].x = i * 400.0f + 100.0f;
                elements[element_count].y = 250.0f;
                elements[element_count].depth = 0.1f + i * 0.04f;
                elements[element_count].scale = 1.5f + i * 0.1f;
                elements[element_count].color = make_color(100, 120, 140, 255);
                elements[element_count].animation_offset = i * 0.5f;
                elements[element_count].visible = 1;
                element_count++;
            }
            
            // Mid-ground trees (depth 0.4-0.6)
            for (int i = 0; i < 15; i++) {
                if (element_count >= MAX_NATURE_ELEMENTS) break;
                elements[element_count].type = ELEMENT_TREE;
                elements[element_count].x = i * 120.0f + 50.0f;
                elements[element_count].y = 300.0f + (i % 3) * 30.0f;
                elements[element_count].depth = 0.4f + (i % 3) * 0.1f;
                elements[element_count].scale = 0.8f + (i % 4) * 0.15f;
                elements[element_count].color = make_color(34, 139, 34, 255);
                elements[element_count].animation_offset = i * 0.3f;
                elements[element_count].visible = 1;
                element_count++;
            }
            
            // Foreground grass (depth 0.8-0.9)
            for (int i = 0; i < 10; i++) {
                if (element_count >= MAX_NATURE_ELEMENTS) break;
                elements[element_count].type = ELEMENT_GRASS;
                elements[element_count].x = i * 180.0f + 20.0f;
                elements[element_count].y = 500.0f;
                elements[element_count].depth = 0.85f;
                elements[element_count].scale = 1.0f;
                elements[element_count].color = make_color(50, 180, 50, 255);
                elements[element_count].animation_offset = i * 0.7f;
                elements[element_count].visible = 1;
                element_count++;
            }
            
            // Clouds (depth 0.05-0.15)
            for (int i = 0; i < 8; i++) {
                if (element_count >= MAX_NATURE_ELEMENTS) break;
                elements[element_count].type = ELEMENT_CLOUD;
                elements[element_count].x = i * 250.0f + 80.0f;
                elements[element_count].y = 50.0f + (i % 3) * 40.0f;
                elements[element_count].depth = 0.05f;
                elements[element_count].scale = 1.0f + (i % 3) * 0.2f;
                elements[element_count].color = make_color(240, 240, 255, 200);
                elements[element_count].animation_offset = i * 1.2f;
                elements[element_count].visible = 1;
                element_count++;
            }
            break;
            
        case WALLPAPER_NATURE_MEADOW:
            // Background mountains
            for (int i = 0; i < 4; i++) {
                if (element_count >= MAX_NATURE_ELEMENTS) break;
                elements[element_count].type = ELEMENT_MOUNTAIN;
                elements[element_count].x = i * 500.0f;
                elements[element_count].y = 280.0f;
                elements[element_count].depth = 0.15f;
                elements[element_count].scale = 1.8f;
                elements[element_count].color = make_color(120, 140, 160, 255);
                elements[element_count].animation_offset = 0.0f;
                elements[element_count].visible = 1;
                element_count++;
            }
            
            // Flowers (depth 0.7-0.9)
            for (int i = 0; i < 20; i++) {
                if (element_count >= MAX_NATURE_ELEMENTS) break;
                elements[element_count].type = ELEMENT_FLOWER;
                elements[element_count].x = (i * 100.0f) + (i % 3) * 30.0f;
                elements[element_count].y = 400.0f + (i % 5) * 40.0f;
                elements[element_count].depth = 0.7f + (i % 3) * 0.07f;
                elements[element_count].scale = 0.5f + (i % 4) * 0.1f;
                // Colorful flowers
                uint8_t r = (i % 3 == 0) ? 255 : (i % 3 == 1) ? 255 : 200;
                uint8_t g = (i % 3 == 0) ? 100 : (i % 3 == 1) ? 200 : 100;
                uint8_t b = (i % 3 == 0) ? 150 : (i % 3 == 1) ? 100 : 255;
                elements[element_count].color = make_color(r, g, b, 255);
                elements[element_count].animation_offset = i * 0.8f;
                elements[element_count].visible = 1;
                element_count++;
            }
            break;
            
        default:
            break;
    }
    
    // Initialize depth layers
    for (int i = 0; i < MAX_DEPTH_LAYERS; i++) {
        layers[i].depth_factor = i * 0.25f;  // 0.0, 0.25, 0.5, 0.75, 1.0
        layers[i].offset_x = 0.0f;
        layers[i].offset_y = 0.0f;
        layers[i].visible = 1;
    }
}

int live_wallpaper_init(void) {
    if (initialized) return 0;
    
    // Initialize configuration with defaults
    config.type = WALLPAPER_NATURE_FOREST;
    config.enabled = 0;  // Disabled by default (optional feature)
    config.parallax_enabled = 1;
    config.particles_enabled = 1;
    config.animation_enabled = 1;
    config.parallax_intensity = 0.5f;
    config.animation_speed = 1.0f;
    config.particle_count = 20;
    
    element_count = 0;
    animation_time = 0;
    initialized = 1;
    
    return 0;
}

void live_wallpaper_shutdown(void) {
    element_count = 0;
    initialized = 0;
}

int live_wallpaper_set_type(wallpaper_type_t type) {
    if (!initialized) return -1;
    
    config.type = type;
    init_nature_scene(type);
    
    return 0;
}

wallpaper_type_t live_wallpaper_get_type(void) {
    return config.type;
}

void live_wallpaper_set_enabled(uint8_t enabled) {
    config.enabled = enabled;
    if (enabled && element_count == 0) {
        init_nature_scene(config.type);
    }
}

uint8_t live_wallpaper_is_enabled(void) {
    return config.enabled;
}

live_wallpaper_config_t* live_wallpaper_get_config(void) {
    return &config;
}

void live_wallpaper_apply_config(const live_wallpaper_config_t* new_config) {
    if (!new_config) return;
    
    uint8_t type_changed = (config.type != new_config->type);
    config = *new_config;
    
    if (type_changed) {
        init_nature_scene(config.type);
    }
}

void live_wallpaper_update(uint32_t delta_time, int32_t cursor_x, int32_t cursor_y) {
    if (!initialized || !config.enabled) return;
    
    // Update animation time
    animation_time += delta_time;
    
    // Update parallax effect based on cursor position
    if (config.parallax_enabled) {
        framebuffer_info_t* fb = framebuffer_get_info();
        if (fb) {
            // Calculate normalized cursor position (-1.0 to 1.0)
            float norm_x = ((float)cursor_x / (float)fb->width - 0.5f) * 2.0f;
            float norm_y = ((float)cursor_y / (float)fb->height - 0.5f) * 2.0f;
            
            // Update each depth layer offset based on cursor position
            for (int i = 0; i < MAX_DEPTH_LAYERS; i++) {
                float intensity = config.parallax_intensity * 30.0f;  // Max 30 pixel offset
                layers[i].offset_x = norm_x * layers[i].depth_factor * intensity;
                layers[i].offset_y = norm_y * layers[i].depth_factor * intensity;
            }
        }
    }
    
    // Update element animations
    if (config.animation_enabled) {
        float time_sec = (float)animation_time / 1000.0f * config.animation_speed;
        
        for (uint32_t i = 0; i < element_count; i++) {
            nature_element_t* elem = &elements[i];
            float anim_time = time_sec + elem->animation_offset;
            
            // Different animation patterns for different elements
            switch (elem->type) {
                case ELEMENT_CLOUD:
                    // Clouds drift slowly
                    elem->x += 0.02f * config.animation_speed;
                    if (elem->x > 2000.0f) elem->x = -200.0f;
                    break;
                    
                case ELEMENT_TREE:
                case ELEMENT_GRASS:
                case ELEMENT_FLOWER:
                    // Gentle swaying motion (stored in animation_offset, not position)
                    // The sway will be applied during rendering
                    break;
                    
                case ELEMENT_LEAF:
                case ELEMENT_BIRD:
                    // Floating/flying motion
                    elem->y += sin_approx(anim_time * 2.0f) * 0.5f;
                    elem->x += 0.3f * config.animation_speed;
                    if (elem->x > 2000.0f) {
                        elem->x = -100.0f;
                        elem->y = 100.0f + ((i * 27) % 200);  // Pseudo-random Y
                    }
                    break;
                    
                default:
                    break;
            }
        }
    }
    
    // Update ambient particles if enabled
    if (config.particles_enabled) {
        gui_update_particles(delta_time);
        
        // Occasionally emit new particles
        if ((animation_time % 1000) < delta_time) {
            // Emit a few floating particles (leaves, etc.)
            framebuffer_info_t* fb = framebuffer_get_info();
            if (fb) {
                int32_t px = ((animation_time * 37) % fb->width);
                int32_t py = 50 + ((animation_time * 17) % 100);
                color_t particle_color = make_color(200, 220, 150, 180);
                gui_emit_particles(px, py, 2, particle_color);
            }
        }
    }
}

/**
 * Draw a simple tree
 */
static void draw_tree(float x, float y, float scale, color_t color, float sway) {
    // Apply parallax offset
    int depth_layer = (int)(elements[0].depth * 4.0f);
    if (depth_layer >= MAX_DEPTH_LAYERS) depth_layer = MAX_DEPTH_LAYERS - 1;
    x += layers[depth_layer].offset_x;
    y += layers[depth_layer].offset_y;
    
    // Tree trunk
    int trunk_width = (int)(10.0f * scale);
    int trunk_height = (int)(60.0f * scale);
    color_t trunk_color = make_color(101, 67, 33, 255);
    
    // Apply sway to trunk position
    int sway_offset = (int)(sway * 3.0f);
    
    framebuffer_draw_filled_rect((int)x - trunk_width/2 + sway_offset, 
                                  (int)y - trunk_height,
                                  trunk_width, trunk_height, trunk_color);
    
    // Tree foliage (simple circle/ellipse approximation)
    int foliage_radius = (int)(40.0f * scale);
    int foliage_y = (int)y - trunk_height - foliage_radius/2;
    
    // Draw layered circles for foliage
    for (int layer = 0; layer < 3; layer++) {
        int layer_radius = foliage_radius - layer * 8;
        int layer_y = foliage_y - layer * 10;
        
        // Make color slightly lighter for each layer
        color_t layer_color = color;
        layer_color.g = (color.g < 235) ? color.g + 20 : 255;
        
        // Draw filled circle approximation
        gui_draw_rounded_rect((int)x - layer_radius + sway_offset,
                             layer_y - layer_radius,
                             layer_radius * 2, layer_radius * 2,
                             layer_radius, layer_color);
    }
}

/**
 * Draw a simple mountain
 */
static void draw_mountain(float x, float y, float scale, color_t color) {
    // Apply parallax offset
    int depth_layer = 0;  // Mountains are in background
    x += layers[depth_layer].offset_x;
    y += layers[depth_layer].offset_y;
    
    int width = (int)(300.0f * scale);
    int height = (int)(200.0f * scale);
    
    // Draw triangle (mountain shape)
    int peak_x = (int)x + width/2;
    int peak_y = (int)y - height;
    int base_y = (int)y;
    
    // Simple filled triangle
    for (int dy = 0; dy < height; dy++) {
        int line_y = base_y - dy;
        int line_width = (width * (height - dy)) / height;
        int line_x = peak_x - line_width/2;
        
        framebuffer_draw_hline(line_x, line_x + line_width, line_y, color);
    }
    
    // Add snow cap
    int snow_height = height / 4;
    color_t snow_color = make_color(255, 255, 255, 255);
    for (int dy = 0; dy < snow_height; dy++) {
        int line_y = peak_y + dy;
        int line_width = (width * dy) / height / 4;
        int line_x = peak_x - line_width/2;
        
        framebuffer_draw_hline(line_x, line_x + line_width, line_y, snow_color);
    }
}

/**
 * Draw a simple cloud
 */
static void draw_cloud(float x, float y, float scale, color_t color) {
    // Apply parallax offset
    int depth_layer = 0;  // Clouds are in far background
    x += layers[depth_layer].offset_x;
    y += layers[depth_layer].offset_y;
    
    int cloud_width = (int)(120.0f * scale);
    int cloud_height = (int)(40.0f * scale);
    
    // Draw overlapping rounded rectangles for cloud puffs
    gui_draw_rounded_rect((int)x, (int)y, cloud_width * 0.6f, cloud_height, 
                         cloud_height/2, color);
    gui_draw_rounded_rect((int)x + cloud_width * 0.3f, (int)y - cloud_height/3, 
                         cloud_width * 0.5f, cloud_height, 
                         cloud_height/2, color);
    gui_draw_rounded_rect((int)x + cloud_width * 0.5f, (int)y, 
                         cloud_width * 0.4f, cloud_height * 0.8f, 
                         cloud_height/2, color);
}

/**
 * Draw grass blades
 */
static void draw_grass(float x, float y, float scale, color_t color, float sway) {
    // Apply parallax offset
    int depth_layer = 4;  // Grass is in foreground
    x += layers[depth_layer].offset_x;
    y += layers[depth_layer].offset_y;
    
    int blade_count = 8;
    int blade_height = (int)(30.0f * scale);
    int blade_width = 2;
    
    for (int i = 0; i < blade_count; i++) {
        int blade_x = (int)x + i * 6;
        int sway_offset = (int)(sway * (i % 3));
        
        // Draw grass blade as vertical line with slight curve
        for (int h = 0; h < blade_height; h++) {
            int curve = (int)((float)h / blade_height * sway_offset);
            framebuffer_draw_pixel(blade_x + curve, (int)y - h, color);
            if (blade_width > 1) {
                framebuffer_draw_pixel(blade_x + curve + 1, (int)y - h, color);
            }
        }
    }
}

/**
 * Draw a simple flower
 */
static void draw_flower(float x, float y, float scale, color_t color, float sway) {
    // Apply parallax offset
    int depth_layer = 3;  // Flowers are in near-foreground
    x += layers[depth_layer].offset_x;
    y += layers[depth_layer].offset_y;
    
    // Stem
    int stem_height = (int)(25.0f * scale);
    int sway_offset = (int)(sway * 2.0f);
    color_t stem_color = make_color(50, 150, 50, 255);
    
    for (int h = 0; h < stem_height; h++) {
        int curve = (int)((float)h / stem_height * sway_offset);
        framebuffer_draw_pixel((int)x + curve, (int)y - h, stem_color);
    }
    
    // Flower head (small circle)
    int flower_size = (int)(6.0f * scale);
    int flower_y = (int)y - stem_height;
    
    gui_draw_rounded_rect((int)x - flower_size/2 + sway_offset, 
                         flower_y - flower_size/2,
                         flower_size, flower_size, 
                         flower_size/2, color);
}

void live_wallpaper_draw_element(const nature_element_t* element) {
    if (!element || !element->visible) return;
    
    // Calculate sway animation
    float time_sec = (float)animation_time / 1000.0f * config.animation_speed;
    float anim_time = time_sec + element->animation_offset;
    float sway = sin_approx(anim_time) * 5.0f * element->depth;  // Deeper elements sway less
    
    switch (element->type) {
        case ELEMENT_TREE:
            draw_tree(element->x, element->y, element->scale, element->color, sway);
            break;
            
        case ELEMENT_MOUNTAIN:
            draw_mountain(element->x, element->y, element->scale, element->color);
            break;
            
        case ELEMENT_CLOUD:
            draw_cloud(element->x, element->y, element->scale, element->color);
            break;
            
        case ELEMENT_GRASS:
            draw_grass(element->x, element->y, element->scale, element->color, sway);
            break;
            
        case ELEMENT_FLOWER:
            draw_flower(element->x, element->y, element->scale, element->color, sway);
            break;
            
        default:
            break;
    }
}

void live_wallpaper_draw(uint32_t width, uint32_t height) {
    if (!initialized || !config.enabled) {
        // Draw default gradient background
        color_t sky_top = make_color(40, 150, 230, 255);
        color_t sky_bottom = make_color(90, 180, 255, 255);
        gui_draw_gradient(0, 0, width, height, sky_top, sky_bottom);
        return;
    }
    
    // Draw sky gradient based on wallpaper type
    color_t sky_top, sky_bottom;
    
    switch (config.type) {
        case WALLPAPER_NATURE_FOREST:
            sky_top = make_color(135, 206, 235, 255);    // Sky blue
            sky_bottom = make_color(176, 224, 230, 255); // Powder blue
            break;
            
        case WALLPAPER_NATURE_MEADOW:
            sky_top = make_color(135, 206, 250, 255);    // Light sky blue
            sky_bottom = make_color(255, 250, 205, 255); // Lemon chiffon
            break;
            
        default:
            sky_top = make_color(40, 150, 230, 255);
            sky_bottom = make_color(90, 180, 255, 255);
            break;
    }
    
    gui_draw_gradient(0, 0, width, height, sky_top, sky_bottom);
    
    // Draw ground/horizon
    color_t ground_color;
    switch (config.type) {
        case WALLPAPER_NATURE_FOREST:
            ground_color = make_color(34, 139, 34, 255);  // Forest green
            break;
        case WALLPAPER_NATURE_MEADOW:
            ground_color = make_color(124, 252, 0, 255);  // Lawn green
            break;
        default:
            ground_color = make_color(50, 180, 50, 255);
            break;
    }
    
    // Draw ground in lower portion
    uint32_t horizon_y = height * 2 / 3;
    framebuffer_draw_filled_rect(0, horizon_y, width, height - horizon_y, ground_color);
    
    // Sort elements by depth (background to foreground)
    // Simple bubble sort is fine for small number of elements
    for (uint32_t i = 0; i < element_count - 1; i++) {
        for (uint32_t j = 0; j < element_count - i - 1; j++) {
            if (elements[j].depth > elements[j + 1].depth) {
                nature_element_t temp = elements[j];
                elements[j] = elements[j + 1];
                elements[j + 1] = temp;
            }
        }
    }
    
    // Draw all nature elements in order (back to front)
    for (uint32_t i = 0; i < element_count; i++) {
        live_wallpaper_draw_element(&elements[i]);
    }
    
    // Draw ambient particles
    if (config.particles_enabled) {
        gui_draw_particles();
    }
}
