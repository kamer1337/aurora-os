/**
 * Aurora OS - GUI Framework Implementation
 * 
 * Basic GUI framework providing windowing, widgets, and event handling.
 */

#include "gui.h"
#include "framebuffer.h"
#include "gui_effects.h"
#include "application.h"
#include "font_manager.h"
#include "desktop_config.h"
#include "live_wallpaper.h"
#include "../memory/memory.h"
#include "../drivers/mouse.h"
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include <stddef.h>

// String functions (minimal implementations)
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static char* strdup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* copy = (char*)kmalloc(len + 1);
    if (copy) {
        for (size_t i = 0; i <= len; i++) {
            copy[i] = str[i];
        }
    }
    return copy;
}

// Global state
static window_t* window_list = NULL;
static window_t* focused_window = NULL;
static int gui_initialized = 0;

// Input state
static int32_t cursor_x = 0;
static int32_t cursor_y = 0;
static uint8_t prev_mouse_buttons = 0;

// Double-click detection state
static uint32_t last_click_time = 0;
static int32_t last_click_x = 0;
static int32_t last_click_y = 0;
#define DOUBLE_CLICK_TIME 500    // milliseconds (based on timer ticks at 100Hz = 50 ticks)
#define DOUBLE_CLICK_DISTANCE 10 // pixels

// Window dragging state
static window_t* dragging_window = NULL;
static int32_t drag_offset_x = 0;
static int32_t drag_offset_y = 0;

// Desktop environment state
static uint8_t start_menu_visible = 0;
static uint8_t context_menu_visible = 0;
static window_t* context_menu_window = NULL;
static int32_t context_menu_x = 0;
static int32_t context_menu_y = 0;

// Start menu animation state
static float start_menu_animation = 0.0f;  // 0.0 = closed, 1.0 = open
static uint8_t start_menu_animating = 0;
static uint8_t start_menu_opening = 0;     // 1 = opening, 0 = closing
#define START_MENU_ANIMATION_SPEED 0.15f   // Animation step per frame

// Start menu keyboard navigation state
static int32_t start_menu_selected_item = 0;
static const int32_t start_menu_item_count = 16;  // Updated for Linux Installer

// Desktop icons
typedef struct desktop_icon {
    const char* label;
    int32_t x;
    int32_t y;
    app_type_t app_type;
    uint8_t has_app;  // Whether this icon launches an app
} desktop_icon_t;

static desktop_icon_t desktop_icons[] = {
    {"File Manager", 30, 80, APP_FILE_MANAGER, 1},
    {"Terminal", 30, 180, APP_TERMINAL, 1},
    {"Settings", 30, 280, APP_SETTINGS, 1},
    {"System Info", 30, 380, APP_SYSTEM_INFO, 1},
    {"Paint", 30, 480, APP_PAINT_EDITOR, 1},
    {"Images", 30, 580, APP_IMAGE_VIEWER, 1},
    {"Notebook", 150, 80, APP_NOTEBOOK, 1},
    {"My PC", 150, 180, APP_MY_PC, 1},
    {"Recycle Bin", 150, 280, APP_RECYCLE_BIN, 1}
};
static const int32_t desktop_icon_count = 9;

// Forward declarations for desktop environment functions
static void gui_draw_start_menu(void);
static void gui_draw_context_menu(void);
static void gui_draw_desktop_icon(desktop_icon_t* icon);
void gui_show_power_options(void);

// External timer function for double-click timing
extern uint32_t timer_get_ticks(void);

// Helper function to check for double-click
static int is_double_click(int32_t x, int32_t y) {
    uint32_t current_time = timer_get_ticks();
    
    // Check time difference (at 100Hz, 50 ticks = 500ms)
    uint32_t time_diff = current_time - last_click_time;
    
    // Check distance from last click
    int32_t dx = x - last_click_x;
    int32_t dy = y - last_click_y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    
    // Update last click info
    last_click_time = current_time;
    last_click_x = x;
    last_click_y = y;
    
    // Check if this qualifies as a double-click
    if (time_diff < 50 && dx < DOUBLE_CLICK_DISTANCE && dy < DOUBLE_CLICK_DISTANCE) {
        // Reset to prevent triple-click detection
        last_click_time = 0;
        return 1;
    }
    
    return 0;
}

int gui_init(void) {
    if (gui_initialized) {
        return 0;
    }
    
    // Initialize framebuffer with default settings
    if (framebuffer_init(0, 0, 0) != 0) {
        return -1;
    }
    
    // Initialize font manager and desktop configuration
    font_manager_init();
    desktop_config_init();
    
    // Initialize live wallpaper system
    live_wallpaper_init();
    
    window_list = NULL;
    focused_window = NULL;
    gui_initialized = 1;
    
    // Initialize application framework
    app_init();
    
    // Clear screen
    framebuffer_clear(COLOR_BLUE);
    
    return 0;
}

void gui_shutdown(void) {
    // Shutdown live wallpaper
    live_wallpaper_shutdown();
    
    // Free all windows
    window_t* window = window_list;
    while (window) {
        window_t* next = window->next;
        gui_destroy_window(window);
        window = next;
    }
    
    window_list = NULL;
    focused_window = NULL;
    gui_initialized = 0;
}

void gui_update(void) {
    if (!gui_initialized) return;
    
    // Update live wallpaper animations (pass current cursor position for parallax)
    desktop_config_t* cfg = desktop_config_get();
    if (cfg && cfg->enable_live_wallpaper) {
        live_wallpaper_update(16, cursor_x, cursor_y);  // ~60 FPS (16ms per frame)
    }
    
    // Update start menu animation
    if (start_menu_animating) {
        if (start_menu_opening) {
            // Opening animation - slide up
            start_menu_animation += START_MENU_ANIMATION_SPEED;
            if (start_menu_animation >= 1.0f) {
                start_menu_animation = 1.0f;
                start_menu_animating = 0;
            }
        } else {
            // Closing animation - slide down
            start_menu_animation -= START_MENU_ANIMATION_SPEED;
            if (start_menu_animation <= 0.0f) {
                start_menu_animation = 0.0f;
                start_menu_animating = 0;
                start_menu_visible = 0;
            }
        }
    }
    
    // Redraw desktop background
    gui_draw_desktop();
    
    // Draw all windows
    window_t* window = window_list;
    while (window) {
        if (window->visible) {
            gui_draw_window(window);
        }
        window = window->next;
    }
    
    // Draw taskbar
    gui_draw_taskbar();
    
    // Draw start menu if visible (including during animation)
    if (start_menu_visible || start_menu_animating) {
        gui_draw_start_menu();
    }
    
    // Draw context menu if visible
    if (context_menu_visible) {
        gui_draw_context_menu();
    }
    
    // Draw cursor last (on top of everything)
    gui_draw_cursor();
}

