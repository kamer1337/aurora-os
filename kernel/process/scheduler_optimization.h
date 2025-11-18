/**
 * Aurora OS - Scheduler Latency Reduction Header
 */

#ifndef SCHEDULER_OPTIMIZATION_H
#define SCHEDULER_OPTIMIZATION_H

#include <stdint.h>

typedef struct {
    int cfs_enabled;
    int priority_inheritance;
    int load_balancing;
    int realtime_support;
    uint32_t avg_latency_us;
} scheduler_opt_state_t;

// Function prototypes
int scheduler_optimization_init(void);
void scheduler_enable_cfs(void);
void scheduler_enable_priority_inheritance(void);
void scheduler_enable_load_balancing(void);
void scheduler_enable_realtime(void);
uint32_t scheduler_get_avg_latency_us(void);

#endif // SCHEDULER_OPTIMIZATION_H
