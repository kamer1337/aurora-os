/**
 * Aurora OS - Desktop Modules System Implementation
 * 
 * Implements a modular desktop architecture with pluggable components.
 */

#include "desktop_modules.h"
#include "quick_launch.h"
#include "system_tray.h"
#include "window_switcher.h"
#include "application.h"
#include "framebuffer.h"
#include "../memory/memory.h"

// Module registry
static desktop_module_t* modules[MODULE_COUNT];
static uint8_t module_system_initialized = 0;

// Forward declarations for module implementations
static void taskbar_module_init(void);
static void taskbar_module_update(void);
static void taskbar_module_draw(void);
static void taskbar_module_shutdown(void);

static void desktop_icons_module_init(void);
static void desktop_icons_module_update(void);
static void desktop_icons_module_draw(void);
static void desktop_icons_module_shutdown(void);

static void start_menu_module_init(void);
static void start_menu_module_update(void);
static void start_menu_module_draw(void);
static void start_menu_module_shutdown(void);

int desktop_modules_init(void) {
    if (module_system_initialized) {
        return 0;
    }
    
    // Initialize module structures
    for (int i = 0; i < MODULE_COUNT; i++) {
        modules[i] = (desktop_module_t*)kmalloc(sizeof(desktop_module_t));
        if (!modules[i]) {
            return -1;
        }
        modules[i]->enabled = 1;  // All modules enabled by default
    }
    
    // Configure Quick Launch module
    modules[MODULE_QUICK_LAUNCH]->type = MODULE_QUICK_LAUNCH;
    modules[MODULE_QUICK_LAUNCH]->name = "Quick Launch";
    modules[MODULE_QUICK_LAUNCH]->init = quick_launch_init;
    modules[MODULE_QUICK_LAUNCH]->update = NULL;
    modules[MODULE_QUICK_LAUNCH]->draw = quick_launch_draw;
    modules[MODULE_QUICK_LAUNCH]->shutdown = NULL;
    
    // Configure System Tray module
    modules[MODULE_SYSTEM_TRAY]->type = MODULE_SYSTEM_TRAY;
    modules[MODULE_SYSTEM_TRAY]->name = "System Tray";
    modules[MODULE_SYSTEM_TRAY]->init = system_tray_init;
    modules[MODULE_SYSTEM_TRAY]->update = system_tray_update;
    modules[MODULE_SYSTEM_TRAY]->draw = system_tray_draw;
    modules[MODULE_SYSTEM_TRAY]->shutdown = NULL;
    
    // Configure Window Switcher module
    modules[MODULE_WINDOW_SWITCHER]->type = MODULE_WINDOW_SWITCHER;
    modules[MODULE_WINDOW_SWITCHER]->name = "Window Switcher";
    modules[MODULE_WINDOW_SWITCHER]->init = NULL;
    modules[MODULE_WINDOW_SWITCHER]->update = window_switcher_update;
    modules[MODULE_WINDOW_SWITCHER]->draw = window_switcher_draw;
    modules[MODULE_WINDOW_SWITCHER]->shutdown = NULL;
    
    // Configure Taskbar module
    modules[MODULE_TASKBAR]->type = MODULE_TASKBAR;
    modules[MODULE_TASKBAR]->name = "Taskbar";
    modules[MODULE_TASKBAR]->init = taskbar_module_init;
    modules[MODULE_TASKBAR]->update = taskbar_module_update;
    modules[MODULE_TASKBAR]->draw = taskbar_module_draw;
    modules[MODULE_TASKBAR]->shutdown = taskbar_module_shutdown;
    
    // Configure Desktop Icons module
    modules[MODULE_DESKTOP_ICONS]->type = MODULE_DESKTOP_ICONS;
    modules[MODULE_DESKTOP_ICONS]->name = "Desktop Icons";
    modules[MODULE_DESKTOP_ICONS]->init = desktop_icons_module_init;
    modules[MODULE_DESKTOP_ICONS]->update = desktop_icons_module_update;
    modules[MODULE_DESKTOP_ICONS]->draw = desktop_icons_module_draw;
    modules[MODULE_DESKTOP_ICONS]->shutdown = desktop_icons_module_shutdown;
    
    // Configure Start Menu module
    modules[MODULE_START_MENU]->type = MODULE_START_MENU;
    modules[MODULE_START_MENU]->name = "Start Menu";
    modules[MODULE_START_MENU]->init = start_menu_module_init;
    modules[MODULE_START_MENU]->update = start_menu_module_update;
    modules[MODULE_START_MENU]->draw = start_menu_module_draw;
    modules[MODULE_START_MENU]->shutdown = start_menu_module_shutdown;
    
    // Initialize all enabled modules
    for (int i = 0; i < MODULE_COUNT; i++) {
        if (modules[i]->enabled && modules[i]->init) {
            modules[i]->init();
        }
    }
    
    module_system_initialized = 1;
    return 0;
}

