/**
 * Aurora OS - Quick Launch Toolbar
 * 
 * Provides quick access toolbar for frequently used applications
 */

#ifndef QUICK_LAUNCH_H
#define QUICK_LAUNCH_H

#include <stdint.h>
#include "gui.h"
#include "gui_effects.h"
#include "application.h"

#define MAX_QUICK_LAUNCH_ITEMS 10

// Quick launch item structure
typedef struct {
    app_type_t app_type;
    sprite_t* icon;
    char* label;
    uint8_t visible;
    void (*on_click)(void);
} quick_launch_item_t;

/**
 * Initialize the quick launch toolbar
 * @param x X position of toolbar
 * @param y Y position of toolbar
 * @param icon_size Size of icons
 * @return 0 on success, -1 on failure
 */
int quick_launch_init(int32_t x, int32_t y, uint32_t icon_size);

/**
 * Add an item to quick launch
 * @param app_type Application type
 * @param icon Icon sprite (can be NULL for default)
 * @param label Label text
 * @param on_click Click callback
 * @return Item index on success, -1 on failure
 */
int quick_launch_add_item(app_type_t app_type, sprite_t* icon, const char* label, void (*on_click)(void));

/**
 * Remove an item from quick launch
 * @param index Item index
 */
void quick_launch_remove_item(int index);

/**
 * Update quick launch (call every frame)
 * @param mouse_x Mouse X position
 * @param mouse_y Mouse Y position
 * @param mouse_clicked Whether mouse was clicked
 */
void quick_launch_update(int32_t mouse_x, int32_t mouse_y, uint8_t mouse_clicked);

/**
 * Draw the quick launch toolbar
 */
void quick_launch_draw(void);

/**
 * Get item at index
 * @param index Item index
 * @return Pointer to item or NULL if invalid
 */
quick_launch_item_t* quick_launch_get_item(int index);

/**
 * Set toolbar visibility
 * @param visible 1 to show, 0 to hide
 */
void quick_launch_set_visible(uint8_t visible);

/**
 * Check if toolbar is visible
 * @return 1 if visible, 0 otherwise
 */
int quick_launch_is_visible(void);

#endif // QUICK_LAUNCH_H
