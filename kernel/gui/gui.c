/**
 * Aurora OS - GUI Framework Implementation
 * 
 * Basic GUI framework providing windowing, widgets, and event handling.
 */

#include "gui.h"
#include "framebuffer.h"
#include "../memory/memory.h"
#include "../drivers/mouse.h"
#include "../drivers/keyboard.h"
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

// Window dragging state
static window_t* dragging_window = NULL;
static int32_t drag_offset_x = 0;
static int32_t drag_offset_y = 0;

int gui_init(void) {
    if (gui_initialized) {
        return 0;
    }
    
    // Initialize framebuffer with default settings
    if (framebuffer_init(0, 0, 0) != 0) {
        return -1;
    }
    
    window_list = NULL;
    focused_window = NULL;
    gui_initialized = 1;
    
    // Clear screen
    framebuffer_clear(COLOR_BLUE);
    
    return 0;
}

void gui_shutdown(void) {
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
    
    // Redraw desktop background
    framebuffer_clear((color_t){30, 130, 200, 255});  // Windows-like blue
    
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
    
    // Draw cursor last (on top of everything)
    gui_draw_cursor();
}

void gui_process_event(event_t* event) {
    if (!gui_initialized || !event) return;
    
    // Process event based on type
    switch (event->type) {
        case EVENT_MOUSE_DOWN:
            // Check if clicked on taskbar
            {
                framebuffer_info_t* fb = framebuffer_get_info();
                if (fb && event->y >= (int32_t)(fb->height - 40)) {
                    // Clicked on taskbar - check window list buttons
                    uint32_t button_x = 95;
                    uint32_t button_width = 150;
                    uint32_t taskbar_y = fb->height - 40;
                    
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
                            
                            // If clicked on titlebar (but not buttons), start dragging
                            clicked_window = window;
                            dragging_window = window;
                            drag_offset_x = event->x - window->bounds.x;
                            drag_offset_y = event->y - window->bounds.y;
                            gui_focus_window(window);
                            break;
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
    
    // Draw window border
    if (window->has_border) {
        framebuffer_draw_rect_outline(window->bounds.x, window->bounds.y, 
                                      window->bounds.width, window->bounds.height,
                                      COLOR_DARK_GRAY);
    }
    
    // Draw titlebar
    if (window->has_titlebar) {
        color_t title_color = window->focused ? 
            (color_t){0, 120, 215, 255} :  // Active window blue
            (color_t){128, 128, 128, 255}; // Inactive gray
            
        framebuffer_draw_rect(window->bounds.x + 1, window->bounds.y + 1,
                             window->bounds.width - 2, title_height,
                             title_color);
        
        // Draw window title
        if (window->title) {
            framebuffer_draw_string(window->bounds.x + 8, window->bounds.y + 8,
                                  window->title, COLOR_WHITE, title_color);
        }
        
        // Draw window control buttons (minimize, maximize, close)
        uint32_t button_size = 16;
        uint32_t button_spacing = 4;
        uint32_t button_y = window->bounds.y + 4;
        
        // Close button (red X)
        uint32_t close_x = window->bounds.x + window->bounds.width - 20;
        framebuffer_draw_rect(close_x, button_y, button_size, button_size, COLOR_RED);
        framebuffer_draw_string(close_x + 4, button_y + 4, "X", COLOR_WHITE, COLOR_RED);
        
        // Maximize button (square)
        uint32_t max_x = close_x - button_size - button_spacing;
        color_t max_color = window->maximized ? (color_t){100, 100, 100, 255} : (color_t){50, 150, 50, 255};
        framebuffer_draw_rect(max_x, button_y, button_size, button_size, max_color);
        framebuffer_draw_rect_outline(max_x + 3, button_y + 3, 10, 10, COLOR_WHITE);
        
        // Minimize button (dash)
        uint32_t min_x = max_x - button_size - button_spacing;
        framebuffer_draw_rect(min_x, button_y, button_size, button_size, (color_t){200, 150, 50, 255});
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
    widget->bounds.width = strlen(text) * 8;
    widget->bounds.height = 8;
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
            // Draw button background
            framebuffer_draw_rect(abs_x, abs_y, widget->bounds.width, widget->bounds.height,
                                widget->bg_color);
            
            // Draw button border
            framebuffer_draw_rect_outline(abs_x, abs_y, widget->bounds.width, widget->bounds.height,
                                        COLOR_BLACK);
            
            // Draw button text (centered)
            if (widget->text) {
                uint32_t text_width = strlen(widget->text) * 8;
                uint32_t text_x = abs_x + (widget->bounds.width - text_width) / 2;
                uint32_t text_y = abs_y + (widget->bounds.height - 8) / 2;
                framebuffer_draw_string(text_x, text_y, widget->text,
                                      widget->fg_color, widget->bg_color);
            }
            break;
            
        case WIDGET_LABEL:
            // Draw label text
            if (widget->text) {
                framebuffer_draw_string(abs_x, abs_y, widget->text,
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

void gui_draw_taskbar(void) {
    framebuffer_info_t* fb = framebuffer_get_info();
    if (!fb) return;
    
    uint32_t taskbar_height = 40;
    uint32_t taskbar_y = fb->height - taskbar_height;
    
    // Draw taskbar background
    framebuffer_draw_rect(0, taskbar_y, fb->width, taskbar_height,
                        (color_t){45, 45, 48, 255});  // Dark gray
    
    // Draw top border
    framebuffer_draw_hline(0, fb->width - 1, taskbar_y, COLOR_GRAY);
    
    // Draw "Start" button
    framebuffer_draw_rect(5, taskbar_y + 5, 80, 30, (color_t){0, 120, 215, 255});
    framebuffer_draw_string(15, taskbar_y + 13, "Aurora OS", COLOR_WHITE,
                          (color_t){0, 120, 215, 255});
    
    // Draw window list (taskbar buttons for open windows)
    uint32_t button_x = 95;
    uint32_t button_width = 150;
    uint32_t button_height = 30;
    
    window_t* window = window_list;
    while (window && button_x + button_width < fb->width - 100) {
        // Show all windows in taskbar (including minimized)
        color_t btn_color;
        if (window->minimized) {
            btn_color = (color_t){40, 40, 45, 255};  // Darker for minimized
        } else if (window->focused) {
            btn_color = (color_t){70, 70, 75, 255};  // Lighter for focused
        } else {
            btn_color = (color_t){55, 55, 60, 255};  // Medium for unfocused
        }
        
        // Draw window button
        framebuffer_draw_rect(button_x, taskbar_y + 5, button_width, button_height, btn_color);
        framebuffer_draw_rect_outline(button_x, taskbar_y + 5, button_width, button_height, COLOR_GRAY);
        
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
                framebuffer_draw_string(button_x + 5, taskbar_y + 13, truncated, COLOR_WHITE, btn_color);
            } else {
                framebuffer_draw_string(button_x + 5, taskbar_y + 13, window->title, COLOR_WHITE, btn_color);
            }
        }
        
        button_x += button_width + 5;
        window = window->next;
    }
    
    // Draw system tray area (placeholder)
    uint32_t tray_x = fb->width - 100;
    framebuffer_draw_string(tray_x, taskbar_y + 13, "12:00 PM", COLOR_WHITE, 
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
                .button = 0,
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
