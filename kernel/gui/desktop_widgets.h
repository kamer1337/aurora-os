/**
 * Aurora OS - Desktop Widgets and Gadgets
 * 
 * Desktop widget system for placing interactive widgets on the desktop
 */

#ifndef DESKTOP_WIDGETS_H
#define DESKTOP_WIDGETS_H

#include <stdint.h>
#include "gui.h"

#define MAX_DESKTOP_WIDGETS 32

// Widget types
typedef enum {
    DESKTOP_WIDGET_CLOCK,
    DESKTOP_WIDGET_WEATHER,
    DESKTOP_WIDGET_SYSTEM_MONITOR,
    DESKTOP_WIDGET_CALENDAR,
    DESKTOP_WIDGET_NOTES,
    DESKTOP_WIDGET_CUSTOM
} desktop_widget_type_t;

// Desktop widget structure
typedef struct {
    desktop_widget_type_t type;
    char title[64];
    rect_t bounds;
    uint8_t visible;
    uint8_t enabled;
    uint8_t draggable;
    color_t bg_color;
    void* data;  // Widget-specific data
    void (*update)(struct desktop_widget_s* widget);
    void (*render)(struct desktop_widget_s* widget);
    void (*on_click)(struct desktop_widget_s* widget, int32_t x, int32_t y);
} desktop_widget_s;

typedef struct desktop_widget_s desktop_widget_t;

/**
 * Initialize desktop widget system
 * @return 0 on success, -1 on failure
 */
int desktop_widgets_init(void);

/**
 * Shutdown desktop widget system
 */
void desktop_widgets_shutdown(void);

/**
 * Create a desktop widget
 * @param type Widget type
 * @param x X position
 * @param y Y position
 * @param width Width
 * @param height Height
 * @return Pointer to created widget or NULL on failure
 */
desktop_widget_t* desktop_widget_create(desktop_widget_type_t type, int32_t x, int32_t y, 
                                       uint32_t width, uint32_t height);

/**
 * Destroy a desktop widget
 * @param widget Widget to destroy
 */
void desktop_widget_destroy(desktop_widget_t* widget);

/**
 * Update all desktop widgets
 */
void desktop_widgets_update(void);

/**
 * Render all desktop widgets
 */
void desktop_widgets_render(void);

/**
 * Handle click on desktop widgets
 * @param x X coordinate
 * @param y Y coordinate
 * @return 1 if handled, 0 otherwise
 */
int desktop_widgets_handle_click(int32_t x, int32_t y);

/**
 * Show a desktop widget
 * @param widget Widget to show
 */
void desktop_widget_show(desktop_widget_t* widget);

/**
 * Hide a desktop widget
 * @param widget Widget to hide
 */
void desktop_widget_hide(desktop_widget_t* widget);

/**
 * Get all desktop widgets
 * @param out_widgets Array to store widget pointers
 * @param max_widgets Maximum number of widgets to return
 * @return Number of widgets returned
 */
int desktop_widgets_get_all(desktop_widget_t** out_widgets, uint32_t max_widgets);

/**
 * Save desktop widget configuration
 * @return 0 on success, -1 on failure
 */
int desktop_widgets_save_config(void);

/**
 * Load desktop widget configuration
 * @return 0 on success, -1 on failure
 */
int desktop_widgets_load_config(void);

#endif // DESKTOP_WIDGETS_H
