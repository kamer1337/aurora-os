/**
 * Aurora OS - Example Theme Plugins
 * 
 * Demonstrates how to create theme plugins for Aurora OS
 */

#ifndef THEME_PLUGINS_EXAMPLES_H
#define THEME_PLUGINS_EXAMPLES_H

#include "theme_plugin.h"

/**
 * Initialize all example plugins
 * Registers the example plugins with the system
 * @return 0 on success, -1 on failure
 */
int theme_plugins_examples_init(void);

/**
 * Shutdown all example plugins
 */
void theme_plugins_examples_shutdown(void);

/**
 * Get the Neon theme plugin
 * @return Pointer to neon plugin
 */
theme_plugin_t* theme_plugin_get_neon(void);

/**
 * Get the Retro theme plugin
 * @return Pointer to retro plugin
 */
theme_plugin_t* theme_plugin_get_retro(void);

/**
 * Get the Matrix theme plugin
 * @return Pointer to matrix plugin
 */
theme_plugin_t* theme_plugin_get_matrix(void);

#endif // THEME_PLUGINS_EXAMPLES_H
