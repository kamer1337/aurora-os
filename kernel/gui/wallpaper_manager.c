/**
 * Aurora OS - Wallpaper Manager Implementation
 * 
 * Manages desktop wallpapers including static and dynamic wallpapers
 */

#include "wallpaper_manager.h"
#include "live_wallpaper.h"
#include "desktop_config.h"
#include "framebuffer.h"
#include "gui.h"
#include "../memory/memory.h"

// Global wallpaper configuration
static wallpaper_config_t config;
static uint8_t initialized = 0;

// Helper to create a color
static color_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    color_t c = {r, g, b, a};
    return c;
}

// Gradient style definitions
static const struct {
    gradient_style_t style;
    const char* name;
    color_t start;
    color_t end;
} gradient_styles[] = {
    {GRADIENT_STYLE_BLUE_SKY, "Blue Sky", {40, 150, 230, 255}, {80, 180, 255, 255}},
    {GRADIENT_STYLE_OCEAN, "Ocean", {20, 80, 140, 255}, {40, 140, 200, 255}},
    {GRADIENT_STYLE_SUNSET, "Sunset", {180, 80, 60, 255}, {240, 140, 80, 255}},
    {GRADIENT_STYLE_FOREST, "Forest", {40, 80, 50, 255}, {60, 130, 70, 255}},
    {GRADIENT_STYLE_PURPLE_DREAM, "Purple Dream", {80, 40, 120, 255}, {140, 90, 200, 255}},
    {GRADIENT_STYLE_NIGHT, "Night", {20, 20, 40, 255}, {40, 40, 80, 255}},
    {GRADIENT_STYLE_WARM, "Warm", {150, 80, 40, 255}, {200, 130, 70, 255}},
    {GRADIENT_STYLE_COOL, "Cool", {40, 100, 140, 255}, {80, 160, 200, 255}},
    {GRADIENT_STYLE_GRAYSCALE, "Grayscale", {100, 100, 100, 255}, {200, 200, 200, 255}},
};

int wallpaper_manager_init(void) {
    if (initialized) {
        return 0;
    }
    
    // Set default configuration
    config.mode = WALLPAPER_MODE_GRADIENT_V;
    config.solid_color = make_color(40, 150, 230, 255);
    config.gradient_style = GRADIENT_STYLE_BLUE_SKY;
    config.gradient_start = make_color(40, 150, 230, 255);
    config.gradient_end = make_color(80, 180, 255, 255);
    config.gradient_center = make_color(60, 165, 240, 255);
    config.live_type = WALLPAPER_NONE;
    config.live_enabled = 0;
    config.image_path = NULL;
    
    initialized = 1;
    return 0;
}

void wallpaper_manager_shutdown(void) {
    initialized = 0;
}

wallpaper_config_t* wallpaper_manager_get_config(void) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    return &config;
}

int wallpaper_manager_apply_config(const wallpaper_config_t* new_config) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    if (!new_config) {
        return -1;
    }
    
    config = *new_config;
    
    // Update desktop config colors based on wallpaper
    desktop_config_t* desktop_cfg = desktop_config_get();
    if (desktop_cfg) {
        if (config.mode == WALLPAPER_MODE_GRADIENT_V || 
            config.mode == WALLPAPER_MODE_GRADIENT) {
            desktop_cfg->desktop_bg_start = config.gradient_start;
            desktop_cfg->desktop_bg_end = config.gradient_end;
        } else if (config.mode == WALLPAPER_MODE_SOLID) {
            desktop_cfg->desktop_bg_start = config.solid_color;
            desktop_cfg->desktop_bg_end = config.solid_color;
        }
    }
    
    // Update live wallpaper if needed
    if (config.mode == WALLPAPER_MODE_LIVE && config.live_enabled) {
        live_wallpaper_set_enabled(1);
        live_wallpaper_set_type(config.live_type);
    } else {
        live_wallpaper_set_enabled(0);
    }
    
    return 0;
}

int wallpaper_manager_set_mode(wallpaper_mode_t mode) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    if (mode < 0 || mode >= WALLPAPER_MODE_COUNT) {
        return -1;
    }
    
    config.mode = mode;
    wallpaper_manager_apply_config(&config);
    
    return 0;
}

int wallpaper_manager_set_solid_color(color_t color) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    config.mode = WALLPAPER_MODE_SOLID;
    config.solid_color = color;
    
    wallpaper_manager_apply_config(&config);
    return 0;
}

