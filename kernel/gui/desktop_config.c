/**
 * Aurora OS - Desktop Configuration System Implementation
 * 
 * Manages desktop appearance and behavior settings
 */

#include "desktop_config.h"
#include "gui.h"
#include "font_manager.h"
#include "framebuffer.h"
#include "theme_manager.h"
#include "wallpaper_manager.h"
#include "../memory/memory.h"
#include "../../filesystem/vfs/vfs.h"

/* Configuration file path */
#define CONFIG_FILE_PATH "/etc/aurora/desktop.cfg"
#define CONFIG_DIR_PATH "/etc/aurora"

/* Configuration file magic number for validation */
#define CONFIG_MAGIC 0x41555243  /* "AURC" */
#define CONFIG_VERSION 1

/* Configuration file header */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t checksum;
} config_header_t;

// Global configuration
static desktop_config_t config;
static uint8_t config_initialized = 0;

/* Simple checksum calculation */
static uint32_t calculate_checksum(const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint32_t sum = 0;
    for (size_t i = 0; i < size; i++) {
        sum += bytes[i];
        sum = (sum << 1) | (sum >> 31); /* Rotate left by 1 */
    }
    return sum;
}

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
    
    // Widget window transparency (100 = fully opaque)
    config.widget_transparency = 100;
    
    // Taskbar settings
    config.taskbar_position = 0;      // Bottom
    config.taskbar_auto_hide = 0;     // Don't auto-hide
    config.taskbar_height = 40;       // 40 pixels
    
    // Desktop icon layout
    config.icon_grid_auto = 1;        // Auto-arrange
    config.icon_label_position = 0;   // Labels below icons
    
    // Window behavior
    config.double_click_titlebar = 0; // Maximize
    config.focus_follows_mouse = 0;   // Click to focus
    config.raise_on_focus = 1;        // Raise on focus
    
    // Cursor settings
    config.cursor_theme = 0;          // Default cursor
    config.cursor_blink_rate = 5;     // Medium blink rate
    
    // Sound settings
    config.enable_ui_sounds = 1;      // Enable sounds
    config.sound_volume = 70;         // 70% volume
    
    config_initialized = 1;
    
    // Initialize theme and wallpaper managers
    theme_manager_init();
    wallpaper_manager_init();
    
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
    if (!config_initialized) {
        return -1;
    }
    
    /* Create configuration directory if it doesn't exist */
    inode_t stat;
    if (vfs_stat(CONFIG_DIR_PATH, &stat) < 0) {
        if (vfs_mkdir(CONFIG_DIR_PATH) < 0) {
            /* Directory creation failed - this is not fatal if it already exists */
            /* Try to proceed anyway as the directory might exist */
        }
    }
    
    /* Open config file for writing with create and truncate flags */
    int fd = vfs_open(CONFIG_FILE_PATH, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        return -1;
    }
    
    /* Prepare header */
    config_header_t header;
    header.magic = CONFIG_MAGIC;
    header.version = CONFIG_VERSION;
    header.size = sizeof(desktop_config_t);
    header.checksum = calculate_checksum(&config, sizeof(desktop_config_t));
    
    /* Write header */
    int bytes_written = vfs_write(fd, &header, sizeof(config_header_t));
    if (bytes_written != (int)sizeof(config_header_t)) {
        vfs_close(fd);
        return -1;
    }
    
    /* Write configuration data */
    bytes_written = vfs_write(fd, &config, sizeof(desktop_config_t));
    if (bytes_written != (int)sizeof(desktop_config_t)) {
        vfs_close(fd);
        return -1;
    }
    
    vfs_close(fd);
    return 0;
}

