/**
 * Aurora OS - Process Management Implementation
 * 
 * Process creation, scheduling, and management
 */

#include "process.h"
#include "../memory/memory.h"
#include <stddef.h>

/* Process table */
#define MAX_PROCESSES 64
#define PROCESS_STACK_SIZE 4096

static process_t process_table[MAX_PROCESSES];
static process_t* current_process = NULL;
static process_t* ready_queue_head = NULL;
static process_t* ready_queue_tail = NULL;
static uint32_t next_pid = 1;
static uint32_t scheduler_enabled = 0;

/**
 * Find free process slot
 */
static process_t* alloc_process(void) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_TERMINATED || process_table[i].pid == 0) {
            return &process_table[i];
        }
    }
    return NULL;
}

/**
 * Add process to ready queue
 */
static void enqueue_process(process_t* process) {
    if (!process) {
        return;
    }
    
    process->next = NULL;
    process->state = PROCESS_READY;
    
    if (!ready_queue_head) {
        ready_queue_head = process;
        ready_queue_tail = process;
    } else {
        ready_queue_tail->next = process;
        ready_queue_tail = process;
    }
}

/**
 * Remove process from ready queue
 */
static process_t* dequeue_process(void) {
    if (!ready_queue_head) {
        return NULL;
    }
    
    process_t* process = ready_queue_head;
    ready_queue_head = process->next;
    
    if (!ready_queue_head) {
        ready_queue_tail = NULL;
    }
    
    process->next = NULL;
    return process;
}

/**
 * Idle process - runs when no other process is ready
 */
static void idle_process(void) {
    while (1) {
        /* Halt CPU until next interrupt */
        __asm__ volatile("hlt");
    }
}

/**
 * Initialize process management subsystem
 */
void process_init(void) {
    /* Initialize process table */
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_TERMINATED;
        process_table[i].stack_ptr = NULL;
        process_table[i].priority = 0;
        process_table[i].next = NULL;
    }
    
    current_process = NULL;
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    next_pid = 1;
    
    /* Create idle process */
    process_t* idle = process_create(idle_process, 0);
    if (idle) {
        idle->state = PROCESS_READY;
        current_process = idle;
    }
}

/**
 * Create a new process
 */
process_t* process_create(void (*entry)(void), uint32_t priority) {
    if (!entry) {
        return NULL;
    }
    
    /* Allocate process control block */
    process_t* process = alloc_process();
    if (!process) {
        return NULL;
    }
    
    /* Allocate process stack */
    void* stack = kmalloc(PROCESS_STACK_SIZE);
    if (!stack) {
        process->pid = 0;
        return NULL;
    }
    
    /* Initialize process control block */
    process->pid = next_pid++;
    process->state = PROCESS_READY;
    process->priority = priority;
    process->next = NULL;
    
    /* Setup stack pointer (stack grows downward) */
    uint32_t* stack_top = (uint32_t*)((uint8_t*)stack + PROCESS_STACK_SIZE);
    
    /* Push initial context onto stack */
    stack_top--;
    *stack_top = (uint32_t)entry; /* Entry point */
    
    process->stack_ptr = (uint32_t*)stack_top;
    
    /* Add to ready queue */
    enqueue_process(process);
    
    return process;
}

/**
 * Terminate a process
 */
void process_terminate(uint32_t pid) {
    /* Find process by PID */
    process_t* process = NULL;
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            process = &process_table[i];
            break;
        }
    }
    
    if (!process) {
        return;
    }
    
    /* Free process resources */
    if (process->stack_ptr) {
        /* Calculate stack base from stack pointer */
        uint32_t* stack_base = (uint32_t*)((uint32_t)process->stack_ptr & ~(PROCESS_STACK_SIZE - 1));
        kfree(stack_base);
    }
    
    /* Mark process as terminated */
    process->state = PROCESS_TERMINATED;
    process->pid = 0;
    process->stack_ptr = NULL;
    process->next = NULL;
    
    /* If this was the current process, schedule next */
    if (current_process == process) {
        current_process = NULL;
        scheduler_schedule();
    }
}

/**
 * Yield CPU to another process
 */
void process_yield(void) {
    if (!scheduler_enabled) {
        return;
    }
    
    /* Save current process state and schedule next */
    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
        enqueue_process(current_process);
    }
    
    scheduler_schedule();
}

/**
 * Initialize scheduler
 */
void scheduler_init(void) {
    scheduler_enabled = 1;
}

/**
 * Context switch helper (simplified - no real context saving)
 */
static void switch_context(process_t* from, process_t* to) {
    (void)from; /* In real implementation, would save context here */
    
    if (to) {
        current_process = to;
        to->state = PROCESS_RUNNING;
        /* In real implementation, would restore context and jump to process */
    }
}

/**
 * Schedule next process to run (Round-robin scheduling)
 */
void scheduler_schedule(void) {
    if (!scheduler_enabled) {
        return;
    }
    
    /* Get next process from ready queue */
    process_t* next = dequeue_process();
    
    if (!next) {
        /* No process ready, keep current or idle */
        if (current_process && current_process->state == PROCESS_RUNNING) {
            return;
        }
        /* Find idle process (PID 1) */
        for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
            if (process_table[i].pid == 1) {
                next = &process_table[i];
                break;
            }
        }
    }
    
    if (next) {
        process_t* old = current_process;
        switch_context(old, next);
    }
}
