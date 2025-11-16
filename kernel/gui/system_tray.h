/**
 * Aurora OS - System Tray and Notification Area
 * 
 * Provides system tray functionality with notification icons and status indicators
 */

#ifndef SYSTEM_TRAY_H
#define SYSTEM_TRAY_H

#include <stdint.h>
#include "framebuffer.h"
#include "gui_effects.h"

// System tray icon types
typedef enum {
    TRAY_ICON_NETWORK,
    TRAY_ICON_VOLUME,
    TRAY_ICON_BATTERY,
    TRAY_ICON_CLOCK,
    TRAY_ICON_NOTIFICATION,
    TRAY_ICON_CUSTOM,
    TRAY_ICON_MAX
} tray_icon_type_t;

// System tray icon structure
typedef struct {
    tray_icon_type_t type;
    sprite_t* icon;
    char* tooltip;
    uint8_t visible;
    void (*on_click)(void);
} tray_icon_t;

// Notification structure
typedef struct {
    char* title;
    char* message;
    uint32_t duration;  // Duration in milliseconds
    uint32_t start_time;
    uint8_t active;
    color_t color;
} notification_t;

/**
 * Initialize the system tray
 * @param x X position of tray
 * @param y Y position of tray
 * @param width Width of tray
 * @return 0 on success, -1 on failure
 */
int system_tray_init(int32_t x, int32_t y, uint32_t width);

/**
 * Add an icon to the system tray
 * @param type Icon type
 * @param icon Sprite icon (16x16 recommended)
 * @param tooltip Tooltip text
 * @param on_click Click callback
 * @return Icon index on success, -1 on failure
 */
int system_tray_add_icon(tray_icon_type_t type, sprite_t* icon, const char* tooltip, void (*on_click)(void));

/**
 * Remove an icon from the system tray
 * @param index Icon index
 */
void system_tray_remove_icon(int index);

/**
 * Update system tray (call every frame)
 * @param mouse_x Mouse X position
 * @param mouse_y Mouse Y position
 * @param mouse_clicked Whether mouse was clicked
 */
void system_tray_update(int32_t mouse_x, int32_t mouse_y, uint8_t mouse_clicked);

/**
 * Draw the system tray
 */
void system_tray_draw(void);

/**
 * Show a notification
 * @param title Notification title
 * @param message Notification message
 * @param duration Duration in milliseconds (0 = default 3000ms)
 * @param color Notification background color
 * @return Notification ID on success, -1 on failure
 */
int system_tray_notify(const char* title, const char* message, uint32_t duration, color_t color);

/**
 * Update notifications (call every frame)
 * @param current_time Current time in milliseconds
 */
void system_tray_update_notifications(uint32_t current_time);

/**
 * Draw active notifications
 */
void system_tray_draw_notifications(void);

/**
 * Set system time (for clock display)
 * @param hours Hours (0-23)
 * @param minutes Minutes (0-59)
 */
void system_tray_set_time(uint8_t hours, uint8_t minutes);

/**
 * Set network status
 * @param connected 1 if connected, 0 otherwise
 * @param strength Signal strength (0-100)
 */
void system_tray_set_network_status(uint8_t connected, uint8_t strength);

/**
 * Set volume level
 * @param level Volume level (0-100)
 * @param muted 1 if muted, 0 otherwise
 */
void system_tray_set_volume(uint8_t level, uint8_t muted);

/**
 * Set battery status
 * @param level Battery level (0-100)
 * @param charging 1 if charging, 0 otherwise
 */
void system_tray_set_battery(uint8_t level, uint8_t charging);

#endif // SYSTEM_TRAY_H
