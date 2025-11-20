/**
 * Aurora OS - Window Snapping and Tiling
 * 
 * Window snapping to screen edges and tiling features
 */

#include "window_snap.h"
#include "gui.h"
#include "framebuffer.h"

// Global snap state
static snap_config_t g_snap_config;
static uint8_t g_initialized = 0;
static uint8_t g_preview_visible = 0;
static snap_position_t g_preview_position = SNAP_NONE;

int window_snap_init(void) {
    if (g_initialized) {
        return 0;
    }
    
    // Initialize default configuration
    g_snap_config.enabled = 1;
    g_snap_config.snap_threshold = 20;  // 20 pixels from edge
    g_snap_config.show_preview = 1;
    
    g_preview_visible = 0;
    g_preview_position = SNAP_NONE;
    
    g_initialized = 1;
    return 0;
}

snap_position_t window_snap_check(window_t* window, int32_t x, int32_t y) {
    if (!g_initialized || !g_snap_config.enabled || !window) {
        return SNAP_NONE;
    }
    
    uint32_t screen_w = framebuffer_get_info()->width;
    uint32_t screen_h = framebuffer_get_info()->height;
    uint32_t threshold = g_snap_config.snap_threshold;
    
    // Check edges
    uint8_t near_left = (x < threshold);
    uint8_t near_right = (x + window->bounds.width > screen_w - threshold);
    uint8_t near_top = (y < threshold);
    uint8_t near_bottom = (y + window->bounds.height > screen_h - threshold);
    
    // Determine snap position
    if (near_top && near_left) {
        return SNAP_TOP_LEFT;
    } else if (near_top && near_right) {
        return SNAP_TOP_RIGHT;
    } else if (near_bottom && near_left) {
        return SNAP_BOTTOM_LEFT;
    } else if (near_bottom && near_right) {
        return SNAP_BOTTOM_RIGHT;
    } else if (near_left) {
        return SNAP_LEFT;
    } else if (near_right) {
        return SNAP_RIGHT;
    } else if (near_top) {
        return SNAP_MAXIMIZE;  // Drag to top = maximize
    }
    
    return SNAP_NONE;
}

int window_snap_apply(window_t* window, snap_position_t position) {
    if (!window) {
        return -1;
    }
    
    uint32_t screen_w = framebuffer_get_info()->width;
    uint32_t screen_h = framebuffer_get_info()->height;
    
    // Save current bounds for restore
    if (position != SNAP_NONE && !window->maximized) {
        window->normal_bounds = window->bounds;
    }
    
    switch (position) {
        case SNAP_LEFT:
            window->bounds.x = 0;
            window->bounds.y = 0;
            window->bounds.width = screen_w / 2;
            window->bounds.height = screen_h;
            break;
            
        case SNAP_RIGHT:
            window->bounds.x = screen_w / 2;
            window->bounds.y = 0;
            window->bounds.width = screen_w / 2;
            window->bounds.height = screen_h;
            break;
            
        case SNAP_TOP:
            window->bounds.x = 0;
            window->bounds.y = 0;
            window->bounds.width = screen_w;
            window->bounds.height = screen_h / 2;
            break;
            
        case SNAP_BOTTOM:
            window->bounds.x = 0;
            window->bounds.y = screen_h / 2;
            window->bounds.width = screen_w;
            window->bounds.height = screen_h / 2;
            break;
            
        case SNAP_TOP_LEFT:
            window->bounds.x = 0;
            window->bounds.y = 0;
            window->bounds.width = screen_w / 2;
            window->bounds.height = screen_h / 2;
            break;
            
        case SNAP_TOP_RIGHT:
            window->bounds.x = screen_w / 2;
            window->bounds.y = 0;
            window->bounds.width = screen_w / 2;
            window->bounds.height = screen_h / 2;
            break;
            
        case SNAP_BOTTOM_LEFT:
            window->bounds.x = 0;
            window->bounds.y = screen_h / 2;
            window->bounds.width = screen_w / 2;
            window->bounds.height = screen_h / 2;
            break;
            
        case SNAP_BOTTOM_RIGHT:
            window->bounds.x = screen_w / 2;
            window->bounds.y = screen_h / 2;
            window->bounds.width = screen_w / 2;
            window->bounds.height = screen_h / 2;
            break;
            
        case SNAP_MAXIMIZE:
            if (!window->maximized) {
                gui_maximize_window(window);
            }
            break;
            
        case SNAP_NONE:
            // Restore original bounds if was snapped
            if (window->maximized) {
                gui_restore_window(window);
            }
            break;
    }
    
    gui_update();
    return 0;
}

