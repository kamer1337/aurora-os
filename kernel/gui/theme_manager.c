/**
 * Aurora OS - Theme Manager Implementation
 * 
 * Manages desktop color themes and appearance presets
 */

#include "theme_manager.h"
#include "desktop_config.h"
#include "gui.h"
#include "../memory/memory.h"

// Global theme array
static theme_t themes[THEME_COUNT];
static theme_type_t current_theme_type = THEME_DEFAULT;
static uint8_t initialized = 0;

// Helper to create a color
static color_t make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    color_t c = {r, g, b, a};
    return c;
}

int theme_manager_init(void) {
    if (initialized) {
        return 0;
    }
    
    // Initialize Default Theme (Aurora Blue)
    themes[THEME_DEFAULT].type = THEME_DEFAULT;
    themes[THEME_DEFAULT].name = "Aurora (Default)";
    themes[THEME_DEFAULT].desktop_bg_start = make_color(40, 150, 230, 255);
    themes[THEME_DEFAULT].desktop_bg_end = make_color(80, 180, 255, 255);
    themes[THEME_DEFAULT].taskbar_bg = make_color(45, 45, 48, 255);
    themes[THEME_DEFAULT].taskbar_fg = make_color(255, 255, 255, 255);
    themes[THEME_DEFAULT].taskbar_highlight = make_color(60, 170, 255, 255);
    themes[THEME_DEFAULT].window_title_active = make_color(45, 170, 255, 255);
    themes[THEME_DEFAULT].window_title_inactive = make_color(165, 165, 165, 255);
    themes[THEME_DEFAULT].window_border = make_color(100, 100, 100, 255);
    themes[THEME_DEFAULT].window_bg = make_color(240, 240, 240, 255);
    themes[THEME_DEFAULT].button_bg = make_color(220, 220, 220, 255);
    themes[THEME_DEFAULT].button_fg = make_color(0, 0, 0, 255);
    themes[THEME_DEFAULT].button_hover = make_color(200, 220, 255, 255);
    themes[THEME_DEFAULT].button_pressed = make_color(150, 190, 240, 255);
    themes[THEME_DEFAULT].text_primary = make_color(0, 0, 0, 255);
    themes[THEME_DEFAULT].text_secondary = make_color(80, 80, 80, 255);
    themes[THEME_DEFAULT].text_disabled = make_color(160, 160, 160, 255);
    
    // Initialize Dark Theme
    themes[THEME_DARK].type = THEME_DARK;
    themes[THEME_DARK].name = "Dark";
    themes[THEME_DARK].desktop_bg_start = make_color(30, 30, 35, 255);
    themes[THEME_DARK].desktop_bg_end = make_color(50, 50, 60, 255);
    themes[THEME_DARK].taskbar_bg = make_color(20, 20, 25, 255);
    themes[THEME_DARK].taskbar_fg = make_color(240, 240, 240, 255);
    themes[THEME_DARK].taskbar_highlight = make_color(80, 80, 100, 255);
    themes[THEME_DARK].window_title_active = make_color(60, 60, 80, 255);
    themes[THEME_DARK].window_title_inactive = make_color(50, 50, 55, 255);
    themes[THEME_DARK].window_border = make_color(40, 40, 45, 255);
    themes[THEME_DARK].window_bg = make_color(45, 45, 50, 255);
    themes[THEME_DARK].button_bg = make_color(60, 60, 70, 255);
    themes[THEME_DARK].button_fg = make_color(240, 240, 240, 255);
    themes[THEME_DARK].button_hover = make_color(80, 80, 100, 255);
    themes[THEME_DARK].button_pressed = make_color(50, 50, 70, 255);
    themes[THEME_DARK].text_primary = make_color(240, 240, 240, 255);
    themes[THEME_DARK].text_secondary = make_color(180, 180, 180, 255);
    themes[THEME_DARK].text_disabled = make_color(100, 100, 100, 255);
    
    // Initialize Light Theme
    themes[THEME_LIGHT].type = THEME_LIGHT;
    themes[THEME_LIGHT].name = "Light";
    themes[THEME_LIGHT].desktop_bg_start = make_color(230, 240, 250, 255);
    themes[THEME_LIGHT].desktop_bg_end = make_color(250, 250, 255, 255);
    themes[THEME_LIGHT].taskbar_bg = make_color(240, 240, 245, 255);
    themes[THEME_LIGHT].taskbar_fg = make_color(20, 20, 20, 255);
    themes[THEME_LIGHT].taskbar_highlight = make_color(200, 220, 255, 255);
    themes[THEME_LIGHT].window_title_active = make_color(100, 150, 230, 255);
    themes[THEME_LIGHT].window_title_inactive = make_color(200, 200, 200, 255);
    themes[THEME_LIGHT].window_border = make_color(180, 180, 180, 255);
    themes[THEME_LIGHT].window_bg = make_color(255, 255, 255, 255);
    themes[THEME_LIGHT].button_bg = make_color(240, 240, 245, 255);
    themes[THEME_LIGHT].button_fg = make_color(0, 0, 0, 255);
    themes[THEME_LIGHT].button_hover = make_color(220, 230, 250, 255);
    themes[THEME_LIGHT].button_pressed = make_color(180, 200, 240, 255);
    themes[THEME_LIGHT].text_primary = make_color(20, 20, 20, 255);
    themes[THEME_LIGHT].text_secondary = make_color(100, 100, 100, 255);
    themes[THEME_LIGHT].text_disabled = make_color(180, 180, 180, 255);
    
    // Initialize Ocean Theme
    themes[THEME_OCEAN].type = THEME_OCEAN;
    themes[THEME_OCEAN].name = "Ocean";
    themes[THEME_OCEAN].desktop_bg_start = make_color(30, 80, 120, 255);
    themes[THEME_OCEAN].desktop_bg_end = make_color(60, 140, 180, 255);
    themes[THEME_OCEAN].taskbar_bg = make_color(20, 60, 90, 255);
    themes[THEME_OCEAN].taskbar_fg = make_color(240, 250, 255, 255);
    themes[THEME_OCEAN].taskbar_highlight = make_color(50, 120, 160, 255);
    themes[THEME_OCEAN].window_title_active = make_color(40, 110, 160, 255);
    themes[THEME_OCEAN].window_title_inactive = make_color(100, 140, 160, 255);
    themes[THEME_OCEAN].window_border = make_color(30, 80, 120, 255);
    themes[THEME_OCEAN].window_bg = make_color(230, 245, 250, 255);
    themes[THEME_OCEAN].button_bg = make_color(60, 140, 180, 255);
    themes[THEME_OCEAN].button_fg = make_color(255, 255, 255, 255);
    themes[THEME_OCEAN].button_hover = make_color(80, 160, 200, 255);
    themes[THEME_OCEAN].button_pressed = make_color(40, 120, 160, 255);
    themes[THEME_OCEAN].text_primary = make_color(20, 40, 60, 255);
    themes[THEME_OCEAN].text_secondary = make_color(60, 90, 120, 255);
    themes[THEME_OCEAN].text_disabled = make_color(140, 160, 180, 255);
    
    // Initialize Forest Theme
    themes[THEME_FOREST].type = THEME_FOREST;
    themes[THEME_FOREST].name = "Forest";
    themes[THEME_FOREST].desktop_bg_start = make_color(40, 80, 50, 255);
    themes[THEME_FOREST].desktop_bg_end = make_color(60, 120, 70, 255);
    themes[THEME_FOREST].taskbar_bg = make_color(30, 60, 35, 255);
    themes[THEME_FOREST].taskbar_fg = make_color(240, 250, 240, 255);
    themes[THEME_FOREST].taskbar_highlight = make_color(70, 140, 80, 255);
    themes[THEME_FOREST].window_title_active = make_color(50, 130, 70, 255);
    themes[THEME_FOREST].window_title_inactive = make_color(100, 140, 110, 255);
    themes[THEME_FOREST].window_border = make_color(40, 90, 50, 255);
    themes[THEME_FOREST].window_bg = make_color(240, 250, 245, 255);
    themes[THEME_FOREST].button_bg = make_color(70, 150, 80, 255);
    themes[THEME_FOREST].button_fg = make_color(255, 255, 255, 255);
    themes[THEME_FOREST].button_hover = make_color(90, 170, 100, 255);
    themes[THEME_FOREST].button_pressed = make_color(50, 130, 60, 255);
    themes[THEME_FOREST].text_primary = make_color(20, 40, 25, 255);
    themes[THEME_FOREST].text_secondary = make_color(60, 100, 70, 255);
    themes[THEME_FOREST].text_disabled = make_color(140, 170, 150, 255);
    
    // Initialize Sunset Theme
    themes[THEME_SUNSET].type = THEME_SUNSET;
    themes[THEME_SUNSET].name = "Sunset";
    themes[THEME_SUNSET].desktop_bg_start = make_color(180, 80, 60, 255);
    themes[THEME_SUNSET].desktop_bg_end = make_color(240, 140, 80, 255);
    themes[THEME_SUNSET].taskbar_bg = make_color(150, 60, 50, 255);
    themes[THEME_SUNSET].taskbar_fg = make_color(255, 245, 230, 255);
    themes[THEME_SUNSET].taskbar_highlight = make_color(220, 120, 80, 255);
    themes[THEME_SUNSET].window_title_active = make_color(220, 100, 70, 255);
    themes[THEME_SUNSET].window_title_inactive = make_color(180, 140, 120, 255);
    themes[THEME_SUNSET].window_border = make_color(150, 80, 60, 255);
    themes[THEME_SUNSET].window_bg = make_color(255, 250, 245, 255);
    themes[THEME_SUNSET].button_bg = make_color(230, 130, 90, 255);
    themes[THEME_SUNSET].button_fg = make_color(255, 255, 255, 255);
    themes[THEME_SUNSET].button_hover = make_color(250, 150, 110, 255);
    themes[THEME_SUNSET].button_pressed = make_color(200, 110, 70, 255);
    themes[THEME_SUNSET].text_primary = make_color(60, 30, 20, 255);
    themes[THEME_SUNSET].text_secondary = make_color(120, 80, 60, 255);
    themes[THEME_SUNSET].text_disabled = make_color(180, 150, 140, 255);
    
    // Initialize Purple Theme
    themes[THEME_PURPLE].type = THEME_PURPLE;
    themes[THEME_PURPLE].name = "Purple";
    themes[THEME_PURPLE].desktop_bg_start = make_color(80, 40, 120, 255);
    themes[THEME_PURPLE].desktop_bg_end = make_color(130, 80, 180, 255);
    themes[THEME_PURPLE].taskbar_bg = make_color(60, 30, 90, 255);
    themes[THEME_PURPLE].taskbar_fg = make_color(250, 240, 255, 255);
    themes[THEME_PURPLE].taskbar_highlight = make_color(120, 80, 160, 255);
    themes[THEME_PURPLE].window_title_active = make_color(110, 70, 170, 255);
    themes[THEME_PURPLE].window_title_inactive = make_color(140, 120, 160, 255);
    themes[THEME_PURPLE].window_border = make_color(80, 50, 120, 255);
    themes[THEME_PURPLE].window_bg = make_color(245, 240, 250, 255);
    themes[THEME_PURPLE].button_bg = make_color(130, 90, 180, 255);
    themes[THEME_PURPLE].button_fg = make_color(255, 255, 255, 255);
    themes[THEME_PURPLE].button_hover = make_color(150, 110, 200, 255);
    themes[THEME_PURPLE].button_pressed = make_color(110, 70, 160, 255);
    themes[THEME_PURPLE].text_primary = make_color(40, 20, 60, 255);
    themes[THEME_PURPLE].text_secondary = make_color(100, 70, 130, 255);
    themes[THEME_PURPLE].text_disabled = make_color(170, 150, 190, 255);
    
    // Initialize Custom Theme (copy of default)
    themes[THEME_CUSTOM] = themes[THEME_DEFAULT];
    themes[THEME_CUSTOM].type = THEME_CUSTOM;
    themes[THEME_CUSTOM].name = "Custom";
    
    initialized = 1;
    return 0;
}

