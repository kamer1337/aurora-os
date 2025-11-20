/**
 * Aurora OS - Application Store
 * 
 * GUI application for browsing and installing applications
 */

#ifndef APP_STORE_H
#define APP_STORE_H

#include <stdint.h>
#include "gui.h"
#include "package_manager.h"

// Store categories
typedef enum {
    STORE_CAT_ALL,
    STORE_CAT_PRODUCTIVITY,
    STORE_CAT_DEVELOPMENT,
    STORE_CAT_MULTIMEDIA,
    STORE_CAT_GAMES,
    STORE_CAT_UTILITIES,
    STORE_CAT_INSTALLED
} store_category_t;

/**
 * Launch the application store
 * @return Pointer to created window or NULL on failure
 */
window_t* launch_app_store(void);

/**
 * Refresh package list in store
 */
void app_store_refresh(void);

/**
 * Install package from store
 * @param package_name Package to install
 * @return 0 on success, -1 on failure
 */
int app_store_install(const char* package_name);

/**
 * Uninstall package from store
 * @param package_name Package to uninstall
 * @return 0 on success, -1 on failure
 */
int app_store_uninstall(const char* package_name);

#endif // APP_STORE_H
