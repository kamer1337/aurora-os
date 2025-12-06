/**
 * @file notification_system.h
 * @brief Modern Notification System for Aurora OS
 * 
 * Provides desktop notifications with animations, icons, and actions
 */

#ifndef NOTIFICATION_SYSTEM_H
#define NOTIFICATION_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include "gui.h"

/* Notification types */
typedef enum {
    NOTIFICATION_INFO,
    NOTIFICATION_WARNING,
    NOTIFICATION_ERROR,
    NOTIFICATION_SUCCESS
} notification_type_t;

/* Notification priority */
typedef enum {
    NOTIFICATION_PRIORITY_LOW,
    NOTIFICATION_PRIORITY_NORMAL,
    NOTIFICATION_PRIORITY_HIGH,
    NOTIFICATION_PRIORITY_URGENT
} notification_priority_t;

/* Notification action callback */
typedef void (*notification_action_t)(void* user_data);

/* Notification structure */
typedef struct notification {
    uint32_t id;
    char title[128];
    char message[512];
    notification_type_t type;
    notification_priority_t priority;
    uint32_t icon;              /* Icon ID or color */
    uint32_t timeout_ms;        /* Auto-dismiss timeout (0 = no timeout) */
    uint64_t created_time;
    uint64_t expire_time;
    
    /* Actions */
    char action_text[64];
    notification_action_t action_callback;
    void* action_user_data;
    
    /* Visual state */
    int32_t x, y;
    uint32_t width, height;
    float opacity;              /* 0.0 to 1.0 */
    float slide_offset;         /* Animation offset */
    bool visible;
    bool animating_in;
    bool animating_out;
    
    struct notification* next;
} notification_t;

/* Notification system state */
typedef struct {
    notification_t* notifications;
    uint32_t notification_count;
    uint32_t next_id;
    
    /* Display settings */
    int32_t notification_x;     /* Right edge position */
    int32_t notification_y;     /* Top starting position */
    uint32_t notification_width;
    uint32_t notification_spacing;
    uint32_t max_visible;
    
    /* Timing */
    uint32_t default_timeout_ms;
    uint32_t animation_duration_ms;
} notification_system_t;

/**
 * Initialize the notification system
 * @return 0 on success, -1 on failure
 */
int notification_system_init(void);

/**
 * Shutdown the notification system
 */
void notification_system_shutdown(void);

/**
 * Create and show a notification
 * @param title Notification title
 * @param message Notification message
 * @param type Notification type
 * @param priority Notification priority
 * @param timeout_ms Auto-dismiss timeout in milliseconds (0 = no timeout)
 * @return Notification ID, or 0 on failure
 */
uint32_t notification_show(
    const char* title,
    const char* message,
    notification_type_t type,
    notification_priority_t priority,
    uint32_t timeout_ms
);

/**
 * Create a notification with an action button
 * @param title Notification title
 * @param message Notification message
 * @param type Notification type
 * @param action_text Text for action button
 * @param action_callback Callback function for action
 * @param user_data User data passed to callback
 * @return Notification ID, or 0 on failure
 */
uint32_t notification_show_with_action(
    const char* title,
    const char* message,
    notification_type_t type,
    const char* action_text,
    notification_action_t action_callback,
    void* user_data
);

/**
 * Dismiss a notification by ID
 * @param id Notification ID
 */
void notification_dismiss(uint32_t id);

/**
 * Dismiss all notifications
 */
void notification_dismiss_all(void);

/**
 * Update and render all notifications
 * @param delta_ms Time elapsed since last update in milliseconds
 */
void notification_system_update(uint32_t delta_ms);

/**
 * Render all visible notifications
 */
void notification_system_render(void);

/**
 * Handle mouse click on notifications
 * @param x Mouse X coordinate
 * @param y Mouse Y coordinate
 * @return true if a notification was clicked
 */
bool notification_system_handle_click(int32_t x, int32_t y);

/* Convenience functions for common notification types */

/**
 * Show an info notification
 */
static inline uint32_t notification_info(const char* title, const char* message) {
    return notification_show(title, message, NOTIFICATION_INFO, 
                           NOTIFICATION_PRIORITY_NORMAL, 5000);
}

/**
 * Show a warning notification
 */
static inline uint32_t notification_warning(const char* title, const char* message) {
    return notification_show(title, message, NOTIFICATION_WARNING,
                           NOTIFICATION_PRIORITY_NORMAL, 8000);
}

/**
 * Show an error notification
 */
static inline uint32_t notification_error(const char* title, const char* message) {
    return notification_show(title, message, NOTIFICATION_ERROR,
                           NOTIFICATION_PRIORITY_HIGH, 10000);
}

/**
 * Show a success notification
 */
static inline uint32_t notification_success(const char* title, const char* message) {
    return notification_show(title, message, NOTIFICATION_SUCCESS,
                           NOTIFICATION_PRIORITY_NORMAL, 4000);
}

#endif /* NOTIFICATION_SYSTEM_H */