int wallpaper_manager_set_gradient(gradient_style_t style, wallpaper_mode_t mode) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    if (style < 0 || style >= GRADIENT_STYLE_CUSTOM) {
        return -1;
    }
    
    // Validate mode is a gradient type
    if (mode != WALLPAPER_MODE_GRADIENT && 
        mode != WALLPAPER_MODE_GRADIENT_H && 
        mode != WALLPAPER_MODE_GRADIENT_V && 
        mode != WALLPAPER_MODE_GRADIENT_RADIAL) {
        mode = WALLPAPER_MODE_GRADIENT_V;  // Default to vertical
    }
    
    config.mode = mode;
    config.gradient_style = style;
    config.gradient_start = gradient_styles[style].start;
    config.gradient_end = gradient_styles[style].end;
    
    wallpaper_manager_apply_config(&config);
    return 0;
}

int wallpaper_manager_set_custom_gradient(color_t start, color_t end, wallpaper_mode_t mode) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    // Validate mode is a gradient type
    if (mode != WALLPAPER_MODE_GRADIENT && 
        mode != WALLPAPER_MODE_GRADIENT_H && 
        mode != WALLPAPER_MODE_GRADIENT_V && 
        mode != WALLPAPER_MODE_GRADIENT_RADIAL) {
        mode = WALLPAPER_MODE_GRADIENT_V;  // Default to vertical
    }
    
    config.mode = mode;
    config.gradient_style = GRADIENT_STYLE_CUSTOM;
    config.gradient_start = start;
    config.gradient_end = end;
    
    wallpaper_manager_apply_config(&config);
    return 0;
}

int wallpaper_manager_set_live(wallpaper_type_t type) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    config.mode = WALLPAPER_MODE_LIVE;
    config.live_type = type;
    config.live_enabled = 1;
    
    wallpaper_manager_apply_config(&config);
    return 0;
}

const char* wallpaper_manager_get_gradient_name(gradient_style_t style) {
    if (style < 0 || style >= GRADIENT_STYLE_CUSTOM) {
        return "Unknown";
    }
    
    return gradient_styles[style].name;
}

void wallpaper_manager_get_gradient_colors(gradient_style_t style, color_t* start, color_t* end) {
    if (style < 0 || style >= GRADIENT_STYLE_CUSTOM || !start || !end) {
        return;
    }
    
    *start = gradient_styles[style].start;
    *end = gradient_styles[style].end;
}

void wallpaper_manager_draw(uint32_t width, uint32_t height) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    switch (config.mode) {
        case WALLPAPER_MODE_NONE:
        case WALLPAPER_MODE_SOLID:
            // Draw solid color
            framebuffer_draw_rect(0, 0, width, height, config.solid_color);
            break;
            
        case WALLPAPER_MODE_GRADIENT:
        case WALLPAPER_MODE_GRADIENT_V:
            // Draw vertical gradient
            for (uint32_t y = 0; y < height; y++) {
                float t = (float)y / (float)height;
                color_t c;
                c.r = (uint8_t)(config.gradient_start.r * (1.0f - t) + config.gradient_end.r * t);
                c.g = (uint8_t)(config.gradient_start.g * (1.0f - t) + config.gradient_end.g * t);
                c.b = (uint8_t)(config.gradient_start.b * (1.0f - t) + config.gradient_end.b * t);
                c.a = 255;
                framebuffer_draw_rect(0, y, width, 1, c);
            }
            break;
            
        case WALLPAPER_MODE_GRADIENT_H:
            // Draw horizontal gradient
            for (uint32_t x = 0; x < width; x++) {
                float t = (float)x / (float)width;
                color_t c;
                c.r = (uint8_t)(config.gradient_start.r * (1.0f - t) + config.gradient_end.r * t);
                c.g = (uint8_t)(config.gradient_start.g * (1.0f - t) + config.gradient_end.g * t);
                c.b = (uint8_t)(config.gradient_start.b * (1.0f - t) + config.gradient_end.b * t);
                c.a = 255;
                framebuffer_draw_rect(x, 0, 1, height, c);
            }
            break;
            
        case WALLPAPER_MODE_GRADIENT_RADIAL:
            // Draw radial gradient from center
            {
                int32_t center_x = width / 2;
                int32_t center_y = height / 2;
                float max_dist = center_x > center_y ? center_x : center_y;
                
                for (uint32_t y = 0; y < height; y++) {
                    for (uint32_t x = 0; x < width; x++) {
                        int32_t dx = (int32_t)x - center_x;
                        int32_t dy = (int32_t)y - center_y;
                        float dist = 0.0f;
                        
                        // Approximate distance
                        if (dx < 0) dx = -dx;
                        if (dy < 0) dy = -dy;
                        dist = (dx > dy) ? dx + dy / 2.0f : dy + dx / 2.0f;
                        
                        float t = dist / max_dist;
                        if (t > 1.0f) t = 1.0f;
                        
                        color_t c;
                        c.r = (uint8_t)(config.gradient_start.r * (1.0f - t) + config.gradient_end.r * t);
                        c.g = (uint8_t)(config.gradient_start.g * (1.0f - t) + config.gradient_end.g * t);
                        c.b = (uint8_t)(config.gradient_start.b * (1.0f - t) + config.gradient_end.b * t);
                        c.a = 255;
                        
                        framebuffer_draw_pixel(x, y, c);
                    }
                }
            }
            break;
            
        case WALLPAPER_MODE_LIVE:
            // Draw live wallpaper
            if (config.live_enabled) {
                live_wallpaper_draw(width, height);
            }
            break;
            
        case WALLPAPER_MODE_IMAGE:
            // Image loading and drawing implementation
            // This would require:
            // 1. Image decoder (BMP, PNG, JPEG support)
            // 2. VFS integration to load image file
            // 3. Scaling/stretching to fit screen
            // 4. Color conversion to framebuffer format
            //
            // Example implementation:
            // if (config.image_path) {
            //     image_t* img = image_load(config.image_path);
            //     if (img) {
            //         image_draw_scaled(img, 0, 0, width, height);
            //         image_free(img);
            //         break;
            //     }
            // }
            // Fall back to solid color if image loading fails
            framebuffer_draw_rect(0, 0, width, height, config.solid_color);
            break;
            
        default:
            // Default to solid color
            framebuffer_draw_rect(0, 0, width, height, config.solid_color);
            break;
    }
}