void desktop_modules_shutdown(void) {
    if (!module_system_initialized) {
        return;
    }
    
    // Shutdown all modules
    for (int i = 0; i < MODULE_COUNT; i++) {
        if (modules[i]) {
            if (modules[i]->shutdown) {
                modules[i]->shutdown();
            }
            kfree(modules[i]);
            modules[i] = NULL;
        }
    }
    
    module_system_initialized = 0;
}

void desktop_modules_update(void) {
    if (!module_system_initialized) {
        return;
    }
    
    for (int i = 0; i < MODULE_COUNT; i++) {
        if (modules[i] && modules[i]->enabled && modules[i]->update) {
            modules[i]->update();
        }
    }
}

void desktop_modules_draw(void) {
    if (!module_system_initialized) {
        return;
    }
    
    for (int i = 0; i < MODULE_COUNT; i++) {
        if (modules[i] && modules[i]->enabled && modules[i]->draw) {
            modules[i]->draw();
        }
    }
}

int desktop_module_enable(desktop_module_type_t type) {
    if (type >= MODULE_COUNT) {
        return -1;
    }
    
    if (modules[type]) {
        modules[type]->enabled = 1;
        if (modules[type]->init) {
            modules[type]->init();
        }
        return 0;
    }
    
    return -1;
}

int desktop_module_disable(desktop_module_type_t type) {
    if (type >= MODULE_COUNT) {
        return -1;
    }
    
    if (modules[type]) {
        modules[type]->enabled = 0;
        if (modules[type]->shutdown) {
            modules[type]->shutdown();
        }
        return 0;
    }
    
    return -1;
}

int desktop_module_is_enabled(desktop_module_type_t type) {
    if (type >= MODULE_COUNT) {
        return 0;
    }
    
    return modules[type] ? modules[type]->enabled : 0;
}

desktop_module_t* desktop_module_get(desktop_module_type_t type) {
    if (type >= MODULE_COUNT) {
        return NULL;
    }
    
    return modules[type];
}

/* Desktop icons data structure */
typedef struct desktop_icon {
    char* label;
    int x;
    int y;
    float depth;  // Depth coordinate (0.0 = foreground, 1.0 = background)
    app_type_t app;
    struct desktop_icon* next;
} desktop_icon_t;

static desktop_icon_t* desktop_icons = NULL;
static float depth_offset = 0.0f;  // User-controlled depth navigation
static desktop_icon_t* selected_icon = NULL;  // Currently selected icon for depth manipulation

/* Depth navigation functions for 3D desktop icons */

/**
 * Navigate depth forward (bring closer)
 */
void desktop_icons_depth_forward(void) {
    depth_offset -= 0.1f;  // Move closer
    if (depth_offset < -1.0f) {
        depth_offset = -1.0f;  // Clamp
    }
}

/**
 * Navigate depth backward (push away)
 */
