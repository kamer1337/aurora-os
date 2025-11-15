/**
 * Aurora OS - Timer Driver Implementation
 * 
 * Programmable Interval Timer (PIT) driver
 */

#include "timer.h"

/* Timer state */
static volatile uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 0;

/**
 * Write byte to port
 */
static inline void outb(uint16_t port, uint8_t value) {
    /* In a real implementation, this would use inline assembly */
    /* For now, this is a stub */
    (void)port;
    (void)value;
}

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
