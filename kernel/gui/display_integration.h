/**
 * Aurora OS - Display Manager Integration Header
 */

#ifndef DISPLAY_INTEGRATION_H
#define DISPLAY_INTEGRATION_H

#include <stdint.h>

/**
 * Initialize display system with display manager
 * @return 0 on success, -1 on failure
 */
int display_system_init(void);

/**
 * Switch display resolution at runtime
 * @param width Desired width
 * @param height Desired height
 * @param refresh_rate Desired refresh rate
 * @return 0 on success, -1 on failure
 */
int display_system_set_resolution(uint32_t width, uint32_t height, uint32_t refresh_rate);

/**
 * Get current display information
 * @param width Output parameter for width
 * @param height Output parameter for height
 * @param bpp Output parameter for bits per pixel
 * @return 0 on success, -1 on failure
 */
int display_system_get_info(uint32_t* width, uint32_t* height, uint32_t* bpp);

/**
 * Enable hardware acceleration if available
 * @return 0 on success, -1 on failure
 */
int display_system_enable_hw_accel(void);

/**
 * Configure dual monitor setup
 * @return 0 on success, -1 on failure
 */
int display_system_setup_dual_monitors(void);

/**
 * List all available resolutions
 */
void display_system_list_resolutions(void);

#endif // DISPLAY_INTEGRATION_H