void gui_process_event(event_t* event) {
    if (!gui_initialized || !event) return;
    
    // Process event based on type
    switch (event->type) {
        case EVENT_MOUSE_DOWN:
            // Check if clicked on start menu
            if (start_menu_visible) {
                framebuffer_info_t* fb = framebuffer_get_info();
                if (fb) {
                    uint32_t menu_width = 250;
                    uint32_t menu_height = 400;
                    uint32_t menu_x = 5;
                    uint32_t menu_y = fb->height - 40 - menu_height;
                    rect_t menu_rect = {menu_x, menu_y, menu_width, menu_height};
                    
                    if (gui_point_in_rect(event->x, event->y, &menu_rect)) {
                        // Clicked inside start menu - handle menu items
                        uint32_t item_height = 40;
                        uint32_t header_height = 70;
                        uint32_t item_spacing = 5;
                        int32_t relative_y = event->y - (int32_t)menu_y - (int32_t)header_height;
                        
                        // Check if clicked on a menu item (not header)
                        if (relative_y >= 0 && relative_y < (int32_t)((item_height + item_spacing) * 16)) {
                            uint32_t item_index = (uint32_t)relative_y / (item_height + item_spacing);
                            
                            // Launch the corresponding application
                            switch (item_index) {
                                case 0: // File Manager
                                    app_launch(APP_FILE_MANAGER);
                                    break;
                                case 1: // Terminal
                                    app_launch(APP_TERMINAL);
                                    break;
                                case 2: // Paint Editor
                                    app_launch(APP_PAINT_EDITOR);
                                    break;
                                case 3: // Image Viewer
                                    app_launch(APP_IMAGE_VIEWER);
                                    break;
                                case 4: // Notebook
                                    app_launch(APP_NOTEBOOK);
                                    break;
                                case 5: // Apps Uninstaller
                                    app_launch(APP_UNINSTALLER);
                                    break;
                                case 6: // Task Manager
                                    app_launch(APP_TASK_MANAGER);
                                    break;
                                case 7: // Control Panel
                                    app_launch(APP_CONTROL_PANEL);
                                    break;
                                case 8: // System Settings
                                    app_launch(APP_SETTINGS);
                                    break;
                                case 9: // System Information
                                    app_launch(APP_SYSTEM_INFO);
                                    break;
                                case 10: // Calculator
                                    app_launch(APP_CALCULATOR);
                                    break;
                                case 11: // Goals Manager
                                    app_launch(APP_GOALS_MANAGER);
                                    break;
                                case 12: // Linux Installer
                                    app_launch(APP_LINUX_INSTALLER);
                                    break;
                                case 13: // Help & Support
                                    app_launch(APP_HELP);
                                    break;
                                case 14: // Toggle Wallpaper
                                    {
                                        desktop_config_t* cfg = desktop_config_get();
                                        if (cfg) {
                                            cfg->enable_live_wallpaper = !cfg->enable_live_wallpaper;
                                            live_wallpaper_set_enabled(cfg->enable_live_wallpaper);
                                            if (cfg->enable_live_wallpaper) {
                                                // Set default forest scene
                                                live_wallpaper_set_type(WALLPAPER_NATURE_FOREST);
                                            }
                                        }
                                    }
                                    break;
                                case 15: // Power Options
                                    gui_show_power_options();
                                    app_launch(APP_POWER_OPTIONS);
                                    break;
                            }
                        }
                        
                        // Close the menu with animation after launching
                        start_menu_animating = 1;
                        start_menu_opening = 0;
                        return;
                    } else {
                        // Clicked outside start menu - close it with animation
                        start_menu_animating = 1;
                        start_menu_opening = 0;
                    }
                }
            }
            
            // Check if clicked on context menu
            if (context_menu_visible) {
                rect_t context_rect = {context_menu_x, context_menu_y, 150, 120};
                if (gui_point_in_rect(event->x, event->y, &context_rect)) {
                    // Clicked inside context menu - determine which item
                    uint32_t item_height = 30;
                    uint32_t relative_y = event->y - context_menu_y;
                    uint32_t item_index = relative_y / item_height;
                    
                    if (context_menu_window && item_index < 4) {
                        switch (item_index) {
                            case 0: // Restore
                                gui_restore_window(context_menu_window);
                                break;
                            case 1: // Minimize
                                gui_minimize_window(context_menu_window);
                                break;
                            case 2: // Maximize
                                if (context_menu_window->maximized) {
                                    gui_restore_window(context_menu_window);
                                } else {
                                    gui_maximize_window(context_menu_window);
                                }
                                break;
                            case 3: // Close
                                gui_destroy_window(context_menu_window);
                                break;
                        }
                    }
                    gui_hide_context_menu();
                    return;
                } else {
                    // Clicked outside context menu - close it
                    gui_hide_context_menu();
                }
            }
            
            // Check if clicked on taskbar
            {
                framebuffer_info_t* fb = framebuffer_get_info();
                if (fb && event->y >= (int32_t)(fb->height - 40)) {
                    uint32_t taskbar_y = fb->height - 40;
                    
                    // Check if clicked on Start button
                    rect_t start_button_rect = {5, taskbar_y + 5, 80, 30};
                    if (gui_point_in_rect(event->x, event->y, &start_button_rect)) {
                        gui_toggle_start_menu();
                        return;
                    }
                    
                    // Hide start menu with animation if clicking elsewhere on taskbar
                    if (start_menu_visible && !start_menu_animating) {
                        start_menu_animating = 1;
                        start_menu_opening = 0;
                    }
                    
                    // Clicked on taskbar - check window list buttons
                    uint32_t button_x = 95;
                    uint32_t button_width = 150;
                    
                    window_t* window = window_list;
                    while (window && button_x + button_width < fb->width - 100) {
                        rect_t button_rect = {button_x, taskbar_y + 5, button_width, 30};
                        if (gui_point_in_rect(event->x, event->y, &button_rect)) {
                            if (window->minimized) {
                                gui_restore_window(window);
                            } else {
                                gui_focus_window(window);
                            }
                            return;
                        }
                        button_x += button_width + 5;
                        window = window->next;
                    }
                    // Clicked on taskbar but not on a window button - don't process further
                    return;
                }
            }
            
            // Check if clicked on a desktop icon
            {
                framebuffer_info_t* fb = framebuffer_get_info();
                if (fb && event->y < (int32_t)(fb->height - 40)) {
                    for (int i = 0; i < desktop_icon_count; i++) {
                        uint32_t icon_width = 80;
                        uint32_t icon_height = 90;
                        rect_t icon_rect = {
                            desktop_icons[i].x,
                            desktop_icons[i].y,
                            icon_width,
                            icon_height
                        };
                        
                        if (gui_point_in_rect(event->x, event->y, &icon_rect)) {
                            // Launch app on double-click
                            if (desktop_icons[i].has_app && is_double_click(event->x, event->y)) {
                                app_launch(desktop_icons[i].app_type);
                            }
                            // Single click just selects the icon (visual feedback would go here)
                            return;
                        }
                    }
                }
            }
            
            // Check if we clicked on a window's titlebar (for dragging)
            {
                window_t* window = window_list;
                window_t* clicked_window = NULL;
                
                // Find which window was clicked (iterate in reverse Z-order)
                while (window) {
                    if (window->visible) {
                        rect_t titlebar_rect = {
                            window->bounds.x,
                            window->bounds.y,
                            window->bounds.width,
                            window->has_titlebar ? 30u : 0u
                        };
                        
                        // Check if clicked on titlebar buttons
                        if (window->has_titlebar && gui_point_in_rect(event->x, event->y, &titlebar_rect)) {
                            uint32_t button_y = window->bounds.y + 4;
                            uint32_t button_size = 16;
                            uint32_t button_spacing = 4;
                            
                            // Close button
                            uint32_t close_x = window->bounds.x + window->bounds.width - 20;
                            rect_t close_rect = {close_x, button_y, button_size, button_size};
                            if (gui_point_in_rect(event->x, event->y, &close_rect)) {
                                gui_destroy_window(window);
                                break;
                            }
                            
                            // Maximize button
                            uint32_t max_x = close_x - button_size - button_spacing;
                            rect_t max_rect = {max_x, button_y, button_size, button_size};
                            if (gui_point_in_rect(event->x, event->y, &max_rect)) {
                                if (window->maximized) {
                                    gui_restore_window(window);
                                } else {
                                    gui_maximize_window(window);
                                }
                                break;
                            }
                            
                            // Minimize button
                            uint32_t min_x = max_x - button_size - button_spacing;
                            rect_t min_rect = {min_x, button_y, button_size, button_size};
                            if (gui_point_in_rect(event->x, event->y, &min_rect)) {
                                gui_minimize_window(window);
                                break;
                            }
                            
                            // If clicked on titlebar (but not buttons), start dragging or show context menu
                            if (event->button == 2) {
                                // Right-click on titlebar - show context menu
                                gui_show_context_menu(window, event->x, event->y);
                                break;
                            } else {
                                // Left-click - check for double-click to maximize/restore
                                if (is_double_click(event->x, event->y)) {
                                    if (window->maximized) {
                                        gui_restore_window(window);
                                    } else {
                                        gui_maximize_window(window);
                                    }
                                    break;
                                }
                                // Single click - start dragging
                                clicked_window = window;
                                dragging_window = window;
                                drag_offset_x = event->x - window->bounds.x;
                                drag_offset_y = event->y - window->bounds.y;
                                gui_focus_window(window);
                                break;
                            }
                        }
                        
                        // Check if clicked anywhere on window
                        if (gui_point_in_rect(event->x, event->y, &window->bounds)) {
                            clicked_window = window;
                            gui_focus_window(window);
                        }
                    }
                    window = window->next;
                }
                
                // Check if click is on a widget in the focused window
                if (clicked_window && !dragging_window) {
                    widget_t* widget = clicked_window->widgets;
                    while (widget) {
                        rect_t abs_rect = {
                            clicked_window->bounds.x + widget->bounds.x,
                            clicked_window->bounds.y + (clicked_window->has_titlebar ? 30 : 0) + widget->bounds.y,
                            widget->bounds.width,
                            widget->bounds.height
                        };
                        
                        if (gui_point_in_rect(event->x, event->y, &abs_rect)) {
                            if (widget->on_click) {
                                widget->on_click(widget, event->x, event->y);
                            }
                            break;
                        }
                        widget = widget->next;
                    }
                }
            }
            break;
            
        case EVENT_MOUSE_UP:
            // Stop dragging
            dragging_window = NULL;
            break;
            
        case EVENT_MOUSE_MOVE:
            // Handle window dragging
            if (dragging_window) {
                dragging_window->bounds.x = event->x - drag_offset_x;
                dragging_window->bounds.y = event->y - drag_offset_y;
                
                // Keep window on screen
                if (dragging_window->bounds.x < 0) dragging_window->bounds.x = 0;
                if (dragging_window->bounds.y < 0) dragging_window->bounds.y = 0;
            }
            break;
            
        case EVENT_PAINT:
            gui_update();
            break;
            
        case EVENT_KEY_DOWN:
            // Handle keyboard navigation for start menu
            if (start_menu_visible) {
                switch (event->key) {
                    case 0x48: // Up arrow
                        if (start_menu_selected_item > 0) {
                            start_menu_selected_item--;
                        } else {
                            // Wrap to bottom
                            start_menu_selected_item = start_menu_item_count - 1;
                        }
                        break;
                    case 0x50: // Down arrow
                    case '\t': // Tab key - also navigates down
                        if (start_menu_selected_item < start_menu_item_count - 1) {
                            start_menu_selected_item++;
                        } else {
                            // Wrap to top
                            start_menu_selected_item = 0;
                        }
                        break;
                    case 0x4B: // Left arrow - previous page (first item)
                        start_menu_selected_item = 0;
                        break;
                    case 0x4D: // Right arrow - next page (last item)
                        start_menu_selected_item = start_menu_item_count - 1;
                        break;
                    case ' ': // Space key - also activates selected item
                    case 0x0D: // Enter key (also '\r')
                    case '\n':
                        // Launch the selected application
                        switch (start_menu_selected_item) {
                            case 0: // File Manager
                                app_launch(APP_FILE_MANAGER);
                                break;
                            case 1: // Terminal
                                app_launch(APP_TERMINAL);
                                break;
                            case 2: // Paint Editor
                                app_launch(APP_PAINT_EDITOR);
                                break;
                            case 3: // Image Viewer
                                app_launch(APP_IMAGE_VIEWER);
                                break;
                            case 4: // Notebook
                                app_launch(APP_NOTEBOOK);
                                break;
                            case 5: // Apps Uninstaller
                                app_launch(APP_UNINSTALLER);
                                break;
                            case 6: // Task Manager
                                app_launch(APP_TASK_MANAGER);
                                break;
                            case 7: // Control Panel
                                app_launch(APP_CONTROL_PANEL);
                                break;
                            case 8: // System Settings
                                app_launch(APP_SETTINGS);
                                break;
                            case 9: // System Information
                                app_launch(APP_SYSTEM_INFO);
                                break;
                            case 10: // Calculator
                                app_launch(APP_CALCULATOR);
                                break;
                            case 11: // Goals Manager
                                app_launch(APP_GOALS_MANAGER);
                                break;
                            case 12: // Linux Installer
                                app_launch(APP_LINUX_INSTALLER);
                                break;
                            case 13: // Help & Support
                                app_launch(APP_HELP);
                                break;
                            case 14: // Toggle Wallpaper
                                {
                                    desktop_config_t* cfg = desktop_config_get();
                                    if (cfg) {
                                        cfg->enable_live_wallpaper = !cfg->enable_live_wallpaper;
                                        live_wallpaper_set_enabled(cfg->enable_live_wallpaper);
                                        if (cfg->enable_live_wallpaper) {
                                            // Set default forest scene
                                            live_wallpaper_set_type(WALLPAPER_NATURE_FOREST);
                                        }
                                    }
                                }
                                break;
                            case 15: // Power Options
                                app_launch(APP_POWER_OPTIONS);
                                break;
                        }
                        // Close menu with animation
                        start_menu_animating = 1;
                        start_menu_opening = 0;
                        start_menu_selected_item = 0;
                        break;
                    case 0x1B: // Escape key
                        // Close menu with animation
                        start_menu_animating = 1;
                        start_menu_opening = 0;
                        start_menu_selected_item = 0;
                        break;
                }
                return;
            }
            break;
            
        default:
            break;
    }
}

