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
    PROCESS_WAITING,     /* Waiting for child process */
    PROCESS_TERMINATED
} process_state_t;

/* Process control block */
typedef struct process {
    uint32_t pid;
    uint32_t ppid;               /* Parent process ID */
    process_state_t state;
    uint32_t* stack_ptr;
    uint32_t priority;
    int32_t exit_status;         /* Exit status when terminated */
    uint32_t wait_target;        /* PID being waited for (0 = any child) */
    struct process* next;
} process_t;

/* Process management functions */
void process_init(void);
process_t* process_create(void (*entry)(void), uint32_t priority);
void process_terminate(uint32_t pid);
void process_yield(void);

/* Process wait/exec functions */
int32_t process_wait(uint32_t pid, int32_t* status);
int32_t process_exec(const char* path, char* const argv[]);

/* Process lookup */
process_t* process_get_current(void);
process_t* process_find_by_pid(uint32_t pid);

/* Scheduler functions */
void scheduler_init(void);
void scheduler_schedule(void);

#endif /* AURORA_PROCESS_H */
