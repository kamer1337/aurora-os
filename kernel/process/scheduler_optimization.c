/**
 * Aurora OS - Scheduler Latency Reduction
 * Medium-Term Goal (Q2 2026): Scheduler latency reduction
 * 
 * This module implements advanced scheduling algorithms to reduce
 * latency and improve real-time responsiveness.
 * Includes SCHED_FIFO, SCHED_RR, and SCHED_DEADLINE support.
 */

#include "scheduler_optimization.h"
#include "../memory/memory.h"

/* Maximum tasks for RT scheduling */
#define MAX_RT_TASKS 256

/* RT task storage */
typedef struct {
    uint32_t pid;
    rt_task_params_t params;
    int active;
    int original_priority;  /* For priority inheritance */
} rt_task_entry_t;

static rt_task_entry_t rt_tasks[MAX_RT_TASKS];

/* Scheduler optimization state */
static scheduler_opt_state_t sched_opt = {
    .cfs_enabled = 0,
    .priority_inheritance = 0,
    .load_balancing = 0,
    .realtime_support = 0,
    .avg_latency_us = 1000  /* Current baseline: 1ms */
};

/**
 * Find RT task by PID
 */
static rt_task_entry_t* find_rt_task(uint32_t pid) {
    for (int i = 0; i < MAX_RT_TASKS; i++) {
        if (rt_tasks[i].active && rt_tasks[i].pid == pid) {
            return &rt_tasks[i];
        }
    }
    return NULL;
}

/**
 * Allocate RT task entry
 */
static rt_task_entry_t* alloc_rt_task(void) {
    for (int i = 0; i < MAX_RT_TASKS; i++) {
        if (!rt_tasks[i].active) {
            return &rt_tasks[i];
        }
    }
    return NULL;
}

/**
 * Initialize scheduler optimizations
 * @return 0 on success, -1 on failure
 */
int scheduler_optimization_init(void) {
    /* Initialize RT task array */
    for (int i = 0; i < MAX_RT_TASKS; i++) {
        rt_tasks[i].active = 0;
        rt_tasks[i].pid = 0;
    }
    
    /* Initialize RT runqueues */
    for (int i = 0; i <= MAX_RT_PRIORITY; i++) {
        sched_opt.rt_rq.fifo_queue[i] = NULL;
        sched_opt.rt_rq.rr_queue[i] = NULL;
    }
    sched_opt.rt_rq.deadline_queue = NULL;
    sched_opt.rt_rq.active_count = 0;
    sched_opt.rt_rq.highest_priority_bitmap = 0;
    
    /* Enable Completely Fair Scheduler (CFS) algorithm */
    sched_opt.cfs_enabled = 1;
    
    /* Enable priority inheritance to prevent priority inversion */
    sched_opt.priority_inheritance = 1;
    
    /* Enable load balancing across multiple CPU cores */
    sched_opt.load_balancing = 1;
    
    /* Set initial latency target */
    sched_opt.avg_latency_us = 1000;  /* Target: reduce to <100us */
    
    return 0;
}

/**
 * Enable Completely Fair Scheduler
 */
void scheduler_enable_cfs(void) {
    sched_opt.cfs_enabled = 1;
}

/**
 * Enable priority inheritance
 */
void scheduler_enable_priority_inheritance(void) {
    sched_opt.priority_inheritance = 1;
}

/**
 * Enable load balancing across CPUs
 */
void scheduler_enable_load_balancing(void) {
    sched_opt.load_balancing = 1;
}

/**
 * Enable real-time scheduling support
 */
void scheduler_enable_realtime(void) {
    sched_opt.realtime_support = 1;
}

/**
 * Get average scheduling latency in microseconds
 */
uint32_t scheduler_get_avg_latency_us(void) {
    return sched_opt.avg_latency_us;
}

/**
 * Create a real-time task
 */
