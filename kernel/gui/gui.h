/**
 * Aurora OS - GUI Framework
 * 
 * Basic GUI framework providing windowing, widgets, and event handling.
 */

#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include <stddef.h>
#include "framebuffer.h"

// Forward declarations
typedef struct window window_t;
typedef struct widget widget_t;

// Widget types
typedef enum {
    WIDGET_BUTTON,
    WIDGET_LABEL,
    WIDGET_TEXTBOX,
    WIDGET_PANEL,
    WIDGET_MENUBAR,
    WIDGET_MENU,
    WIDGET_CUSTOM
} widget_type_t;

// Event types
typedef enum {
    EVENT_NONE,
    EVENT_MOUSE_MOVE,
    EVENT_MOUSE_DOWN,
    EVENT_MOUSE_UP,
    EVENT_KEY_DOWN,
    EVENT_KEY_UP,
    EVENT_PAINT,
    EVENT_CLOSE
} event_type_t;

// Rectangle structure
typedef struct {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
} rect_t;

// Event structure
typedef struct {
    event_type_t type;
    int32_t x;
    int32_t y;
    uint32_t button;
    uint32_t key;
    void* data;
} event_t;

// Widget structure
struct widget {
    widget_type_t type;
    rect_t bounds;
    color_t bg_color;
    color_t fg_color;
    char* text;
    uint8_t visible;
    uint8_t enabled;
    void (*on_paint)(widget_t* widget);
    void (*on_click)(widget_t* widget, int32_t x, int32_t y);
    window_t* parent_window;
    widget_t* next;
};

// Window structure
struct window {
    char* title;
    rect_t bounds;
    rect_t normal_bounds;  // Saved bounds for restore after maximize
    color_t bg_color;
    color_t title_color;
    uint8_t visible;
    uint8_t focused;
    uint8_t has_border;
    uint8_t has_titlebar;
    uint8_t minimized;
    uint8_t maximized;
    uint8_t transparency;  // Window transparency (0-100, 0=fully transparent, 100=opaque)
    widget_t* widgets;
    window_t* next;
};

/**
 * Initialize the GUI system
 * @return 0 on success, -1 on failure
 */
int gui_init(void);

/**
 * Shutdown the GUI system
 */
void gui_shutdown(void);

/**
 * Update and render all GUI elements
 */
void gui_update(void);

/**
 * Process an event
 * @param event Event to process
 */
void gui_process_event(event_t* event);

/**
 * Create a new window
 * @param title Window title
 * @param x X position
 * @param y Y position
 * @param width Window width
 * @param height Window height
 * @return Pointer to created window or NULL on failure
 */
window_t* gui_create_window(const char* title, int32_t x, int32_t y, uint32_t width, uint32_t height);

/**
 * Destroy a window
 * @param window Window to destroy
 */
void gui_destroy_window(window_t* window);

/**
 * Close a window (alias for gui_destroy_window)
 * @param window Window to close
 */
void gui_close_window(window_t* window);

/**
 * Show a window
 * @param window Window to show
 */
void gui_show_window(window_t* window);

/**
 * Hide a window
 * @param window Window to hide
 */
void gui_hide_window(window_t* window);

/**
 * Set window focus
 * @param window Window to focus
 */
void gui_focus_window(window_t* window);

/**
 * Draw a window and its contents
 * @param window Window to draw
 */
void gui_draw_window(window_t* window);

/**
 * Create a button widget
 * @param window Parent window
 * @param text Button text
 * @param x X position relative to window
 * @param y Y position relative to window
 * @param width Button width
 * @param height Button height
 * @return Pointer to created widget or NULL on failure
 */
widget_t* gui_create_button(window_t* window, const char* text, int32_t x, int32_t y, uint32_t width, uint32_t height);

/**
 * Create a label widget
 * @param window Parent window
 * @param text Label text
 * @param x X position relative to window
 * @param y Y position relative to window
 * @return Pointer to created widget or NULL on failure
 */
widget_t* gui_create_label(window_t* window, const char* text, int32_t x, int32_t y);

/**
 * Create a panel widget
 * @param window Parent window
 * @param x X position relative to window
 * @param y Y position relative to window
 * @param width Panel width
 * @param height Panel height
 * @return Pointer to created widget or NULL on failure
 */
widget_t* gui_create_panel(window_t* window, int32_t x, int32_t y, uint32_t width, uint32_t height);

/**
 * Set widget click handler
 * @param widget Widget to set handler for
 * @param handler Click handler function
 */
void gui_set_widget_click_handler(widget_t* widget, void (*handler)(widget_t*, int32_t, int32_t));

/**
 * Draw a widget
 * @param widget Widget to draw
 * @param window_x Window X offset
 * @param window_y Window Y offset
 */
void gui_draw_widget(widget_t* widget, int32_t window_x, int32_t window_y);

/**
 * Check if point is inside rectangle
 * @param x X coordinate
 * @param y Y coordinate
 * @param rect Rectangle to check
 * @return 1 if inside, 0 otherwise
 */
int gui_point_in_rect(int32_t x, int32_t y, rect_t* rect);

/**
 * Draw a simple taskbar
 */
void gui_draw_taskbar(void);

/**
 * Initialize input handling (mouse and keyboard)
 */
void gui_init_input(void);

/**
 * Process input events and update GUI
 */
void gui_handle_input(void);

/**
 * Draw mouse cursor
 */
void gui_draw_cursor(void);

/**
 * Get cursor position
 */
void gui_get_cursor_pos(int32_t* x, int32_t* y);

/**
 * Minimize a window
 */
void gui_minimize_window(window_t* window);

/**
 * Maximize a window
 */
void gui_maximize_window(window_t* window);

/**
 * Restore a window from minimized/maximized state
 */
void gui_restore_window(window_t* window);

/**
 * Draw desktop wallpaper/background
 */
void gui_draw_desktop(void);

/**
 * Toggle start menu visibility
 */
void gui_toggle_start_menu(void);

/**
 * Show context menu for a window
 * @param window Window to show context menu for
 * @param x X position for menu
 * @param y Y position for menu
 */
void gui_show_context_menu(window_t* window, int32_t x, int32_t y);

/**
 * Hide the context menu if visible
 */
void gui_hide_context_menu(void);

/**
 * Check if start menu is visible
 */
int gui_is_start_menu_visible(void);

/**
 * Get the window list (linked list of all windows)
 * @return Pointer to first window in list, or NULL if none
 */
window_t* gui_get_window_list(void);

/**
 * Set focused window
 * @param window Window to focus
 */
void gui_set_focused_window(window_t* window);

/**
 * Bring window to front (top of Z-order)
 * @param window Window to bring to front
 */
void gui_bring_to_front(window_t* window);

/**
 * Set window transparency
 * @param window Window to set transparency for
 * @param transparency Transparency level (0-100, 0=fully transparent, 100=opaque)
 */
void gui_set_window_transparency(window_t* window, uint8_t transparency);

/**
 * Get window transparency
 * @param window Window to get transparency for
 * @return Transparency level (0-100)
 */
uint8_t gui_get_window_transparency(window_t* window);

/**
 * Show power options dialog
 * Displays shutdown, restart, and sleep options
 */
void gui_show_power_options(void);

#endif // GUI_H