window_t* gui_create_window(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!gui_initialized) return NULL;
    
    window_t* window = (window_t*)kmalloc(sizeof(window_t));
    if (!window) return NULL;
    
    window->title = strdup(title);
    window->bounds.x = x;
    window->bounds.y = y;
    window->bounds.width = width;
    window->bounds.height = height;
    window->normal_bounds = window->bounds;
    window->bg_color = COLOR_WHITE;
    window->title_color = (color_t){0, 0, 128, 255};  // Dark blue
    window->visible = 1;
    window->focused = 0;
    window->has_border = 1;
    window->has_titlebar = 1;
    window->minimized = 0;
    window->maximized = 0;
    window->transparency = 100;  // Fully opaque by default
    window->widgets = NULL;
    window->next = window_list;
    
    window_list = window;
    
    return window;
}

void gui_destroy_window(window_t* window) {
    if (!window) return;
    
    // Free all widgets
    widget_t* widget = window->widgets;
    while (widget) {
        widget_t* next = widget->next;
        if (widget->text) {
            kfree(widget->text);
        }
        kfree(widget);
        widget = next;
    }
    
    // Remove from window list
    if (window_list == window) {
        window_list = window->next;
    } else {
        window_t* prev = window_list;
        while (prev && prev->next != window) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = window->next;
        }
    }
    
    if (focused_window == window) {
        focused_window = window_list;
    }
    
    if (window->title) {
        kfree(window->title);
    }
    kfree(window);
}

