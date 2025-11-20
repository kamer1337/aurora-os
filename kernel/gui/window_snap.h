/**
 * Aurora OS - Window Snapping and Tiling
 * 
 * Window snapping to screen edges and tiling features
 */

#ifndef WINDOW_SNAP_H
#define WINDOW_SNAP_H

#include <stdint.h>
#include "gui.h"

// Snap positions
typedef enum {
    SNAP_NONE,
    SNAP_LEFT,
    SNAP_RIGHT,
    SNAP_TOP,
    SNAP_BOTTOM,
    SNAP_TOP_LEFT,
    SNAP_TOP_RIGHT,
    SNAP_BOTTOM_LEFT,
    SNAP_BOTTOM_RIGHT,
    SNAP_MAXIMIZE
} snap_position_t;

// Snap configuration
typedef struct {
    uint8_t enabled;
    uint32_t snap_threshold;  // Pixels from edge to trigger snap
    uint8_t show_preview;     // Show preview when dragging
} snap_config_t;

/**
 * Initialize window snapping system
 * @return 0 on success, -1 on failure
 */
int window_snap_init(void);

/**
 * Check if window should snap and return snap position
 * @param window Window being moved
 * @param x Current X position
 * @param y Current Y position
 * @return Snap position
 */
snap_position_t window_snap_check(window_t* window, int32_t x, int32_t y);

/**
 * Apply snap to window
 * @param window Window to snap
 * @param position Snap position
 * @return 0 on success, -1 on failure
 */
int window_snap_apply(window_t* window, snap_position_t position);

/**
 * Show snap preview
 * @param position Snap position to preview
 */
void window_snap_show_preview(snap_position_t position);

/**
 * Hide snap preview
 */
void window_snap_hide_preview(void);

/**
 * Get snap configuration
 * @return Pointer to snap configuration
 */
snap_config_t* window_snap_get_config(void);

/**
 * Handle keyboard shortcut for window snapping
 * @param window Window to snap
 * @param key Key code
 * @return 1 if handled, 0 otherwise
 */
int window_snap_handle_shortcut(window_t* window, uint32_t key);

#endif // WINDOW_SNAP_H
