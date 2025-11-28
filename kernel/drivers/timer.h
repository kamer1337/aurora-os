/**
 * Aurora OS - Timer Driver Header
 * 
 * Programmable Interval Timer (PIT) driver
 */

#ifndef AURORA_TIMER_H
#define AURORA_TIMER_H

#include <stdint.h>

/* PIT ports */
#define PIT_CHANNEL0_PORT 0x40
#define PIT_CHANNEL1_PORT 0x41
#define PIT_CHANNEL2_PORT 0x42
#define PIT_COMMAND_PORT  0x43

/* PIT frequency */
#define PIT_FREQUENCY 1193182

/* Timer driver functions */
void timer_init(uint32_t frequency);
void timer_handler(void);
uint32_t timer_get_ticks(void);
void timer_wait(uint32_t ticks);
void timer_sleep(uint32_t milliseconds);

/* Time tracking functions */
uint32_t timer_get_seconds(void);
uint32_t timer_get_minutes(void);
uint32_t timer_get_hours(void);
void timer_get_time_string(char* buffer, uint32_t buffer_size);

#endif /* AURORA_TIMER_H */
