/**
 * Aurora OS - Timer Driver Implementation
 * 
 * Programmable Interval Timer (PIT) driver
 */

#include "timer.h"
#include "../core/port_io.h"

/* Timer state */
static volatile uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

/**
 * Initialize timer
 */
void timer_init(uint32_t frequency) {
    /* Save frequency */
    timer_frequency = frequency;
    
    /* Calculate divisor */
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    /* Send command byte: Channel 0, Access mode lobyte/hibyte, Rate generator */
    outb(PIT_COMMAND_PORT, 0x36);
    
    /* Send divisor */
    outb(PIT_CHANNEL0_PORT, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0_PORT, (uint8_t)((divisor >> 8) & 0xFF));
    
    /* Reset tick counter */
    timer_ticks = 0;
}

/**
 * Timer interrupt handler
 */
void timer_handler(void) {
    timer_ticks++;
}

/**
 * Get current tick count
 */
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

/**
 * Wait for specified number of ticks
 */
void timer_wait(uint32_t ticks) {
    uint32_t start_ticks = timer_ticks;
    while (timer_ticks - start_ticks < ticks) {
        /* Wait */
    }
}

/**
 * Sleep for specified milliseconds
 */
void timer_sleep(uint32_t milliseconds) {
    if (timer_frequency == 0) {
        return;
    }
    
    /* Calculate ticks needed */
    uint32_t ticks = (milliseconds * timer_frequency) / 1000;
    timer_wait(ticks);
}

/**
 * Get seconds since boot (wrapped to 0-59)
 */
uint32_t timer_get_seconds(void) {
    if (timer_frequency == 0) {
        return 0;
    }
    return (timer_ticks / timer_frequency) % 60;
}

/**
 * Get minutes since boot (wrapped to 0-59)
 */
uint32_t timer_get_minutes(void) {
    if (timer_frequency == 0) {
        return 0;
    }
    return (timer_ticks / (timer_frequency * 60)) % 60;
}

/**
 * Get hours since boot (wrapped to 0-23 in 12-hour format with offset)
 */
uint32_t timer_get_hours(void) {
    if (timer_frequency == 0) {
        return 12;  /* Default to 12:00 */
    }
    /* Start at 12:00 and wrap at 24 hours */
    return (12 + (timer_ticks / (timer_frequency * 3600))) % 24;
}

/**
 * Get time as formatted string (HH:MM AM/PM)
 */
void timer_get_time_string(char* buffer, uint32_t buffer_size) {
    if (!buffer || buffer_size < 9) {
        return;
    }
    
    uint32_t hours = timer_get_hours();
    uint32_t minutes = timer_get_minutes();
    
    /* Convert to 12-hour format */
    const char* period = (hours >= 12) ? "PM" : "AM";
    uint32_t display_hours = hours % 12;
    if (display_hours == 0) {
        display_hours = 12;
    }
    
    /* Format: "HH:MM AM" */
    buffer[0] = '0' + (display_hours / 10);
    buffer[1] = '0' + (display_hours % 10);
    buffer[2] = ':';
    buffer[3] = '0' + (minutes / 10);
    buffer[4] = '0' + (minutes % 10);
    buffer[5] = ' ';
    buffer[6] = period[0];
    buffer[7] = period[1];
    buffer[8] = '\0';
}