void gui_close_window(window_t* window) {
    // gui_close_window is an alias for gui_destroy_window
    gui_destroy_window(window);
}

void gui_show_window(window_t* window) {
    if (window) {
        window->visible = 1;
    }
}

void gui_hide_window(window_t* window) {
    if (window) {
        window->visible = 0;
    }
}

void gui_focus_window(window_t* window) {
    if (focused_window) {
        focused_window->focused = 0;
    }
    focused_window = window;
    if (window) {
        window->focused = 1;
    }
}

void gui_draw_window(window_t* window) {
    if (!window || !window->visible) return;
    
    uint32_t title_height = window->has_titlebar ? 24 : 0;
    
    // Draw 3D shadow for depth (5-pixel offset, 8-pixel blur)
    if (window->focused) {
        gui_draw_shadow(window->bounds.x, window->bounds.y, 
                       window->bounds.width, window->bounds.height, 5, 8);
    } else {
        // Lighter shadow for unfocused windows
        gui_draw_shadow(window->bounds.x, window->bounds.y, 
                       window->bounds.width, window->bounds.height, 3, 5);
    }
    
    // Draw window border with rounded corners
    if (window->has_border) {
        gui_draw_rounded_rect(window->bounds.x, window->bounds.y,
                             window->bounds.width, window->bounds.height,
                             12, COLOR_DARK_GRAY);  // Increased corner radius from 8 to 12
    }
    
    // Draw titlebar with gradient
    if (window->has_titlebar) {
        color_t title_color1, title_color2;
        if (window->focused) {
            // Active window - vivid blue gradient
            title_color1 = (color_t){45, 170, 255, 255};  // Brighter, more vivid blue
            title_color2 = (color_t){10, 120, 220, 255};  // Deeper vivid blue
        } else {
            // Inactive window - lighter gray gradient
            title_color1 = (color_t){165, 165, 165, 255};
            title_color2 = (color_t){125, 125, 125, 255};
        }
        
        gui_draw_gradient(window->bounds.x + 1, window->bounds.y + 1,
                         window->bounds.width - 2, title_height,
                         title_color1, title_color2);
        
        // Draw window title with shadow for depth
        if (window->title) {
            // Draw text shadow
            font_manager_draw_string(window->bounds.x + 9, window->bounds.y + 9,
                                  window->title, COLOR_BLACK, title_color1);
            // Draw text
            font_manager_draw_string(window->bounds.x + 8, window->bounds.y + 8,
                                  window->title, COLOR_WHITE, title_color1);
        }
        
        // Draw window control buttons with 3D effects
        uint32_t button_size = 16;
        uint32_t button_spacing = 4;
        uint32_t button_y = window->bounds.y + 4;
        
        // Close button (red X) with 3D effect
        uint32_t close_x = window->bounds.x + window->bounds.width - 20;
        color_t vivid_red = (color_t){255, 60, 60, 255};  // Brighter red
        gui_draw_3d_button(close_x, button_y, button_size, button_size, vivid_red, 0);
        font_manager_draw_string(close_x + 4, button_y + 4, "X", COLOR_WHITE, vivid_red);
        
        // Maximize button (square) with 3D effect
        uint32_t max_x = close_x - button_size - button_spacing;
        color_t max_color = window->maximized ? (color_t){120, 120, 120, 255} : (color_t){60, 180, 75, 255};  // Vivid green
        gui_draw_3d_button(max_x, button_y, button_size, button_size, max_color, 0);
        framebuffer_draw_rect_outline(max_x + 3, button_y + 3, 10, 10, COLOR_WHITE);
        
        // Minimize button (dash) with 3D effect
        uint32_t min_x = max_x - button_size - button_spacing;
        color_t vivid_yellow = (color_t){255, 200, 50, 255};  // Brighter yellow
        gui_draw_3d_button(min_x, button_y, button_size, button_size, vivid_yellow, 0);
        framebuffer_draw_hline(min_x + 3, min_x + 13, button_y + 12, COLOR_WHITE);
    }
    
    // Draw window background
    framebuffer_draw_rect(window->bounds.x + 1, window->bounds.y + title_height + 1,
                         window->bounds.width - 2, window->bounds.height - title_height - 2,
                         window->bg_color);
    
    // Draw widgets
    widget_t* widget = window->widgets;
    while (widget) {
        if (widget->visible) {
            gui_draw_widget(widget, window->bounds.x, window->bounds.y + title_height);
        }
        widget = widget->next;
    }
}

