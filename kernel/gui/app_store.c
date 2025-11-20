/**
 * Aurora OS - Application Store
 * 
 * GUI application for browsing and installing applications
 */

#include "app_store.h"
#include "gui.h"
#include "framebuffer.h"
#include "package_manager.h"

static window_t* g_store_window = NULL;
static store_category_t g_current_category = STORE_CAT_ALL;
static package_t g_displayed_packages[32];
static int g_displayed_count = 0;
static int g_scroll_offset = 0;

// Refresh package list
static void refresh_package_list(void) {
    uint8_t installed_only = (g_current_category == STORE_CAT_INSTALLED);
    g_displayed_count = pkgman_list_packages(g_displayed_packages, 32, installed_only);
}

// Render package list
static void render_package_list(window_t* window) {
    if (!window) return;
    
    int list_x = window->bounds.x + 200;
    int list_y = window->bounds.y + 60;
    int list_w = window->bounds.width - 220;
    int list_h = window->bounds.height - 80;
    
    // Background
    color_t bg = {40, 40, 50, 255};
    framebuffer_draw_rect(list_x, list_y, list_w, list_h, bg);
    
    // Draw packages
    int y = list_y + 10;
    int item_h = 80;
    
    for (int i = g_scroll_offset; i < g_displayed_count && y < list_y + list_h - item_h; i++) {
        package_t* pkg = &g_displayed_packages[i];
        
        // Item background
        color_t item_bg = pkg->installed ? 
            (color_t){60, 80, 100, 255} : 
            (color_t){50, 50, 60, 255};
        framebuffer_draw_rect(list_x + 10, y, list_w - 20, item_h - 5, item_bg);
        
        // Package name
        color_t name_color = {255, 255, 255, 255};
        framebuffer_draw_string(list_x + 20, y + 10, pkg->name, name_color, (color_t){0, 0, 0, 0});
        
        // Version
        color_t version_color = {150, 150, 150, 255};
        framebuffer_draw_string(list_x + 20, y + 30, pkg->version, version_color, (color_t){0, 0, 0, 0});
        
        // Description
        color_t desc_color = {180, 180, 180, 255};
        framebuffer_draw_string(list_x + 20, y + 50, pkg->description, desc_color, (color_t){0, 0, 0, 0});
        
        // Status indicator
        const char* status_text = pkg->installed ? "[INSTALLED]" : "[AVAILABLE]";
        color_t status_color = pkg->installed ? 
            (color_t){100, 255, 100, 255} : 
            (color_t){255, 255, 100, 255};
        framebuffer_draw_string(list_x + list_w - 130, y + 10, status_text, status_color, (color_t){0, 0, 0, 0});
        
        y += item_h;
    }
    
    // Draw scrollbar if needed
    if (g_displayed_count > 6) {
        int scrollbar_x = list_x + list_w - 15;
        int scrollbar_h = list_h - 20;
        int thumb_h = (scrollbar_h * 6) / g_displayed_count;
        int thumb_y = list_y + 10 + ((scrollbar_h - thumb_h) * g_scroll_offset) / (g_displayed_count - 6);
        
        color_t scrollbar_bg = {30, 30, 40, 255};
        framebuffer_draw_rect(scrollbar_x, list_y + 10, 10, scrollbar_h, scrollbar_bg);
        
        color_t thumb_color = {100, 150, 255, 255};
        framebuffer_draw_rect(scrollbar_x, thumb_y, 10, thumb_h, thumb_color);
    }
}

// Render category sidebar
static void render_sidebar(window_t* window) {
    if (!window) return;
    
    int sidebar_x = window->bounds.x + 10;
    int sidebar_y = window->bounds.y + 60;
    int sidebar_w = 180;
    
    const char* categories[] = {
        "All Apps",
        "Productivity",
        "Development",
        "Multimedia",
        "Games",
        "Utilities",
        "Installed"
    };
    
    for (int i = 0; i < 7; i++) {
        int btn_y = sidebar_y + (i * 40);
        
        color_t btn_color = (i == g_current_category) ?
            (color_t){80, 120, 200, 255} :
            (color_t){50, 50, 60, 255};
        
        framebuffer_draw_rect(sidebar_x, btn_y, sidebar_w, 35, btn_color);
        
        color_t text_color = {255, 255, 255, 255};
        framebuffer_draw_string(sidebar_x + 10, btn_y + 10, categories[i], text_color, (color_t){0, 0, 0, 0});
    }
}

// Custom paint function
static void store_paint(widget_t* widget) {
    if (!widget || !widget->parent_window) return;
    render_sidebar(widget->parent_window);
    render_package_list(widget->parent_window);
}

// Custom click handler
static void store_click(widget_t* widget, int32_t x, int32_t y) {
    if (!widget || !widget->parent_window) return;
    
    window_t* window = widget->parent_window;
    int sidebar_x = window->bounds.x + 10;
    int sidebar_y = window->bounds.y + 60;
    int sidebar_w = 180;
    
    // Check sidebar clicks
    if (x >= sidebar_x && x < sidebar_x + sidebar_w) {
        int relative_y = y - sidebar_y;
        if (relative_y >= 0 && relative_y < 280) {
            int category = relative_y / 40;
            if (category >= 0 && category < 7) {
                g_current_category = (store_category_t)category;
                g_scroll_offset = 0;
                refresh_package_list();
                gui_update();
            }
        }
    }
    
    // Check package list clicks
    int list_x = window->bounds.x + 200;
    int list_y = window->bounds.y + 60;
    int list_w = window->bounds.width - 220;
    
    if (x >= list_x && x < list_x + list_w) {
        int relative_y = y - list_y - 10;
        int item_h = 80;
        int clicked_idx = (relative_y / item_h) + g_scroll_offset;
        
        if (clicked_idx >= 0 && clicked_idx < g_displayed_count) {
            package_t* pkg = &g_displayed_packages[clicked_idx];
            
            // Toggle install/uninstall
            if (pkg->installed) {
                app_store_uninstall(pkg->name);
            } else {
                app_store_install(pkg->name);
            }
            
            refresh_package_list();
            gui_update();
        }
    }
}

window_t* launch_app_store(void) {
    // Initialize package manager
    pkgman_init();
    
    // Create window
    g_store_window = gui_create_window("Application Store", 200, 100, 900, 650);
    if (!g_store_window) {
        return NULL;
    }
    
    g_store_window->bg_color = (color_t){30, 30, 40, 255};
    
    // Create main content widget
    widget_t* content = gui_create_panel(g_store_window, 10, 50, 880, 590);
    if (content) {
        content->on_paint = store_paint;
        content->on_click = store_click;
    }
    
    // Load initial package list
    refresh_package_list();
    
    gui_show_window(g_store_window);
    return g_store_window;
}

void app_store_refresh(void) {
    pkgman_update();
    refresh_package_list();
    if (g_store_window) {
        gui_update();
    }
}

int app_store_install(const char* package_name) {
    int result = pkgman_install(package_name);
    if (result == 0) {
        refresh_package_list();
    }
    return result;
}

int app_store_uninstall(const char* package_name) {
    int result = pkgman_remove(package_name);
    if (result == 0) {
        refresh_package_list();
    }
    return result;
}