void desktop_icons_depth_backward(void) {
    depth_offset += 0.1f;  // Move away
    if (depth_offset > 1.0f) {
        depth_offset = 1.0f;  // Clamp
    }
}

/**
 * Reset depth navigation
 */
void desktop_icons_depth_reset(void) {
    depth_offset = 0.0f;
}

/**
 * Get current depth offset
 */
float desktop_icons_get_depth_offset(void) {
    return depth_offset;
}

/**
 * Adjust selected icon depth
 */
void desktop_icons_adjust_selected_depth(float delta) {
    if (selected_icon) {
        selected_icon->depth += delta;
        if (selected_icon->depth < 0.0f) {
            selected_icon->depth = 0.0f;
        }
        if (selected_icon->depth > 1.0f) {
            selected_icon->depth = 1.0f;
        }
    }
}

// Placeholder implementations for module-specific functions
static void taskbar_module_init(void) {
    // Taskbar initialization
    // Set up taskbar data structures if needed
}

static void taskbar_module_update(void) {
    // Taskbar update logic
    // Update taskbar state, time display, etc.
}

static void taskbar_module_draw(void) {
    // Taskbar drawing is handled in gui.c via gui_draw_taskbar()
    // This function can be used for additional taskbar elements
}

static void taskbar_module_shutdown(void) {
    // Taskbar cleanup
    // Free any taskbar-specific resources
}

static void desktop_icons_module_init(void) {
    /* Create default desktop icons for common applications with depth */
    desktop_icon_t* icon;
    
    /* File Manager icon - closest depth */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "File Manager";
        icon->x = 50;
        icon->y = 100;
        icon->depth = 0.0f;  // Foreground
        icon->app = APP_FILE_MANAGER;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* System Info icon - slightly back */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "System Info";
        icon->x = 50;
        icon->y = 200;
        icon->depth = 0.2f;  // Slightly behind
        icon->app = APP_SYSTEM_INFO;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* Disk Manager icon - medium depth */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "Disk Manager";
        icon->x = 50;
        icon->y = 300;
        icon->depth = 0.4f;  // Medium depth
        icon->app = APP_DISK_MANAGER;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* My PC icon - further back */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "My PC";
        icon->x = 50;
        icon->y = 400;
        icon->depth = 0.6f;  // Further back
        icon->app = APP_MY_PC;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* Recycle Bin icon - background */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "Recycle Bin";
        icon->x = 50;
        icon->y = 500;
        icon->depth = 0.8f;  // Background
        icon->app = APP_RECYCLE_BIN;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* Clock & Calendar icon - second column, varied depths */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "Clock";
        icon->x = 200;
        icon->y = 100;
        icon->depth = 0.3f;
        icon->app = APP_CLOCK;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* Music Player icon */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "Music";
        icon->x = 200;
        icon->y = 200;
        icon->depth = 0.5f;
        icon->app = APP_MUSIC_PLAYER;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* Video Player icon */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "Video";
        icon->x = 200;
        icon->y = 300;
        icon->depth = 0.7f;
        icon->app = APP_VIDEO_PLAYER;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
    
    /* Goals Manager icon */
    icon = (desktop_icon_t*)kmalloc(sizeof(desktop_icon_t));
    if (icon) {
        icon->label = "Goals";
        icon->x = 200;
        icon->y = 400;
        icon->depth = 0.4f;
        icon->app = APP_GOALS_MANAGER;
        icon->next = desktop_icons;
        desktop_icons = icon;
    }
}

static void desktop_icons_module_update(void) {
    // Desktop icons update logic
    // Handle icon selection, drag and drop, depth navigation
    // This would be called on each frame to process icon interactions
    
    // Note: In a full implementation, keyboard/mouse input would be processed here
    // For now, depth navigation could be controlled via keyboard shortcuts
}

