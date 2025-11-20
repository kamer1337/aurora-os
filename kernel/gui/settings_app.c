/**
 * Aurora OS - System Settings Application
 * 
 * Central configuration management for Aurora OS
 */

#include "settings_app.h"
#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"

// Global settings
static system_settings_t g_settings;
static window_t* g_settings_window = NULL;
static settings_category_t g_current_category = SETTINGS_DISPLAY;

// Initialize default settings
static void init_default_settings(void) {
    // Display settings
    g_settings.display.resolution_width = 1920;
    g_settings.display.resolution_height = 1080;
    g_settings.display.color_depth = 32;
    g_settings.display.refresh_rate = 60;
    
    // Theme settings
    const char* default_theme = "Aurora Dark";
    for (int i = 0; i < 32 && default_theme[i]; i++) {
        g_settings.theme.theme_name[i] = default_theme[i];
    }
    g_settings.theme.primary_color = (color_t){30, 30, 40, 255};
    g_settings.theme.secondary_color = (color_t){50, 50, 60, 255};
    g_settings.theme.accent_color = (color_t){100, 150, 255, 255};
    g_settings.theme.enable_effects = 1;
    g_settings.theme.enable_animations = 1;
    g_settings.theme.enable_transparency = 1;
    
    // User settings
    const char* default_user = "user";
    for (int i = 0; i < 32 && default_user[i]; i++) {
        g_settings.user.username[i] = default_user[i];
    }
    const char* default_fullname = "Aurora User";
    for (int i = 0; i < 64 && default_fullname[i]; i++) {
        g_settings.user.full_name[i] = default_fullname[i];
    }
    g_settings.user.auto_login = 0;
    g_settings.user.show_desktop_icons = 1;
    g_settings.user.show_taskbar = 1;
    
    g_settings.settings_changed = 0;
}

