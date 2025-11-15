/**
 * Aurora OS - Process Management Header
 * 
 * Process and thread management structures and functions
 */

#ifndef AURORA_PROCESS_H
#define AURORA_PROCESS_H

#include <stdint.h>

/* Process states */
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

/* Process control block */
typedef struct process {
    uint32_t pid;
    process_state_t state;
    uint32_t* stack_ptr;
    uint32_t priority;
    struct process* next;
} process_t;

/* Process management functions */
void process_init(void);
process_t* process_create(void (*entry)(void), uint32_t priority);
void process_terminate(uint32_t pid);
void process_yield(void);

/* Scheduler functions */
void scheduler_init(void);
void scheduler_schedule(void);

#endif /* AURORA_PROCESS_H */
