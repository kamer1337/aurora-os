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
    uint32_t last_raw_ticks;    /* Last raw timer tick value */
    uint32_t update_count;      /* Update counter */
    bool initialized;           /* System initialized flag */
} timing_state_t;

static timing_state_t g_timing_state = {0};

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
    
    /* Assume timer is initialized with 1000 Hz (1ms per tick) */
    g_timing_state.timer_frequency = 1000;
    
    g_timing_state.initialized = true;
}

/**
 * Update timing system state
 * Should be called from timer interrupt handler
 */
void timing_system_update(void) {
    if (!g_timing_state.initialized) {
        return;
    }
    
    /* Get current raw ticks */
    uint32_t current_ticks = timer_get_ticks();
    
    /* Calculate elapsed ticks (handle wraparound) */
    uint32_t elapsed;
    if (current_ticks >= g_timing_state.last_raw_ticks) {
        elapsed = current_ticks - g_timing_state.last_raw_ticks;
    } else {
        /* Handle 32-bit wraparound */
        elapsed = (0xFFFFFFFF - g_timing_state.last_raw_ticks) + current_ticks + 1;
    }
    
    /* Convert to milliseconds (assuming 1000 Hz timer) */
    g_timing_state.ticks_ms += elapsed;
    
    /* Convert to microseconds (1ms = 1000us) */
    g_timing_state.ticks_us += (uint64_t)elapsed * 1000;
    
    /* Update last tick value */
    g_timing_state.last_raw_ticks = current_ticks;
    g_timing_state.update_count++;
}

/**
 * Get system ticks since boot (milliseconds)
 */
uint64_t get_system_ticks(void) {
    if (!g_timing_state.initialized) {
        timing_system_init();
    }
    
    /* Get current timer ticks and calculate elapsed time */
    uint32_t current_ticks = timer_get_ticks();
    
    /* Calculate elapsed since last update */
    uint32_t elapsed;
    if (current_ticks >= g_timing_state.last_raw_ticks) {
        elapsed = current_ticks - g_timing_state.last_raw_ticks;
    } else {
        elapsed = (0xFFFFFFFF - g_timing_state.last_raw_ticks) + current_ticks + 1;
    }
    
    /* Return total milliseconds */
    return g_timing_state.ticks_ms + elapsed;
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
    
    /* Get current milliseconds */
    uint64_t ms = get_system_ticks();
    
    /* Convert to microseconds */
    return ms * 1000;
}

/**
 * Sleep for specified milliseconds
 */
void timing_sleep_ms(uint32_t milliseconds) {
    timer_sleep(milliseconds);
}

/**
 * Sleep for specified microseconds
 * For very short durations, uses busy-wait
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
    
    /* For remaining microseconds, busy-wait */
    if (microseconds > 0) {
        uint64_t start = timing_get_microseconds();
        uint64_t target = start + microseconds;
        
        /* Busy-wait (not ideal, but necessary for sub-millisecond precision) */
        while (timing_get_microseconds() < target) {
            /* Spin */
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