widget_t* gui_create_button(window_t* window, const char* text, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!window) return NULL;
    
    widget_t* widget = (widget_t*)kmalloc(sizeof(widget_t));
    if (!widget) return NULL;
    
    widget->type = WIDGET_BUTTON;
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.width = width;
    widget->bounds.height = height;
    widget->bg_color = COLOR_LIGHT_GRAY;
    widget->fg_color = COLOR_BLACK;
    widget->text = strdup(text);
    widget->visible = 1;
    widget->enabled = 1;
    widget->on_paint = NULL;
    widget->on_click = NULL;
    widget->parent_window = window;
    widget->next = window->widgets;
    
    window->widgets = widget;
    
    return widget;
}

widget_t* gui_create_label(window_t* window, const char* text, int32_t x, int32_t y) {
    if (!window) return NULL;
    
    widget_t* widget = (widget_t*)kmalloc(sizeof(widget_t));
    if (!widget) return NULL;
    
    widget->type = WIDGET_LABEL;
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.width = strlen(text) * font_manager_get_char_advance();
    widget->bounds.height = font_manager_get_char_height();
    widget->bg_color = COLOR_WHITE;
    widget->fg_color = COLOR_BLACK;
    widget->text = strdup(text);
    widget->visible = 1;
    widget->enabled = 1;
    widget->on_paint = NULL;
    widget->on_click = NULL;
    widget->parent_window = window;
    widget->next = window->widgets;
    
    window->widgets = widget;
    
    return widget;
}

widget_t* gui_create_panel(window_t* window, int32_t x, int32_t y, uint32_t width, uint32_t height) {
    if (!window) return NULL;
    
    widget_t* widget = (widget_t*)kmalloc(sizeof(widget_t));
    if (!widget) return NULL;
    
    widget->type = WIDGET_PANEL;
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.width = width;
    widget->bounds.height = height;
    widget->bg_color = COLOR_LIGHT_GRAY;
    widget->fg_color = COLOR_BLACK;
    widget->text = NULL;
    widget->visible = 1;
    widget->enabled = 1;
    widget->on_paint = NULL;
    widget->on_click = NULL;
    widget->parent_window = window;
    widget->next = window->widgets;
    
    window->widgets = widget;
    
    return widget;
}

void gui_set_widget_click_handler(widget_t* widget, void (*handler)(widget_t*, int32_t, int32_t)) {
    if (widget) {
        widget->on_click = handler;
    }
}

void gui_draw_widget(widget_t* widget, int32_t window_x, int32_t window_y) {
    if (!widget || !widget->visible) return;
    
    int32_t abs_x = window_x + widget->bounds.x;
    int32_t abs_y = window_y + widget->bounds.y;
    
    switch (widget->type) {
        case WIDGET_BUTTON:
            // Draw button with 3D effect
            gui_draw_3d_button(abs_x, abs_y, widget->bounds.width, widget->bounds.height,
                              widget->bg_color, 0);
            
            // Draw button text (centered) with shadow for depth
            if (widget->text) {
                uint32_t text_width = strlen(widget->text) * font_manager_get_char_width();
                uint32_t text_x = abs_x + (widget->bounds.width - text_width) / 2;
                uint32_t text_y = abs_y + (widget->bounds.height - font_manager_get_char_height()) / 2;
                
                // Draw text shadow for depth
                font_manager_draw_string(text_x + 1, text_y + 1, widget->text,
                                      COLOR_DARK_GRAY, widget->bg_color);
                // Draw text
                font_manager_draw_string(text_x, text_y, widget->text,
                                      widget->fg_color, widget->bg_color);
            }
            break;
            
        case WIDGET_LABEL:
            // Draw label text
            if (widget->text) {
                font_manager_draw_string(abs_x, abs_y, widget->text,
                                      widget->fg_color, widget->bg_color);
            }
            break;
            
        case WIDGET_PANEL:
            // Draw panel background
            framebuffer_draw_rect(abs_x, abs_y, widget->bounds.width, widget->bounds.height,
                                widget->bg_color);
            
            // Draw panel border
            framebuffer_draw_rect_outline(abs_x, abs_y, widget->bounds.width, widget->bounds.height,
                                        COLOR_GRAY);
            break;
            
        default:
            break;
    }
}

int gui_point_in_rect(int32_t x, int32_t y, rect_t* rect) {
    if (!rect) return 0;
    
    return (x >= rect->x && x < rect->x + (int32_t)rect->width &&
            y >= rect->y && y < rect->y + (int32_t)rect->height);
}

void gui_draw_desktop(void) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Check if live wallpaper is enabled
    desktop_config_t* cfg = desktop_config_get();
    if (cfg && cfg->enable_live_wallpaper && live_wallpaper_is_enabled()) {
        // Draw live wallpaper (excludes taskbar area)
        live_wallpaper_draw(fb->width, fb->height - 40);
    } else {
        // Draw default background with vivid gradient (bright sky blue to lighter blue)
        for (uint32_t y = 0; y < fb->height - 40; y++) {
            uint8_t r = 40 + (y * 40 / fb->height);
            uint8_t g = 150 + (y * 30 / fb->height);
            uint8_t b = 230 + (y * 25 / fb->height);
            color_t line_color = {r, g, b, 255};
            framebuffer_draw_hline(0, fb->width - 1, y, line_color);
        }
    }
    
    // Draw desktop title
    const char* os_name = "Aurora OS Desktop";
    uint32_t text_width = strlen(os_name) * font_manager_get_char_width();
    uint32_t text_x = fb->width / 2 - text_width / 2;
    uint32_t text_y = 20;
    font_manager_draw_string(text_x, text_y, os_name, COLOR_WHITE, 
                          (color_t){0, 0, 0, 0});  // Transparent background
    
    // Draw desktop icons
    for (int i = 0; i < desktop_icon_count; i++) {
        gui_draw_desktop_icon(&desktop_icons[i]);
    }
}

