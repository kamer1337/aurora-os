/**
 * @file platform_util.c
 * @brief Platform utility functions implementation
 */

#include "../../include/platform/platform_util.h"

#ifndef AURORA_STANDALONE
#include "../../kernel/drivers/timer.h"

/**
 * Get system timestamp (ticks since boot)
 * Only available in kernel builds
 */
uint32_t platform_get_timestamp(void) {
    return timer_get_ticks();
}
#endif
