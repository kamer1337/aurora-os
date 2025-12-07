/**
 * @file timing_system.c
 * @brief Unified Timing System Implementation
 * 
 * Provides high-precision timing services by wrapping the PIT timer driver
 */

#include "timing_system.h"
#include "../drivers/timer.h"

/* Timing system state */
typedef struct {
    uint64_t ticks_ms;          /* Milliseconds since boot */
    uint64_t ticks_us;          /* Microseconds since boot */
    uint32_t timer_frequency;   /* Timer frequency in Hz */
    uint32_t ms_per_tick;       /* Precomputed milliseconds per tick */
    uint32_t us_per_tick;       /* Precomputed microseconds per tick */
    uint32_t last_raw_ticks;    /* Last raw timer tick value */
    uint32_t update_count;      /* Update counter */
    bool initialized;           /* System initialized flag */
} timing_state_t;

static timing_state_t g_timing_state = {0};

/**
 * Calculate elapsed ticks handling wraparound
 */
static inline uint32_t calculate_elapsed_ticks(uint32_t current, uint32_t last) {
    if (current >= last) {
        return current - last;
    } else {
        /* Handle 32-bit wraparound */
        return (0xFFFFFFFF - last) + current + 1;
    }
}

/**
 * Initialize timing system
 */
void timing_system_init(void) {
    if (g_timing_state.initialized) {
        return;
    }
    
    /* Get current timer state */
    g_timing_state.last_raw_ticks = timer_get_ticks();
    g_timing_state.ticks_ms = 0;
    g_timing_state.ticks_us = 0;
    g_timing_state.update_count = 0;
    
    /* Timer is initialized with 100 Hz (10ms per tick) in kernel.c */
    g_timing_state.timer_frequency = 100;
    
    /* Precompute conversion factors to avoid repeated division */
    g_timing_state.ms_per_tick = 1000 / g_timing_state.timer_frequency;
    g_timing_state.us_per_tick = g_timing_state.ms_per_tick * 1000;
    
    g_timing_state.initialized = true;
}

/**
 * Update timing system state
 * Called from timer interrupt handler on each tick
 */
void timing_system_update(void) {
    if (!g_timing_state.initialized) {
        return;
    }
    
    /* Get current raw ticks */
    uint32_t current_ticks = timer_get_ticks();
    
    /* Calculate elapsed ticks */
    uint32_t elapsed = calculate_elapsed_ticks(current_ticks, g_timing_state.last_raw_ticks);
    
    /* Update milliseconds and microseconds using precomputed factors */
    g_timing_state.ticks_ms += (uint64_t)elapsed * g_timing_state.ms_per_tick;
    g_timing_state.ticks_us += (uint64_t)elapsed * g_timing_state.us_per_tick;
    
    /* Update last tick value */
    g_timing_state.last_raw_ticks = current_ticks;
    g_timing_state.update_count++;
}

/**
 * Get system ticks since boot (milliseconds)
 * This is a read-only getter that doesn't modify state
 */
uint64_t get_system_ticks(void) {
    if (!g_timing_state.initialized) {
        timing_system_init();
    }
    
    /* Get current timer ticks and calculate elapsed time */
    uint32_t current_ticks = timer_get_ticks();
    
    /* Calculate elapsed since last update */
    uint32_t elapsed = calculate_elapsed_ticks(current_ticks, g_timing_state.last_raw_ticks);
    
    /* Convert to milliseconds and return total (read-only, no state modification) */
    return g_timing_state.ticks_ms + ((uint64_t)elapsed * g_timing_state.ms_per_tick);
}

/**
 * Get system time in seconds since boot
 */
uint64_t timing_get_seconds(void) {
    return get_system_ticks() / 1000;
}

/**
 * Get system time in microseconds since boot
 */
uint64_t timing_get_microseconds(void) {
    if (!g_timing_state.initialized) {
        timing_system_init();
    }
    
    /* Get current timer ticks */
    uint32_t current_ticks = timer_get_ticks();
    
    /* Calculate elapsed since last update */
    uint32_t elapsed = calculate_elapsed_ticks(current_ticks, g_timing_state.last_raw_ticks);
    
    /* Convert to microseconds and return total using precomputed factor */
    return g_timing_state.ticks_us + ((uint64_t)elapsed * g_timing_state.us_per_tick);
}

/**
 * Sleep for specified milliseconds
 */
void timing_sleep_ms(uint32_t milliseconds) {
    timer_sleep(milliseconds);
}

/**
 * Sleep for specified microseconds
 * For very short durations, uses optimized busy-wait
 */
void timing_sleep_us(uint32_t microseconds) {
    if (microseconds == 0) {
        return;
    }
    
    /* For durations >= 1ms, use timer_sleep */
    if (microseconds >= 1000) {
        timer_sleep(microseconds / 1000);
        microseconds %= 1000;
    }
    
    /* For remaining microseconds, use optimized busy-wait */
    if (microseconds > 0) {
        /* Cache start time to minimize function calls */
        uint64_t start_us = timing_get_microseconds();
        uint64_t target_us = start_us + microseconds;
        
        /* Busy-wait with CPU pause for efficiency */
        while (timing_get_microseconds() < target_us) {
            /* CPU pause reduces power and improves performance */
            __asm__ __volatile__("pause");
        }
    }
}

/**
 * Get timing statistics
 */
void timing_get_stats(timing_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    if (!g_timing_state.initialized) {
        timing_system_init();
    }
    
    stats->ticks_ms = g_timing_state.ticks_ms;
    stats->ticks_us = g_timing_state.ticks_us;
    stats->timer_frequency = g_timing_state.timer_frequency;
    stats->update_count = g_timing_state.update_count;
}