void gui_draw_taskbar(void) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    uint32_t taskbar_height = 40;
    uint32_t taskbar_y = fb->height - taskbar_height;
    
    // Draw taskbar background with slight gradient
    gui_draw_gradient(0, taskbar_y, fb->width, taskbar_height,
                     (color_t){50, 50, 54, 255},   // Slightly lighter dark gray
                     (color_t){40, 40, 44, 255});  // Darker gray
    
    // Draw top border
    framebuffer_draw_hline(0, fb->width - 1, taskbar_y, (color_t){100, 100, 100, 255});
    
    // Draw "Start" button with vivid color and rounded corners
    gui_draw_rounded_rect(5, taskbar_y + 5, 80, 30, 4, (color_t){20, 140, 230, 255});  // Vivid blue
    font_manager_draw_string(15, taskbar_y + 13, "Aurora OS", COLOR_WHITE,
                          (color_t){20, 140, 230, 255});
    
    // Draw window list (taskbar buttons for open windows)
    uint32_t button_x = 95;
    uint32_t button_width = 150;
    uint32_t button_height = 30;
    
    window_t* window = window_list;
    while (window && button_x + button_width < fb->width - 100) {
        // Show all windows in taskbar (including minimized)
        color_t btn_color;
        if (window->minimized) {
            btn_color = (color_t){45, 45, 50, 255};  // Slightly lighter for minimized
        } else if (window->focused) {
            btn_color = (color_t){80, 80, 90, 255};  // Vivid highlight for focused
        } else {
            btn_color = (color_t){60, 60, 68, 255};  // Medium for unfocused
        }
        
        // Draw window button with rounded corners
        gui_draw_rounded_rect(button_x, taskbar_y + 5, button_width, button_height, 4, btn_color);
        framebuffer_draw_rect_outline(button_x, taskbar_y + 5, button_width, button_height, 
                                      (color_t){110, 110, 110, 255});  // Lighter outline
        
        // Draw window title (truncated if needed)
        if (window->title) {
            char truncated[20];
            size_t len = strlen(window->title);
            if (len > 18) {
                for (size_t i = 0; i < 15; i++) {
                    truncated[i] = window->title[i];
                }
                truncated[15] = '.';
                truncated[16] = '.';
                truncated[17] = '.';
                truncated[18] = '\0';
                font_manager_draw_string(button_x + 5, taskbar_y + 13, truncated, COLOR_WHITE, btn_color);
            } else {
                font_manager_draw_string(button_x + 5, taskbar_y + 13, window->title, COLOR_WHITE, btn_color);
            }
        }
        
        button_x += button_width + 5;
        window = window->next;
    }
    
    // Draw system tray area with real time
    uint32_t tray_x = fb->width - 100;
    char time_str[16];
    timer_get_time_string(time_str, sizeof(time_str));
    font_manager_draw_string(tray_x, taskbar_y + 13, time_str, COLOR_WHITE, 
                          (color_t){45, 45, 48, 255});
}

void gui_init_input(void) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Initialize mouse driver
    mouse_init();
    mouse_set_bounds(fb->width, fb->height);
    
    // Initialize keyboard driver
    keyboard_init();
    
    // Set initial cursor position
    cursor_x = fb->width / 2;
    cursor_y = fb->height / 2;
    prev_mouse_buttons = 0;
}

void gui_handle_input(void) {
    if (!gui_initialized) return;
    
    // Get mouse state
    mouse_state_t* mouse = mouse_get_state();
    if (mouse) {
        // Update cursor position
        cursor_x = mouse->x;
        cursor_y = mouse->y;
        
        // Handle mouse button events
        uint8_t buttons = mouse->buttons;
        
        // Left button pressed
        if ((buttons & MOUSE_LEFT_BUTTON) && !(prev_mouse_buttons & MOUSE_LEFT_BUTTON)) {
            event_t event = {
                .type = EVENT_MOUSE_DOWN,
                .x = cursor_x,
                .y = cursor_y,
                .button = 0,  // 0 = left button
                .key = 0,
                .data = NULL
            };
            gui_process_event(&event);
        }
        
        // Right button pressed
        if ((buttons & MOUSE_RIGHT_BUTTON) && !(prev_mouse_buttons & MOUSE_RIGHT_BUTTON)) {
            event_t event = {
                .type = EVENT_MOUSE_DOWN,
                .x = cursor_x,
                .y = cursor_y,
                .button = 2,  // 2 = right button (for context menus)
                .key = 0,
                .data = NULL
            };
            gui_process_event(&event);
        }
        
        // Left button released
        if (!(buttons & MOUSE_LEFT_BUTTON) && (prev_mouse_buttons & MOUSE_LEFT_BUTTON)) {
            event_t event = {
                .type = EVENT_MOUSE_UP,
                .x = cursor_x,
                .y = cursor_y,
                .button = 0,
                .key = 0,
                .data = NULL
            };
            gui_process_event(&event);
        }
        
        // Right button released
        if (!(buttons & MOUSE_RIGHT_BUTTON) && (prev_mouse_buttons & MOUSE_RIGHT_BUTTON)) {
            event_t event = {
                .type = EVENT_MOUSE_UP,
                .x = cursor_x,
                .y = cursor_y,
                .button = 2,
                .key = 0,
                .data = NULL
            };
            gui_process_event(&event);
        }
        
        // Mouse moved
        if (mouse->delta_x != 0 || mouse->delta_y != 0) {
            event_t event = {
                .type = EVENT_MOUSE_MOVE,
                .x = cursor_x,
                .y = cursor_y,
                .button = 0,
                .key = 0,
                .data = NULL
            };
            gui_process_event(&event);
        }
        
        prev_mouse_buttons = buttons;
    }
    
    // Handle keyboard input
    if (keyboard_has_input()) {
        char c = keyboard_getchar();
        
        event_t event = {
            .type = EVENT_KEY_DOWN,
            .x = 0,
            .y = 0,
            .button = 0,
            .key = (uint32_t)c,
            .data = NULL
        };
        gui_process_event(&event);
    }
}

void gui_draw_cursor(void) {
    if (!gui_initialized) return;
    
    // Draw a simple arrow cursor (11x16 pixels)
    int32_t x = cursor_x;
    int32_t y = cursor_y;
    
    // Cursor shape (simple arrow)
    static const uint8_t cursor_shape[16] = {
        0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF,
        0xF8, 0xD8, 0x8C, 0x0C, 0x06, 0x06, 0x03, 0x00
    };
    
    color_t cursor_color = COLOR_WHITE;
    color_t outline_color = COLOR_BLACK;
    
    // Draw cursor outline (black)
    for (int32_t cy = 0; cy < 16; cy++) {
        uint8_t row = cursor_shape[cy];
        for (int32_t cx = 0; cx < 8; cx++) {
            if (row & (0x80 >> cx)) {
                // Draw outline pixels
                framebuffer_draw_pixel(x + cx - 1, y + cy, outline_color);
                framebuffer_draw_pixel(x + cx + 1, y + cy, outline_color);
                framebuffer_draw_pixel(x + cx, y + cy - 1, outline_color);
                framebuffer_draw_pixel(x + cx, y + cy + 1, outline_color);
            }
        }
    }
    
    // Draw cursor (white)
    for (int32_t cy = 0; cy < 16; cy++) {
        uint8_t row = cursor_shape[cy];
        for (int32_t cx = 0; cx < 8; cx++) {
            if (row & (0x80 >> cx)) {
                framebuffer_draw_pixel(x + cx, y + cy, cursor_color);
            }
        }
    }
}

void gui_get_cursor_pos(int32_t* x, int32_t* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
}

