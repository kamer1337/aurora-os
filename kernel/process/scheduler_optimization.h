/**
 * Aurora OS - Scheduler Latency Reduction Header
 */

#ifndef SCHEDULER_OPTIMIZATION_H
#define SCHEDULER_OPTIMIZATION_H

#include <stdint.h>

/* Scheduling policies */
#define SCHED_OTHER     0   /* Standard time-sharing */
#define SCHED_FIFO      1   /* First-in first-out real-time */
#define SCHED_RR        2   /* Round-robin real-time */
#define SCHED_BATCH     3   /* Batch processing */
#define SCHED_IDLE      5   /* Idle priority */
#define SCHED_DEADLINE  6   /* Deadline-based real-time */

/* Priority ranges */
#define MIN_NICE_PRIORITY   -20
#define MAX_NICE_PRIORITY    19
#define MIN_RT_PRIORITY       1
#define MAX_RT_PRIORITY      99
#define DEFAULT_PRIORITY     50

/* Time quantum (in microseconds) */
#define DEFAULT_TIME_QUANTUM_US  10000  /* 10ms */
#define RT_TIME_QUANTUM_US       1000   /* 1ms for RT tasks */

/* Deadline scheduling parameters */
typedef struct {
    uint64_t runtime_ns;     /* Execution time requirement */
    uint64_t deadline_ns;    /* Relative deadline */
    uint64_t period_ns;      /* Period for periodic tasks */
} sched_deadline_params_t;

/* Real-time task parameters */
typedef struct {
    int policy;              /* Scheduling policy */
    int priority;            /* Priority (1-99 for RT, -20 to 19 nice for OTHER) */
    uint32_t time_slice_us;  /* Time slice in microseconds */
    sched_deadline_params_t deadline;  /* For SCHED_DEADLINE */
} rt_task_params_t;

/* Run queue entry for real-time scheduling */
typedef struct rt_runqueue_entry {
    uint32_t pid;
    int policy;
    int priority;
    uint64_t deadline;       /* Absolute deadline for EDF */
    uint64_t remaining_time; /* Remaining execution time */
    struct rt_runqueue_entry* next;
} rt_runqueue_entry_t;

/* Real-time runqueue */
typedef struct {
    rt_runqueue_entry_t* fifo_queue[MAX_RT_PRIORITY + 1];  /* Per-priority FIFO queues */
    rt_runqueue_entry_t* rr_queue[MAX_RT_PRIORITY + 1];    /* Per-priority RR queues */
    rt_runqueue_entry_t* deadline_queue;                    /* EDF queue (sorted by deadline) */
    uint32_t active_count;
    uint64_t highest_priority_bitmap;  /* Bitmap for quick priority lookup */
} rt_runqueue_t;

typedef struct {
    int cfs_enabled;
    int priority_inheritance;
    int load_balancing;
    int realtime_support;
    uint32_t avg_latency_us;
    rt_runqueue_t rt_rq;     /* Real-time runqueue */
} scheduler_opt_state_t;

/* Function prototypes */
int scheduler_optimization_init(void);
void scheduler_enable_cfs(void);
void scheduler_enable_priority_inheritance(void);
void scheduler_enable_load_balancing(void);
void scheduler_enable_realtime(void);
uint32_t scheduler_get_avg_latency_us(void);

/* Real-time scheduling functions */
int rt_task_create(uint32_t pid, rt_task_params_t* params);
int rt_task_destroy(uint32_t pid);
int rt_set_scheduler(uint32_t pid, int policy, int priority);
int rt_get_scheduler(uint32_t pid, int* policy, int* priority);
int rt_set_deadline_params(uint32_t pid, sched_deadline_params_t* params);

/* Scheduling policy functions */
uint32_t rt_pick_next_task(void);
void rt_task_tick(void);
void rt_enqueue_task(uint32_t pid, int policy, int priority);
void rt_dequeue_task(uint32_t pid);

/* Priority inheritance */
void priority_inherit_boost(uint32_t holder_pid, uint32_t waiter_priority);
void priority_inherit_restore(uint32_t holder_pid);

/* Deadline scheduling helpers */
int deadline_admission_test(sched_deadline_params_t* params);
void deadline_update(uint32_t pid);

#endif // SCHEDULER_OPTIMIZATION_H
