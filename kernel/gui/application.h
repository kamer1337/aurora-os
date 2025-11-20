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
    APP_PAINT_EDITOR,
    APP_IMAGE_VIEWER,
    APP_NOTEBOOK,
    APP_MY_PC,
    APP_RECYCLE_BIN,
    APP_UNINSTALLER,
    APP_TASK_MANAGER,
    APP_CONTROL_PANEL,
    APP_HELP,
    APP_WEB_BROWSER,
    APP_FILE_EXPLORER,
    APP_CLOCK,          // New: Clock and Calendar
    APP_MUSIC_PLAYER,   // New: Music Player
    APP_VIDEO_PLAYER,   // New: Video Player
    APP_GOALS_MANAGER,  // New: Long-Term Goals Manager
    APP_SYSTEM_SETTINGS,// New: System Settings
    APP_APP_STORE,      // New: Application Store
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
