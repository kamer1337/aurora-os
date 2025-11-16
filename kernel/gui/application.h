/**
 * Aurora OS - Application Framework
 * 
 * Framework for launching and managing GUI applications
 */

#ifndef APPLICATION_H
#define APPLICATION_H

#include <stdint.h>
#include "gui.h"

// Application types
typedef enum {
    APP_FILE_MANAGER,
    APP_TERMINAL,
    APP_SETTINGS,
    APP_SYSTEM_INFO,
    APP_TEXT_EDITOR,
    APP_CALCULATOR,
    APP_DISK_MANAGER,
    APP_MAX
} app_type_t;

// Application structure
typedef struct {
    app_type_t type;
    const char* name;
    const char* description;
    window_t* window;
    uint8_t running;
} application_t;

/**
 * Initialize the application framework
 */
void app_init(void);

/**
 * Launch an application
 * @param type Application type to launch
 * @return 0 on success, -1 on failure
 */
int app_launch(app_type_t type);

/**
 * Close an application
 * @param type Application type to close
 */
void app_close(app_type_t type);

/**
 * Get application by type
 * @param type Application type
 * @return Pointer to application or NULL if not found
 */
application_t* app_get(app_type_t type);

/**
 * Check if an application is running
 * @param type Application type
 * @return 1 if running, 0 otherwise
 */
int app_is_running(app_type_t type);

#endif // APPLICATION_H