/* Helper function to calculate perspective scale based on depth */
static float calculate_perspective_scale(float depth, float depth_offset) {
    // Apply depth offset (user navigation)
    float effective_depth = depth + depth_offset;
    
    // Clamp depth to valid range [0.0, 1.0]
    if (effective_depth < 0.0f) effective_depth = 0.0f;
    if (effective_depth > 1.0f) effective_depth = 1.0f;
    
    // Calculate scale: closer objects (depth=0) are larger (scale=1.0)
    // farther objects (depth=1) are smaller (scale=0.3)
    float scale = 1.0f - (effective_depth * 0.7f);
    
    return scale;
}

/* Helper function to calculate depth-based alpha (transparency) */
static uint8_t calculate_depth_alpha(float depth, float depth_offset) {
    // Apply depth offset
    float effective_depth = depth + depth_offset;
    
    // Clamp depth to valid range
    if (effective_depth < 0.0f) effective_depth = 0.0f;
    if (effective_depth > 1.0f) effective_depth = 1.0f;
    
    // Calculate alpha: closer objects are fully opaque (255)
    // farther objects fade (minimum 100)
    uint8_t alpha = (uint8_t)(255 - (effective_depth * 155));
    
    return alpha;
}

static void desktop_icons_module_draw(void) {
    // Desktop icons drawing with 3D depth perspective
    // Icons farther away appear smaller and more transparent
    
    desktop_icon_t* icon = desktop_icons;
    
    // Sort icons by depth for proper rendering order (back to front)
    // For simplicity, we'll just iterate in reverse depth order
    
    // First pass: render background icons (depth > 0.5)
    icon = desktop_icons;
    while (icon) {
        if (icon->depth > 0.5f) {
            float scale = calculate_perspective_scale(icon->depth, depth_offset);
            uint8_t alpha = calculate_depth_alpha(icon->depth, depth_offset);
            
            // Calculate scaled icon size (base size 48x48)
            int icon_size = (int)(48.0f * scale);
            int label_size = (int)(12.0f * scale);
            
            // Draw icon rectangle (simplified - in full implementation would draw actual icon)
            color_t icon_color = {80, 120, 200, alpha};
            framebuffer_draw_rect(icon->x, icon->y, icon_size, icon_size, icon_color);
            
            // Draw label below icon (simplified)
            // In full implementation, would use font rendering with proper scaling
        }
        icon = icon->next;
    }
    
    // Second pass: render foreground icons (depth <= 0.5)
    icon = desktop_icons;
    while (icon) {
        if (icon->depth <= 0.5f) {
            float scale = calculate_perspective_scale(icon->depth, depth_offset);
            uint8_t alpha = calculate_depth_alpha(icon->depth, depth_offset);
            
            // Calculate scaled icon size
            int icon_size = (int)(48.0f * scale);
            
            // Draw icon rectangle with depth perspective
            color_t icon_color = {80, 120, 200, alpha};
            framebuffer_draw_rect(icon->x, icon->y, icon_size, icon_size, icon_color);
            
            // Highlight selected icon
            if (icon == selected_icon) {
                color_t highlight_color = {255, 200, 0, 200};
                framebuffer_draw_rect_outline(icon->x - 2, icon->y - 2, 
                                             icon_size + 4, icon_size + 4, highlight_color);
            }
        }
        icon = icon->next;
    }
}

static void desktop_icons_module_shutdown(void) {
    /* Free desktop icons */
    while (desktop_icons) {
        desktop_icon_t* next = desktop_icons->next;
        kfree(desktop_icons);
        desktop_icons = next;
    }
}

static void start_menu_module_init(void) {
    // Start menu initialization
    // Set up start menu data structures and default items
}

static void start_menu_module_update(void) {
    // Start menu update logic
    // Handle menu item animations, transitions, etc.
}

static void start_menu_module_draw(void) {
    // Start menu drawing is handled in gui.c
    // This function can be used for additional menu decorations
}

static void start_menu_module_shutdown(void) {
    // Start menu cleanup
    // Free start menu resources
}