int rt_task_create(uint32_t pid, rt_task_params_t* params) {
    if (!params) {
        return -1;
    }
    
    /* Validate parameters */
    if (params->policy == SCHED_FIFO || params->policy == SCHED_RR) {
        if (params->priority < MIN_RT_PRIORITY || params->priority > MAX_RT_PRIORITY) {
            return -1;
        }
    }
    
    /* Find or allocate task entry */
    rt_task_entry_t* task = find_rt_task(pid);
    if (!task) {
        task = alloc_rt_task();
        if (!task) {
            return -1;  /* No free slots */
        }
    }
    
    task->pid = pid;
    task->params = *params;
    task->active = 1;
    task->original_priority = params->priority;
    
    /* Enqueue task */
    rt_enqueue_task(pid, params->policy, params->priority);
    
    return 0;
}

/**
 * Destroy a real-time task
 */
int rt_task_destroy(uint32_t pid) {
    rt_task_entry_t* task = find_rt_task(pid);
    if (!task) {
        return -1;
    }
    
    rt_dequeue_task(pid);
    task->active = 0;
    task->pid = 0;
    
    return 0;
}

/**
 * Set scheduler policy and priority for a task
 */
int rt_set_scheduler(uint32_t pid, int policy, int priority) {
    rt_task_entry_t* task = find_rt_task(pid);
    if (!task) {
        /* Create new RT task */
        rt_task_params_t params = {
            .policy = policy,
            .priority = priority,
            .time_slice_us = (policy == SCHED_RR) ? RT_TIME_QUANTUM_US : 0,
            .deadline = {0, 0, 0}
        };
        return rt_task_create(pid, &params);
    }
    
    /* Update existing task */
    rt_dequeue_task(pid);
    task->params.policy = policy;
    task->params.priority = priority;
    task->original_priority = priority;
    rt_enqueue_task(pid, policy, priority);
    
    return 0;
}

/**
 * Get scheduler policy and priority for a task
 */
int rt_get_scheduler(uint32_t pid, int* policy, int* priority) {
    rt_task_entry_t* task = find_rt_task(pid);
    if (!task || !policy || !priority) {
        return -1;
    }
    
    *policy = task->params.policy;
    *priority = task->params.priority;
    
    return 0;
}

/**
 * Set deadline parameters for SCHED_DEADLINE task
 */
int rt_set_deadline_params(uint32_t pid, sched_deadline_params_t* params) {
    if (!params) {
        return -1;
    }
    
    /* Admission control test */
    if (!deadline_admission_test(params)) {
        return -1;  /* Cannot guarantee deadline */
    }
    
    rt_task_entry_t* task = find_rt_task(pid);
    if (!task) {
        rt_task_params_t task_params = {
            .policy = SCHED_DEADLINE,
            .priority = MAX_RT_PRIORITY,  /* Deadline tasks have highest priority */
            .time_slice_us = 0,
            .deadline = *params
        };
        return rt_task_create(pid, &task_params);
    }
    
    task->params.policy = SCHED_DEADLINE;
    task->params.deadline = *params;
    
    return 0;
}

/**
 * Enqueue task to appropriate runqueue
 */