void theme_manager_shutdown(void) {
    initialized = 0;
}

const theme_t* theme_manager_get_theme(theme_type_t type) {
    if (!initialized) {
        theme_manager_init();
    }
    
    if (type < 0 || type >= THEME_COUNT) {
        return NULL;
    }
    
    return &themes[type];
}

const theme_t* theme_manager_get_current(void) {
    if (!initialized) {
        theme_manager_init();
    }
    
    return &themes[current_theme_type];
}

int theme_manager_set_theme(theme_type_t type) {
    if (!initialized) {
        theme_manager_init();
    }
    
    if (type < 0 || type >= THEME_COUNT) {
        return -1;
    }
    
    current_theme_type = type;
    
    // Apply theme to desktop configuration
    desktop_config_t* config = desktop_config_get();
    if (config) {
        const theme_t* theme = &themes[type];
        config->desktop_bg_start = theme->desktop_bg_start;
        config->desktop_bg_end = theme->desktop_bg_end;
        config->taskbar_bg = theme->taskbar_bg;
        config->taskbar_fg = theme->taskbar_fg;
        config->window_title_active = theme->window_title_active;
        config->window_title_inactive = theme->window_title_inactive;
        config->window_border = theme->window_border;
        
        desktop_config_apply(config);
    }
    
    return 0;
}

