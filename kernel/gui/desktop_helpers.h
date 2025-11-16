/**
 * Aurora OS - Desktop Helper Functions
 * 
 * Utility functions for desktop operations and UI helpers
 */

#ifndef DESKTOP_HELPERS_H
#define DESKTOP_HELPERS_H

#include <stdint.h>
#include "gui.h"

/**
 * Desktop action types
 */
typedef enum {
    DESKTOP_ACTION_NEW_FOLDER,
    DESKTOP_ACTION_NEW_FILE,
    DESKTOP_ACTION_REFRESH,
    DESKTOP_ACTION_PROPERTIES,
    DESKTOP_ACTION_OPEN_TERMINAL,
    DESKTOP_ACTION_OPEN_FILE_MANAGER,
    DESKTOP_ACTION_OPEN_DISK_MANAGER,
    DESKTOP_ACTION_MAX
} desktop_action_t;

/**
 * Initialize desktop helpers
 */
void desktop_helpers_init(void);

/**
 * Perform a desktop action
 * @param action Action to perform
 * @return 0 on success, -1 on failure
 */
int desktop_perform_action(desktop_action_t action);

/**
 * Format a file size for display
 * @param bytes Size in bytes
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return Pointer to formatted string
 */
const char* desktop_format_size(uint64_t bytes, char* buffer, size_t buffer_size);

/**
 * Format a number for display (without using division)
 * @param num Number to format
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return Pointer to formatted string
 */
const char* desktop_format_number(uint32_t num, char* buffer, size_t buffer_size);

/**
 * Get desktop icon count
 * @return Number of desktop icons
 */
int desktop_get_icon_count(void);

/**
 * Open application from desktop
 * @param app_type Application type to launch
 * @return 0 on success, -1 on failure
 */
int desktop_launch_app(uint8_t app_type);

#endif // DESKTOP_HELPERS_H
