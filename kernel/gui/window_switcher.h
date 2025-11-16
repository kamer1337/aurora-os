/**
 * Aurora OS - Window Switcher (Alt-Tab)
 * 
 * Provides Alt-Tab window switching functionality with visual preview
 */

#ifndef WINDOW_SWITCHER_H
#define WINDOW_SWITCHER_H

#include <stdint.h>
#include "gui.h"

/**
 * Initialize the window switcher
 */
void window_switcher_init(void);

/**
 * Show the window switcher overlay
 */
void window_switcher_show(void);

/**
 * Hide the window switcher overlay
 */
void window_switcher_hide(void);

/**
 * Check if window switcher is visible
 * @return 1 if visible, 0 otherwise
 */
int window_switcher_is_visible(void);

/**
 * Move selection to next window
 */
void window_switcher_next(void);

/**
 * Move selection to previous window
 */
void window_switcher_prev(void);

/**
 * Activate the currently selected window
 */
void window_switcher_activate(void);

/**
 * Update window switcher (call every frame when visible)
 * @param delta_time Time since last update in milliseconds
 */
void window_switcher_update(uint32_t delta_time);

/**
 * Draw the window switcher overlay
 */
void window_switcher_draw(void);

/**
 * Handle keyboard input for window switcher
 * @param key Key code
 * @param pressed 1 if pressed, 0 if released
 * @return 1 if input was handled, 0 otherwise
 */
int window_switcher_handle_key(uint32_t key, uint8_t pressed);

#endif // WINDOW_SWITCHER_H