const theme_t* theme_manager_get_by_index(uint32_t index) {
    if (!initialized) {
        theme_manager_init();
    }
    
    if (index >= THEME_COUNT) {
        return NULL;
    }
    
    return &themes[index];
}

uint32_t theme_manager_get_count(void) {
    return THEME_COUNT - 1;  // Exclude THEME_CUSTOM from count
}

int theme_manager_set_custom(const theme_t* custom_theme) {
    if (!initialized) {
        theme_manager_init();
    }
    
    if (!custom_theme) {
        return -1;
    }
    
    themes[THEME_CUSTOM] = *custom_theme;
    themes[THEME_CUSTOM].type = THEME_CUSTOM;
    themes[THEME_CUSTOM].name = "Custom";
    
    return 0;
}

void theme_manager_show_selector(void) {
    if (!initialized) {
        theme_manager_init();
    }
    
    // Create theme selector window
    window_t* theme_window = gui_create_window("Select Theme", 300, 150, 400, 400);
    if (!theme_window) {
        return;
    }
    
    theme_window->bg_color = make_color(240, 240, 240, 255);
    
    int32_t y_pos = 20;
    int32_t x_pos = 20;
    int32_t button_width = 360;
    int32_t button_height = 35;
    int32_t spacing = 10;
    
    // Title
    gui_create_label(theme_window, "Choose a theme:", x_pos, y_pos);
    y_pos += 35;
    
    // Create a button for each theme
    for (uint32_t i = 0; i < THEME_COUNT - 1; i++) {  // Exclude THEME_CUSTOM
        const theme_t* theme = &themes[i];
        
        // Create button with theme name
        widget_t* button = gui_create_button(theme_window, theme->name, x_pos, y_pos, button_width, button_height);
        if (button) {
            // Color the button with theme's window title color for preview
            button->bg_color = theme->window_title_active;
            button->fg_color = make_color(255, 255, 255, 255);
        }
        
        y_pos += button_height + spacing;
    }
    
    // Add instruction text
    y_pos += 10;
    gui_create_label(theme_window, "Note: Click a theme button to apply", x_pos, y_pos);
    y_pos += 25;
    gui_create_label(theme_window, "(Theme application not yet fully", x_pos, y_pos);
    y_pos += 20;
    gui_create_label(theme_window, " implemented in this demo)", x_pos, y_pos);
    
    // Show the window
    gui_show_window(theme_window);
    gui_focus_window(theme_window);
}