void wallpaper_manager_update(uint32_t delta_time, int32_t cursor_x, int32_t cursor_y) {
    if (!initialized) {
        return;
    }
    
    // Update live wallpaper if enabled
    if (config.mode == WALLPAPER_MODE_LIVE && config.live_enabled) {
        live_wallpaper_update(delta_time, cursor_x, cursor_y);
    }
}

void wallpaper_manager_show_selector(void) {
    if (!initialized) {
        wallpaper_manager_init();
    }
    
    // Create wallpaper selector window
    window_t* wp_window = gui_create_window("Select Wallpaper", 250, 100, 500, 500);
    if (!wp_window) {
        return;
    }
    
    wp_window->bg_color = make_color(240, 240, 240, 255);
    
    int32_t y_pos = 20;
    int32_t x_pos = 20;
    int32_t label_x = x_pos;
    int32_t button_x = x_pos + 180;
    int32_t button_width = 280;
    int32_t button_height = 30;
    int32_t spacing = 8;
    
    // Solid colors section
    gui_create_label(wp_window, "=== Solid Colors ===", label_x, y_pos);
    y_pos += 30;
    
    gui_create_label(wp_window, "Blue:", label_x, y_pos + 5);
    gui_create_button(wp_window, "Apply Blue", button_x, y_pos, button_width, button_height);
    y_pos += button_height + spacing;
    
    gui_create_label(wp_window, "Gray:", label_x, y_pos + 5);
    gui_create_button(wp_window, "Apply Gray", button_x, y_pos, button_width, button_height);
    y_pos += button_height + spacing + 10;
    
    // Gradients section
    gui_create_label(wp_window, "=== Gradients ===", label_x, y_pos);
    y_pos += 30;
    
    for (int i = 0; i < GRADIENT_STYLE_CUSTOM; i++) {
        gui_create_label(wp_window, gradient_styles[i].name, label_x, y_pos + 5);
        
        widget_t* btn = gui_create_button(wp_window, "Apply", button_x, y_pos, button_width, button_height);
        if (btn) {
            // Preview gradient on button
            btn->bg_color = gradient_styles[i].start;
        }
        
        y_pos += button_height + spacing;
    }
    
    y_pos += 10;
    
    // Live wallpapers section
    gui_create_label(wp_window, "=== Live Wallpapers ===", label_x, y_pos);
    y_pos += 30;
    
    const char* live_names[] = {
        "None",
        "Forest Scene",
        "Ocean Scene",
        "Mountain Landscape",
        "Meadow with Flowers"
    };
    
    for (int i = 0; i < 5; i++) {
        gui_create_label(wp_window, live_names[i], label_x, y_pos + 5);
        gui_create_button(wp_window, "Apply", button_x, y_pos, button_width, button_height);
        y_pos += button_height + spacing;
    }
    
    // Show the window
    gui_show_window(wp_window);
    gui_focus_window(wp_window);
}