void rt_enqueue_task(uint32_t pid, int policy, int priority) {
    rt_runqueue_entry_t* entry = (rt_runqueue_entry_t*)kmalloc(sizeof(rt_runqueue_entry_t));
    if (!entry) return;
    
    entry->pid = pid;
    entry->policy = policy;
    entry->priority = priority;
    entry->next = NULL;
    
    rt_task_entry_t* task = find_rt_task(pid);
    if (task && policy == SCHED_DEADLINE) {
        entry->deadline = task->params.deadline.deadline_ns;
        entry->remaining_time = task->params.deadline.runtime_ns;
    }
    
    switch (policy) {
        case SCHED_FIFO:
            /* Add to end of FIFO queue for this priority */
            if (!sched_opt.rt_rq.fifo_queue[priority]) {
                sched_opt.rt_rq.fifo_queue[priority] = entry;
            } else {
                rt_runqueue_entry_t* tail = sched_opt.rt_rq.fifo_queue[priority];
                while (tail->next) tail = tail->next;
                tail->next = entry;
            }
            /* Set priority bitmap */
            sched_opt.rt_rq.highest_priority_bitmap |= (1ULL << priority);
            break;
            
        case SCHED_RR:
            /* Add to end of RR queue for this priority */
            if (!sched_opt.rt_rq.rr_queue[priority]) {
                sched_opt.rt_rq.rr_queue[priority] = entry;
            } else {
                rt_runqueue_entry_t* tail = sched_opt.rt_rq.rr_queue[priority];
                while (tail->next) tail = tail->next;
                tail->next = entry;
            }
            sched_opt.rt_rq.highest_priority_bitmap |= (1ULL << priority);
            break;
            
        case SCHED_DEADLINE:
            /* Insert sorted by deadline (EDF) */
            if (!sched_opt.rt_rq.deadline_queue || 
                entry->deadline < sched_opt.rt_rq.deadline_queue->deadline) {
                entry->next = sched_opt.rt_rq.deadline_queue;
                sched_opt.rt_rq.deadline_queue = entry;
            } else {
                rt_runqueue_entry_t* curr = sched_opt.rt_rq.deadline_queue;
                while (curr->next && curr->next->deadline <= entry->deadline) {
                    curr = curr->next;
                }
                entry->next = curr->next;
                curr->next = entry;
            }
            break;
            
        default:
            kfree(entry);
            return;
    }
    
    sched_opt.rt_rq.active_count++;
}

/**
 * Dequeue task from runqueue
 */