// Render settings content
static void render_settings_content(window_t* window) {
    if (!window) return;
    
    // Content area
    int content_x = window->bounds.x + 200;
    int content_y = window->bounds.y + 60;
    int content_w = window->bounds.width - 220;
    int content_h = window->bounds.height - 80;
    
    // Draw content background
    color_t content_bg = {40, 40, 50, 255};
    framebuffer_draw_rect(content_x, content_y, content_w, content_h, content_bg);
    
    // Draw category title
    color_t title_color = {255, 255, 255, 255};
    const char* category_titles[] = {
        "Display Settings",
        "Theme Settings",
        "User Settings",
        "Network Settings",
        "Security Settings",
        "About Aurora OS"
    };
    
    if (g_current_category < 6) {
        framebuffer_draw_string(content_x + 20, content_y + 20, 
                    category_titles[g_current_category], title_color, (color_t){0, 0, 0, 0});
    }
    
    // Draw settings based on category
    int y = content_y + 60;
    color_t label_color = {200, 200, 200, 255};
    color_t value_color = {150, 200, 255, 255};
    
    switch (g_current_category) {
        case SETTINGS_DISPLAY:
            framebuffer_draw_string(content_x + 20, y, "Resolution:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, "1920 x 1080", value_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Color Depth:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, "32-bit", value_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Refresh Rate:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, "60 Hz", value_color, (color_t){0, 0, 0, 0});
            break;
            
        case SETTINGS_THEME:
            framebuffer_draw_string(content_x + 20, y, "Theme:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, g_settings.theme.theme_name, value_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Visual Effects:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, 
                        g_settings.theme.enable_effects ? "Enabled" : "Disabled", 
                        value_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Animations:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, 
                        g_settings.theme.enable_animations ? "Enabled" : "Disabled", 
                        value_color, (color_t){0, 0, 0, 0});
            break;
            
        case SETTINGS_USER:
            framebuffer_draw_string(content_x + 20, y, "Username:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, g_settings.user.username, value_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Full Name:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, g_settings.user.full_name, value_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Auto Login:", label_color, (color_t){0, 0, 0, 0});
            framebuffer_draw_string(content_x + 200, y, 
                        g_settings.user.auto_login ? "Enabled" : "Disabled", 
                        value_color, (color_t){0, 0, 0, 0});
            break;
            
        case SETTINGS_ABOUT:
            framebuffer_draw_string(content_x + 20, y, "Aurora OS", title_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Version: 1.0-RC1", label_color, (color_t){0, 0, 0, 0});
            y += 20;
            framebuffer_draw_string(content_x + 20, y, "Release Candidate", label_color, (color_t){0, 0, 0, 0});
            y += 30;
            framebuffer_draw_string(content_x + 20, y, "Features:", label_color, (color_t){0, 0, 0, 0});
            y += 20;
            framebuffer_draw_string(content_x + 20, y, "- Microkernel Architecture", value_color, (color_t){0, 0, 0, 0});
            y += 20;
            framebuffer_draw_string(content_x + 20, y, "- Advanced GUI with 3D/4D/5D Effects", value_color, (color_t){0, 0, 0, 0});
            y += 20;
            framebuffer_draw_string(content_x + 20, y, "- Quantum Cryptography", value_color, (color_t){0, 0, 0, 0});
            y += 20;
            framebuffer_draw_string(content_x + 20, y, "- ~125 FPS Rendering", value_color, (color_t){0, 0, 0, 0});
            break;
            
        default:
            framebuffer_draw_string(content_x + 20, y, "Settings not yet implemented", label_color, (color_t){0, 0, 0, 0});
            break;
    }
}

// Render category sidebar
static void render_sidebar(window_t* window) {
    if (!window) return;
    
    int sidebar_x = window->bounds.x + 10;
    int sidebar_y = window->bounds.y + 60;
    int sidebar_w = 180;
    
    // Category buttons
    const char* categories[] = {
        "Display",
        "Theme",
        "User",
        "Network",
        "Security",
        "About"
    };
    
    for (int i = 0; i < 6; i++) {
        int btn_y = sidebar_y + (i * 40);
        
        // Highlight selected category
        color_t btn_color;
        if (i == g_current_category) {
            btn_color = (color_t){80, 120, 200, 255};
        } else {
            btn_color = (color_t){50, 50, 60, 255};
        }
        
        framebuffer_draw_rect(sidebar_x, btn_y, sidebar_w, 35, btn_color);
        
        color_t text_color = {255, 255, 255, 255};
        framebuffer_draw_string(sidebar_x + 10, btn_y + 10, categories[i], text_color, (color_t){0, 0, 0, 0});
    }
}

// Custom paint function
static void settings_paint(widget_t* widget) {
    if (!widget || !widget->parent_window) return;
    render_sidebar(widget->parent_window);
    render_settings_content(widget->parent_window);
}

// Custom click handler
static void settings_click(widget_t* widget, int32_t x, int32_t y) {
    if (!widget || !widget->parent_window) return;
    
    window_t* window = widget->parent_window;
    int sidebar_x = window->bounds.x + 10;
    int sidebar_y = window->bounds.y + 60;
    int sidebar_w = 180;
    
    // Check if click is in sidebar
    if (x >= sidebar_x && x < sidebar_x + sidebar_w) {
        // Determine which category was clicked
        int relative_y = y - sidebar_y;
        if (relative_y >= 0 && relative_y < 240) { // 6 categories * 40px
            int category = relative_y / 40;
            if (category >= 0 && category < 6) {
                g_current_category = (settings_category_t)category;
                gui_update();
            }
        }
    }
}

window_t* launch_settings_app(void) {
    // Initialize settings if first launch
    static int initialized = 0;
    if (!initialized) {
        init_default_settings();
        initialized = 1;
    }
    
    // Create window
    g_settings_window = gui_create_window("System Settings", 250, 150, 800, 600);
    if (!g_settings_window) {
        return NULL;
    }
    
    g_settings_window->bg_color = (color_t){30, 30, 40, 255};
    
    // Create main content widget
    widget_t* content = gui_create_panel(g_settings_window, 10, 50, 780, 540);
    if (content) {
        content->on_paint = settings_paint;
        content->on_click = settings_click;
    }
    
    gui_show_window(g_settings_window);
    return g_settings_window;
}

system_settings_t* get_system_settings(void) {
    static int initialized = 0;
    if (!initialized) {
        init_default_settings();
        initialized = 1;
    }
    return &g_settings;
}

int apply_settings(void) {
    // Apply settings changes
    // This would integrate with system modules to apply changes
    g_settings.settings_changed = 0;
    return 0;
}

void reset_settings_to_defaults(void) {
    init_default_settings();
}

int save_settings(void) {
    // Save settings to persistent storage
    // This would write to file system
    return 0;
}