void window_snap_show_preview(snap_position_t position) {
    if (!g_snap_config.show_preview || position == SNAP_NONE) {
        return;
    }
    
    uint32_t screen_w = framebuffer_get_info()->width;
    uint32_t screen_h = framebuffer_get_info()->height;
    
    // Calculate preview bounds
    rect_t preview_bounds;
    
    switch (position) {
        case SNAP_LEFT:
            preview_bounds.x = 0;
            preview_bounds.y = 0;
            preview_bounds.width = screen_w / 2;
            preview_bounds.height = screen_h;
            break;
            
        case SNAP_RIGHT:
            preview_bounds.x = screen_w / 2;
            preview_bounds.y = 0;
            preview_bounds.width = screen_w / 2;
            preview_bounds.height = screen_h;
            break;
            
        case SNAP_TOP_LEFT:
            preview_bounds.x = 0;
            preview_bounds.y = 0;
            preview_bounds.width = screen_w / 2;
            preview_bounds.height = screen_h / 2;
            break;
            
        case SNAP_TOP_RIGHT:
            preview_bounds.x = screen_w / 2;
            preview_bounds.y = 0;
            preview_bounds.width = screen_w / 2;
            preview_bounds.height = screen_h / 2;
            break;
            
        case SNAP_BOTTOM_LEFT:
            preview_bounds.x = 0;
            preview_bounds.y = screen_h / 2;
            preview_bounds.width = screen_w / 2;
            preview_bounds.height = screen_h / 2;
            break;
            
        case SNAP_BOTTOM_RIGHT:
            preview_bounds.x = screen_w / 2;
            preview_bounds.y = screen_h / 2;
            preview_bounds.width = screen_w / 2;
            preview_bounds.height = screen_h / 2;
            break;
            
        case SNAP_MAXIMIZE:
            preview_bounds.x = 0;
            preview_bounds.y = 0;
            preview_bounds.width = screen_w;
            preview_bounds.height = screen_h;
            break;
            
        default:
            return;
    }
    
    // Draw semi-transparent preview overlay
    color_t preview_color = {100, 150, 255, 100};
    framebuffer_draw_rect(preview_bounds.x, preview_bounds.y, 
                preview_bounds.width, preview_bounds.height, preview_color);
    
    // Draw border
    color_t border_color = {150, 200, 255, 200};
    framebuffer_draw_rect_outline(preview_bounds.x, preview_bounds.y,
                preview_bounds.width, preview_bounds.height, border_color);
    
    g_preview_visible = 1;
    g_preview_position = position;
}

void window_snap_hide_preview(void) {
    if (g_preview_visible) {
        g_preview_visible = 0;
        g_preview_position = SNAP_NONE;
        gui_update();
    }
}

snap_config_t* window_snap_get_config(void) {
    if (!g_initialized) {
        window_snap_init();
    }
    return &g_snap_config;
}

int window_snap_handle_shortcut(window_t* window, uint32_t key) {
    if (!g_initialized || !window) {
        return 0;
    }
    
    // Handle Win+Arrow keys for snapping
    // Key codes would need to be defined properly
    switch (key) {
        case 0x4B: // Left arrow
            window_snap_apply(window, SNAP_LEFT);
            return 1;
        case 0x4D: // Right arrow
            window_snap_apply(window, SNAP_RIGHT);
            return 1;
        case 0x48: // Up arrow
            window_snap_apply(window, SNAP_MAXIMIZE);
            return 1;
        case 0x50: // Down arrow
            // Restore window
            if (window->maximized) {
                gui_restore_window(window);
            }
            return 1;
        default:
            return 0;
    }
}