void gui_minimize_window(window_t* window) {
    if (!window || window->minimized) return;
    
    window->minimized = 1;
    window->visible = 0;
    
    // If this was the focused window, focus another one
    if (focused_window == window) {
        focused_window = NULL;
        window_t* w = window_list;
        while (w) {
            if (w != window && w->visible) {
                gui_focus_window(w);
                break;
            }
            w = w->next;
        }
    }
}

void gui_maximize_window(window_t* window) {
    if (!window || window->maximized) return;
    
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    // Save current bounds
    window->normal_bounds = window->bounds;
    
    // Maximize to full screen (minus taskbar)
    window->bounds.x = 0;
    window->bounds.y = 0;
    window->bounds.width = fb->width;
    window->bounds.height = fb->height - 40; // Account for taskbar
    window->maximized = 1;
}

void gui_restore_window(window_t* window) {
    if (!window) return;
    
    if (window->minimized) {
        window->minimized = 0;
        window->visible = 1;
        gui_focus_window(window);
    } else if (window->maximized) {
        // Restore to normal bounds
        window->bounds = window->normal_bounds;
        window->maximized = 0;
    }
}

// Desktop environment functions

static void gui_draw_start_menu(void) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    uint32_t menu_width = 250;
    uint32_t menu_height = 725;  // Increased height for Linux Installer
    uint32_t menu_x = 5;
    
    // Apply slide-up animation using eased animation value
    float eased_anim = gui_ease(start_menu_animation, EASE_OUT_QUAD);
    int32_t anim_offset = (int32_t)((1.0f - eased_anim) * menu_height);
    int32_t menu_y = fb->height - 40 - menu_height + anim_offset;
    
    // Calculate visible portion during animation
    uint32_t visible_height = (uint32_t)(menu_height * eased_anim);
    if (visible_height < 1) visible_height = 1;
    
    // Draw menu background with shadow
    gui_draw_shadow(menu_x, menu_y, menu_width, visible_height, 4, 8);
    framebuffer_draw_rect(menu_x, menu_y, menu_width, visible_height,
                        (color_t){45, 45, 48, 255});
    
    // Draw menu border
    framebuffer_draw_rect_outline(menu_x, menu_y, menu_width, visible_height,
                                 (color_t){80, 80, 85, 255});
    
    // Only draw content if enough menu is visible
    if (eased_anim < 0.1f) return;
    
    // Draw menu header
    uint32_t header_height = 60;
    if (visible_height >= header_height) {
        framebuffer_draw_rect(menu_x, menu_y, menu_width, header_height,
                            (color_t){0, 120, 215, 255});
        font_manager_draw_string(menu_x + 10, menu_y + 10, "Aurora OS", COLOR_WHITE,
                              (color_t){0, 120, 215, 255});
        font_manager_draw_string(menu_x + 10, menu_y + 30, "Start Menu", 
                              (color_t){200, 200, 200, 255},
                              (color_t){0, 120, 215, 255});
    }
    
    // Draw menu items (updated to include Linux Installer)
    const char* items[] = {
        "File Manager",
        "Terminal",
        "Paint Editor",
        "Image Viewer",
        "Notebook",
        "Apps Uninstaller",
        "Task Manager",
        "Control Panel",
        "System Settings",
        "System Information",
        "Calculator",
        "Goals Manager",
        "Linux Installer",
        "Help & Support",
        "Toggle Wallpaper",
        "Power Options"
    };
    
    uint32_t item_height = 40;
    uint32_t item_y = menu_y + 70;
    
    for (int i = 0; i < 16; i++) {
        // Only draw items that are visible
        if (item_y - menu_y + item_height > visible_height) break;
        
        // Draw item background (highlight selected item)
        color_t item_bg;
        if (i == start_menu_selected_item) {
            item_bg = (color_t){0, 120, 215, 255};  // Highlighted (blue)
        } else {
            item_bg = (color_t){55, 55, 60, 255};  // Normal
        }
        framebuffer_draw_rect(menu_x + 5, item_y, menu_width - 10, item_height,
                            item_bg);
        
        // Draw item text
        font_manager_draw_string(menu_x + 15, item_y + 12, items[i],
                              COLOR_WHITE, item_bg);
        
        item_y += item_height + 5;
    }
}

static void gui_draw_context_menu(void) {
    if (!context_menu_visible || !context_menu_window) return;
    
    uint32_t menu_width = 150;
    uint32_t menu_height = 120;
    
    // Draw menu background with shadow
    gui_draw_shadow(context_menu_x, context_menu_y, menu_width, menu_height, 3, 5);
    framebuffer_draw_rect(context_menu_x, context_menu_y, menu_width, menu_height,
                        (color_t){50, 50, 55, 255});
    
    // Draw menu border
    framebuffer_draw_rect_outline(context_menu_x, context_menu_y, 
                                 menu_width, menu_height,
                                 (color_t){80, 80, 85, 255});
    
    // Draw menu items
    const char* items[] = {"Restore", "Minimize", "Maximize", "Close"};
    uint32_t item_height = 30;
    uint32_t item_y = context_menu_y;
    
    for (int i = 0; i < 4; i++) {
        // Draw item text
        font_manager_draw_string(context_menu_x + 10, item_y + 8, items[i],
                              COLOR_WHITE, (color_t){50, 50, 55, 255});
        
        // Draw separator line
        if (i < 3) {
            framebuffer_draw_hline(context_menu_x, context_menu_x + menu_width - 1,
                                 item_y + item_height, 
                                 (color_t){70, 70, 75, 255});
        }
        
        item_y += item_height;
    }
}

static void gui_draw_desktop_icon(desktop_icon_t* icon) {
    if (!icon) return;
    
    uint32_t icon_width = 80;
    uint32_t icon_size = 48;
    
    // Draw icon with rounded corners and vivid colors
    color_t icon_color;
    switch(icon->app_type) {
        case APP_FILE_MANAGER:
            icon_color = (color_t){255, 180, 50, 255};  // Vivid orange for files
            break;
        case APP_TERMINAL:
            icon_color = (color_t){80, 80, 80, 255};    // Dark gray for terminal
            break;
        case APP_SETTINGS:
            icon_color = (color_t){120, 120, 255, 255}; // Vivid purple for settings
            break;
        case APP_SYSTEM_INFO:
            icon_color = (color_t){50, 200, 100, 255};  // Vivid green for info
            break;
        default:
            icon_color = (color_t){120, 170, 255, 255}; // Default vivid blue
            break;
    }
    
    gui_draw_rounded_rect(icon->x + 16, icon->y, icon_size, icon_size, 8, icon_color);
    framebuffer_draw_rect_outline(icon->x + 16, icon->y, icon_size, icon_size, COLOR_WHITE);
    
    // Draw icon label with background
    uint32_t label_len = strlen(icon->label);
    uint32_t label_width = label_len * 8;
    uint32_t label_x = icon->x + (icon_width - label_width) / 2;
    uint32_t label_y = icon->y + icon_size + 8;
    
    // Draw label background (semi-transparent with rounded corners)
    gui_draw_rounded_rect(label_x - 2, label_y - 2, label_width + 4, 12, 
                         3, (color_t){0, 0, 0, 128});
    
    // Draw label text
    font_manager_draw_string(label_x, label_y, icon->label, COLOR_WHITE,
                          (color_t){0, 0, 0, 0});
}

