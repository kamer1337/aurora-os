/**
 * Aurora OS - Desktop Configuration System Implementation
 * 
 * Manages desktop appearance and behavior settings
 */

#include "desktop_config.h"
#include "gui.h"
#include "font_manager.h"
#include "framebuffer.h"
#include "../memory/memory.h"

// Global configuration
static desktop_config_t config;
static uint8_t config_initialized = 0;

int desktop_config_init(void) {
    if (config_initialized) {
        return 0;
    }
    
    // Set default values
    config.default_font = FONT_8X8_CRYSTALLINE;
    
    // Default color scheme - vivid and modern
    config.desktop_bg_start = (color_t){40, 150, 230, 255};   // Sky blue
    config.desktop_bg_end = (color_t){80, 180, 255, 255};     // Lighter blue
    config.taskbar_bg = (color_t){45, 45, 48, 255};           // Dark gray
    config.taskbar_fg = COLOR_WHITE;
    config.window_title_active = (color_t){45, 170, 255, 255};  // Vivid blue
    config.window_title_inactive = (color_t){165, 165, 165, 255};  // Gray
    config.window_border = COLOR_DARK_GRAY;
    
    // Desktop behavior defaults
    config.show_desktop_icons = 1;
    config.show_taskbar = 1;
    config.show_system_tray = 1;
    config.enable_animations = 1;
    config.enable_shadows = 1;
    config.enable_transparency = 0;  // Disabled for performance
    
    // Icon settings
    config.icon_size = 48;
    config.icon_spacing = 20;
    
    // Window settings
    config.window_snap_enabled = 1;
    config.window_animations = 1;
    config.animation_speed = 200;  // milliseconds
    
    // Live wallpaper settings
    config.enable_live_wallpaper = 0;  // Disabled by default (optional feature)
    
    config_initialized = 1;
    
    // Apply initial configuration
    desktop_config_apply(&config);
    
    return 0;
}

desktop_config_t* desktop_config_get(void) {
    if (!config_initialized) {
        desktop_config_init();
    }
    
    return &config;
}

int desktop_config_apply(const desktop_config_t* new_config) {
    if (!new_config) {
        return -1;
    }
    
    if (!config_initialized) {
        desktop_config_init();
    }
    
    // Copy configuration
    config = *new_config;
    
    // Apply font settings
    font_manager_set_current(config.default_font);
    
    // Note: Other settings would trigger appropriate updates
    // For now, they'll take effect on next render cycle
    
    return 0;
}

void desktop_config_reset(void) {
    config_initialized = 0;
    desktop_config_init();
}

int desktop_config_save(void) {
    // TODO: Implement configuration persistence to file
    // For now, this is a stub that returns success
    return 0;
}

int desktop_config_load(void) {
    // TODO: Implement configuration loading from file
    // For now, this is a stub that returns success
    return 0;
}

// Helper function to create dropdown widget (simplified)
static widget_t* create_dropdown(window_t* window, const char* label, int32_t x, int32_t y, int32_t width) {
    // Create a panel to hold the dropdown
    widget_t* panel = gui_create_panel(window, x, y, width, 30);
    if (!panel) return NULL;
    
    // Add label
    gui_create_label(window, label, x + 5, y + 8);
    
    return panel;
}

void desktop_config_show_settings(void) {
    if (!config_initialized) {
        desktop_config_init();
    }
    
    // Create settings window
    window_t* settings_window = gui_create_window("Desktop Settings", 200, 100, 500, 450);
    if (!settings_window) {
        return;
    }
    
    settings_window->bg_color = (color_t){240, 240, 240, 255};
    
    int32_t y_pos = 20;
    int32_t x_label = 20;
    int32_t x_control = 180;
    int32_t spacing = 45;
    
    // Font Selection Section
    gui_create_label(settings_window, "=== Appearance ===", x_label, y_pos);
    y_pos += 30;
    
    gui_create_label(settings_window, "Font:", x_label, y_pos);
    create_dropdown(settings_window, "", x_control, y_pos - 5, 200);
    y_pos += spacing;
    
    // Show available fonts (for reference)
    // const char* fonts[] = {
    //     "Standard 8x8",
    //     "Crystalline 8x8 (Default)",
    //     "Crystalline 5x7",
    //     "Monospace 6x8"
    // };
    
    // Display current font selection
    const font_info_t* current_font_info = font_manager_get_info(config.default_font);
    if (current_font_info) {
        gui_create_label(settings_window, current_font_info->name, x_control + 10, y_pos - spacing + 3);
    }
    
    // Desktop Icons Section
    gui_create_label(settings_window, "=== Desktop ===", x_label, y_pos);
    y_pos += 30;
    
    gui_create_label(settings_window, "Show Desktop Icons:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.show_desktop_icons ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    gui_create_label(settings_window, "Show Taskbar:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.show_taskbar ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    gui_create_label(settings_window, "Show System Tray:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.show_system_tray ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    // Effects Section
    gui_create_label(settings_window, "=== Effects ===", x_label, y_pos);
    y_pos += 30;
    
    gui_create_label(settings_window, "Enable Animations:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.enable_animations ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    gui_create_label(settings_window, "Enable Shadows:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.enable_shadows ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    gui_create_label(settings_window, "Window Snapping:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.window_snap_enabled ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    gui_create_label(settings_window, "Live Wallpaper:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.enable_live_wallpaper ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    // Action buttons
    y_pos += 20;
    gui_create_button(settings_window, "Apply", 150, y_pos, 80, 30);
    gui_create_button(settings_window, "Reset", 240, y_pos, 80, 30);
    gui_create_button(settings_window, "Close", 330, y_pos, 80, 30);
    
    // Show the window
    gui_show_window(settings_window);
    gui_focus_window(settings_window);
}
