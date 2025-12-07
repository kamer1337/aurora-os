/**
 * @file timing_system.h
 * @brief Unified Timing System for Aurora OS
 * 
 * Provides high-precision timing services for the kernel, GUI, and VMs
 */

#ifndef AURORA_TIMING_SYSTEM_H
#define AURORA_TIMING_SYSTEM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Initialize timing system
 * Should be called during kernel initialization after timer_init()
 */
void timing_system_init(void);

/**
 * Get system ticks since boot (milliseconds)
 * Returns 64-bit tick count for high-precision timing
 */
uint64_t get_system_ticks(void);

/**
 * Get system time in seconds since boot
 */
uint64_t timing_get_seconds(void);

/**
 * Get system time in microseconds since boot
 */
uint64_t timing_get_microseconds(void);

/**
 * Sleep for specified milliseconds
 * Wrapper for timer_sleep() with better naming
 */
void timing_sleep_ms(uint32_t milliseconds);

/**
 * Sleep for specified microseconds
 * High-precision sleep (busy-wait for short durations)
 */
void timing_sleep_us(uint32_t microseconds);

/**
 * Update timing system state
 * Called from timer interrupt handler
 */
void timing_system_update(void);

/**
 * Get timing statistics
 */
typedef struct {
    uint64_t ticks_ms;          /* Milliseconds since boot */
    uint64_t ticks_us;          /* Microseconds since boot */
    uint32_t timer_frequency;   /* Timer frequency in Hz */
    uint32_t update_count;      /* Number of timer updates */
} timing_stats_t;

void timing_get_stats(timing_stats_t* stats);

#endif /* AURORA_TIMING_SYSTEM_H */