void gui_toggle_start_menu(void) {
    if (!start_menu_visible && !start_menu_animating) {
        // Start opening animation
        start_menu_visible = 1;
        start_menu_animating = 1;
        start_menu_opening = 1;
        start_menu_animation = 0.0f;
        context_menu_visible = 0;
    } else if (start_menu_visible && !start_menu_animating) {
        // Start closing animation
        start_menu_animating = 1;
        start_menu_opening = 0;
    }
}

void gui_show_context_menu(window_t* window, int32_t x, int32_t y) {
    if (!window) return;
    
    context_menu_window = window;
    context_menu_x = x;
    context_menu_y = y;
    context_menu_visible = 1;
    
    // Hide start menu with animation when showing context menu
    if (start_menu_visible && !start_menu_animating) {
        start_menu_animating = 1;
        start_menu_opening = 0;
    }
}

void gui_hide_context_menu(void) {
    context_menu_visible = 0;
    context_menu_window = NULL;
}

int gui_is_start_menu_visible(void) {
    return start_menu_visible;
}

window_t* gui_get_window_list(void) {
    return window_list;
}

void gui_set_focused_window(window_t* window) {
    if (!window) return;
    gui_focus_window(window);
}

void gui_bring_to_front(window_t* window) {
    if (!window || !window_list) return;
    
    // Remove window from its current position
    if (window == window_list) {
        window_list = window->next;
    } else {
        window_t* prev = window_list;
        while (prev && prev->next != window) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = window->next;
        }
    }
    
    // Add to front of list
    window->next = window_list;
    window_list = window;
}

void gui_set_window_transparency(window_t* window, uint8_t transparency) {
    if (!window) return;
    
    // Clamp transparency to 0-100 range
    if (transparency > 100) {
        transparency = 100;
    }
    
    window->transparency = transparency;
}

uint8_t gui_get_window_transparency(window_t* window) {
    if (!window) return 100;
    return window->transparency;
}

// Power options dialog state
static window_t* power_options_window = NULL;

// Power button click handlers
static void power_shutdown_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    // In a real OS, this would trigger a system shutdown
    // For now, show a confirmation message and close the dialog
    if (power_options_window) {
        gui_destroy_window(power_options_window);
        power_options_window = NULL;
    }
    
    // Create shutdown confirmation dialog
    window_t* confirm = gui_create_window("Shutdown", 350, 250, 300, 150);
    if (confirm) {
        confirm->bg_color = (color_t){240, 240, 245, 255};
        gui_create_label(confirm, "System is shutting down...", 30, 40);
        gui_create_label(confirm, "Please save your work.", 50, 70);
        gui_create_button(confirm, "OK", 110, 100, 80, 35);
        gui_show_window(confirm);
        gui_focus_window(confirm);
    }
}

static void power_restart_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    // In a real OS, this would trigger a system restart
    if (power_options_window) {
        gui_destroy_window(power_options_window);
        power_options_window = NULL;
    }
    
    // Create restart confirmation dialog
    window_t* confirm = gui_create_window("Restart", 350, 250, 300, 150);
    if (confirm) {
        confirm->bg_color = (color_t){240, 240, 245, 255};
        gui_create_label(confirm, "System is restarting...", 40, 40);
        gui_create_label(confirm, "Please wait.", 80, 70);
        gui_create_button(confirm, "OK", 110, 100, 80, 35);
        gui_show_window(confirm);
        gui_focus_window(confirm);
    }
}

static void power_sleep_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    // In a real OS, this would trigger sleep mode
    if (power_options_window) {
        gui_destroy_window(power_options_window);
        power_options_window = NULL;
    }
    
    // Create sleep confirmation dialog
    window_t* confirm = gui_create_window("Sleep", 350, 250, 300, 150);
    if (confirm) {
        confirm->bg_color = (color_t){240, 240, 245, 255};
        gui_create_label(confirm, "Entering sleep mode...", 45, 40);
        gui_create_label(confirm, "Press any key to wake.", 40, 70);
        gui_create_button(confirm, "OK", 110, 100, 80, 35);
        gui_show_window(confirm);
        gui_focus_window(confirm);
    }
}

static void power_cancel_click(widget_t* widget, int32_t x, int32_t y) {
    (void)widget;
    (void)x;
    (void)y;
    if (power_options_window) {
        gui_destroy_window(power_options_window);
        power_options_window = NULL;
    }
}

void gui_show_power_options(void) {
    // Don't create multiple power options dialogs
    if (power_options_window) {
        gui_focus_window(power_options_window);
        return;
    }
    
    power_options_window = gui_create_window("Power Options", 300, 200, 400, 280);
    if (!power_options_window) return;
    
    power_options_window->bg_color = (color_t){45, 45, 48, 255};
    
    // Header
    gui_create_label(power_options_window, "Power Options", 140, 15);
    gui_create_label(power_options_window, "Choose an action:", 145, 45);
    
    // Power option buttons
    widget_t* shutdown_btn = gui_create_button(power_options_window, "Shut Down", 50, 80, 300, 40);
    if (shutdown_btn) {
        shutdown_btn->bg_color = (color_t){200, 60, 60, 255};  // Red
        gui_set_widget_click_handler(shutdown_btn, power_shutdown_click);
    }
    
    widget_t* restart_btn = gui_create_button(power_options_window, "Restart", 50, 130, 300, 40);
    if (restart_btn) {
        restart_btn->bg_color = (color_t){60, 150, 200, 255};  // Blue
        gui_set_widget_click_handler(restart_btn, power_restart_click);
    }
    
    widget_t* sleep_btn = gui_create_button(power_options_window, "Sleep", 50, 180, 300, 40);
    if (sleep_btn) {
        sleep_btn->bg_color = (color_t){80, 80, 90, 255};  // Gray
        gui_set_widget_click_handler(sleep_btn, power_sleep_click);
    }
    
    widget_t* cancel_btn = gui_create_button(power_options_window, "Cancel", 150, 235, 100, 30);
    if (cancel_btn) {
        cancel_btn->bg_color = (color_t){100, 100, 110, 255};
        gui_set_widget_click_handler(cancel_btn, power_cancel_click);
    }
    
    gui_show_window(power_options_window);
    gui_focus_window(power_options_window);
}