void rt_dequeue_task(uint32_t pid) {
    /* Search FIFO queues */
    for (int p = MAX_RT_PRIORITY; p >= MIN_RT_PRIORITY; p--) {
        rt_runqueue_entry_t** head = &sched_opt.rt_rq.fifo_queue[p];
        rt_runqueue_entry_t* prev = NULL;
        rt_runqueue_entry_t* curr = *head;
        
        while (curr) {
            if (curr->pid == pid) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    *head = curr->next;
                }
                if (!*head) {
                    sched_opt.rt_rq.highest_priority_bitmap &= ~(1ULL << p);
                }
                kfree(curr);
                sched_opt.rt_rq.active_count--;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    
    /* Search RR queues */
    for (int p = MAX_RT_PRIORITY; p >= MIN_RT_PRIORITY; p--) {
        rt_runqueue_entry_t** head = &sched_opt.rt_rq.rr_queue[p];
        rt_runqueue_entry_t* prev = NULL;
        rt_runqueue_entry_t* curr = *head;
        
        while (curr) {
            if (curr->pid == pid) {
                if (prev) {
                    prev->next = curr->next;
                } else {
                    *head = curr->next;
                }
                if (!*head) {
                    sched_opt.rt_rq.highest_priority_bitmap &= ~(1ULL << p);
                }
                kfree(curr);
                sched_opt.rt_rq.active_count--;
                return;
            }
            prev = curr;
            curr = curr->next;
        }
    }
    
    /* Search deadline queue */
    rt_runqueue_entry_t** head = &sched_opt.rt_rq.deadline_queue;
    rt_runqueue_entry_t* prev = NULL;
    rt_runqueue_entry_t* curr = *head;
    
    while (curr) {
        if (curr->pid == pid) {
            if (prev) {
                prev->next = curr->next;
            } else {
                *head = curr->next;
            }
            kfree(curr);
            sched_opt.rt_rq.active_count--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

/**
 * Pick next task to run based on real-time scheduling
 */
uint32_t rt_pick_next_task(void) {
    /* SCHED_DEADLINE has highest priority (EDF) */
    if (sched_opt.rt_rq.deadline_queue) {
        return sched_opt.rt_rq.deadline_queue->pid;
    }
    
    /* Check FIFO and RR queues from highest to lowest priority */
    for (int p = MAX_RT_PRIORITY; p >= MIN_RT_PRIORITY; p--) {
        if (sched_opt.rt_rq.fifo_queue[p]) {
            return sched_opt.rt_rq.fifo_queue[p]->pid;
        }
        if (sched_opt.rt_rq.rr_queue[p]) {
            return sched_opt.rt_rq.rr_queue[p]->pid;
        }
    }
    
    return 0;  /* No RT task ready */
}

/**
 * Handle scheduler tick for real-time tasks
 */
void rt_task_tick(void) {
    if (!sched_opt.realtime_support) return;
    
    /* Handle RR time slice expiration */
    for (int p = MAX_RT_PRIORITY; p >= MIN_RT_PRIORITY; p--) {
        rt_runqueue_entry_t* curr = sched_opt.rt_rq.rr_queue[p];
        if (curr) {
            /* Move current task to end of queue (round-robin) */
            if (curr->next) {
                sched_opt.rt_rq.rr_queue[p] = curr->next;
                rt_runqueue_entry_t* tail = curr->next;
                while (tail->next) tail = tail->next;
                tail->next = curr;
                curr->next = NULL;
            }
            break;
        }
    }
    
    /* Update deadline task remaining time */
    if (sched_opt.rt_rq.deadline_queue) {
        rt_runqueue_entry_t* dl = sched_opt.rt_rq.deadline_queue;
        if (dl->remaining_time > 0) {
            dl->remaining_time -= RT_TIME_QUANTUM_US * 1000;  /* Convert to ns */
        }
    }
}

/**
 * Boost holder's priority to waiter's priority (priority inheritance)
 */
void priority_inherit_boost(uint32_t holder_pid, uint32_t waiter_priority) {
    if (!sched_opt.priority_inheritance) return;
    
    rt_task_entry_t* holder = find_rt_task(holder_pid);
    if (!holder) return;
    
    if ((int)waiter_priority > holder->params.priority) {
        rt_dequeue_task(holder_pid);
        holder->params.priority = (int)waiter_priority;
        rt_enqueue_task(holder_pid, holder->params.policy, holder->params.priority);
    }
}

/**
 * Restore holder's original priority after releasing lock
 */
void priority_inherit_restore(uint32_t holder_pid) {
    if (!sched_opt.priority_inheritance) return;
    
    rt_task_entry_t* holder = find_rt_task(holder_pid);
    if (!holder) return;
    
    if (holder->params.priority != holder->original_priority) {
        rt_dequeue_task(holder_pid);
        holder->params.priority = holder->original_priority;
        rt_enqueue_task(holder_pid, holder->params.policy, holder->params.priority);
    }
}

/**
 * Admission test for SCHED_DEADLINE tasks
 * Uses Liu & Layland bound for schedulability
 */
int deadline_admission_test(sched_deadline_params_t* params) {
    if (!params) return 0;
    
    /* Simple utilization bound check */
    /* U = runtime / period <= 1 for single task */
    if (params->period_ns == 0) return 0;
    if (params->runtime_ns > params->period_ns) return 0;
    if (params->deadline_ns < params->runtime_ns) return 0;
    
    /* Calculate total system utilization */
    uint64_t total_util = 0;
    for (int i = 0; i < MAX_RT_TASKS; i++) {
        if (rt_tasks[i].active && rt_tasks[i].params.policy == SCHED_DEADLINE) {
            if (rt_tasks[i].params.deadline.period_ns > 0) {
                total_util += (rt_tasks[i].params.deadline.runtime_ns * 1000) / 
                              rt_tasks[i].params.deadline.period_ns;
            }
        }
    }
    
    /* Add new task utilization */
    total_util += (params->runtime_ns * 1000) / params->period_ns;
    
    /* Check if total utilization <= 100% (1000 in our scaled representation) */
    return total_util <= 1000;
}

/**
 * Update deadline for periodic task
 */
void deadline_update(uint32_t pid) {
    rt_task_entry_t* task = find_rt_task(pid);
    if (!task || task->params.policy != SCHED_DEADLINE) return;
    
    /* Dequeue and re-enqueue with new deadline */
    rt_dequeue_task(pid);
    
    /* Update absolute deadline for next period */
    /* In a real implementation, this would use current time */
    rt_enqueue_task(pid, SCHED_DEADLINE, MAX_RT_PRIORITY);
}