int desktop_config_load(void) {
    /* Check if config file exists */
    inode_t stat;
    if (vfs_stat(CONFIG_FILE_PATH, &stat) < 0) {
        /* No config file, use defaults */
        if (!config_initialized) {
            desktop_config_init();
        }
        return 0;
    }
    
    /* Open config file for reading */
    int fd = vfs_open(CONFIG_FILE_PATH, O_RDONLY);
    if (fd < 0) {
        if (!config_initialized) {
            desktop_config_init();
        }
        return -1;
    }
    
    /* Read header */
    config_header_t header;
    int bytes_read = vfs_read(fd, &header, sizeof(config_header_t));
    if (bytes_read != (int)sizeof(config_header_t)) {
        vfs_close(fd);
        if (!config_initialized) {
            desktop_config_init();
        }
        return -1;
    }
    
    /* Validate header */
    if (header.magic != CONFIG_MAGIC) {
        vfs_close(fd);
        if (!config_initialized) {
            desktop_config_init();
        }
        return -1;
    }
    
    /* Check version compatibility */
    if (header.version > CONFIG_VERSION) {
        vfs_close(fd);
        if (!config_initialized) {
            desktop_config_init();
        }
        return -1;
    }
    
    /* Check size */
    if (header.size != sizeof(desktop_config_t)) {
        vfs_close(fd);
        if (!config_initialized) {
            desktop_config_init();
        }
        return -1;
    }
    
    /* Read configuration data into temporary buffer */
    desktop_config_t temp_config;
    bytes_read = vfs_read(fd, &temp_config, sizeof(desktop_config_t));
    vfs_close(fd);
    
    if (bytes_read != (int)sizeof(desktop_config_t)) {
        if (!config_initialized) {
            desktop_config_init();
        }
        return -1;
    }
    
    /* Validate checksum */
    uint32_t checksum = calculate_checksum(&temp_config, sizeof(desktop_config_t));
    if (checksum != header.checksum) {
        if (!config_initialized) {
            desktop_config_init();
        }
        return -1;
    }
    
    /* Copy validated configuration */
    config = temp_config;
    config_initialized = 1;
    
    /* Apply loaded configuration */
    font_manager_set_current(config.default_font);
    
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
    
    // Create settings window (increased height for new options)
    window_t* settings_window = gui_create_window("Desktop Settings", 150, 50, 600, 680);
    if (!settings_window) {
        return;
    }
    
    settings_window->bg_color = (color_t){240, 240, 240, 255};
    
    int32_t y_pos = 20;
    int32_t x_label = 20;
    int32_t x_control = 200;
    int32_t spacing = 35;
    
    // Font Selection Section
    gui_create_label(settings_window, "=== Appearance ===", x_label, y_pos);
    y_pos += 25;
    
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
    y_pos += 25;
    
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
    y_pos += 25;
    
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
    
    // Transparency Section
    gui_create_label(settings_window, "=== Transparency ===", x_label, y_pos);
    y_pos += 25;
    
    gui_create_label(settings_window, "Enable Transparency:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.enable_transparency ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    gui_create_label(settings_window, "Widget Transparency:", x_label, y_pos);
    // Show transparency level (0-100)
    char trans_label[16];
    uint8_t trans_val = config.widget_transparency;
    if (trans_val >= 100) {
        trans_label[0] = '1';
        trans_label[1] = '0';
        trans_label[2] = '0';
        trans_label[3] = '%';
        trans_label[4] = '\0';
    } else if (trans_val >= 10) {
        trans_label[0] = (trans_val / 10) + '0';
        trans_label[1] = (trans_val % 10) + '0';
        trans_label[2] = '%';
        trans_label[3] = '\0';
    } else {
        trans_label[0] = trans_val + '0';
        trans_label[1] = '%';
        trans_label[2] = '\0';
    }
    gui_create_button(settings_window, trans_label, x_control, y_pos - 5, 60, 25);
    y_pos += spacing;
    
    // Taskbar Section
    gui_create_label(settings_window, "=== Taskbar ===", x_label, y_pos);
    y_pos += 25;
    
    gui_create_label(settings_window, "Auto-hide Taskbar:", x_label, y_pos);
    gui_create_button(settings_window, 
        config.taskbar_auto_hide ? "[X]" : "[ ]", 
        x_control, y_pos - 5, 40, 25);
    y_pos += spacing;
    
    // Theme and Wallpaper Section
    gui_create_label(settings_window, "=== Theme & Wallpaper ===", x_label, y_pos);
    y_pos += 25;
    
    gui_create_label(settings_window, "Theme:", x_label, y_pos);
    gui_create_button(settings_window, "Change Theme...", x_control, y_pos - 5, 120, 25);
    y_pos += spacing;
    
    gui_create_label(settings_window, "Wallpaper:", x_label, y_pos);
    gui_create_button(settings_window, "Change Wallpaper...", x_control, y_pos - 5, 120, 25);
    y_pos += spacing;
    
    // Action buttons
    y_pos += 10;
    gui_create_button(settings_window, "Apply", 150, y_pos, 80, 30);
    gui_create_button(settings_window, "Reset", 240, y_pos, 80, 30);
    gui_create_button(settings_window, "Save", 330, y_pos, 80, 30);
    gui_create_button(settings_window, "Close", 420, y_pos, 80, 30);
    
    // Show the window
    gui_show_window(settings_window);
    gui_focus_window(settings_window);
}

void desktop_config_show_theme_selector(void) {
    theme_manager_show_selector();
}

void desktop_config_show_wallpaper_selector(void) {
    wallpaper_manager_show_selector();
}
