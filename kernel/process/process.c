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
        process_table[i].ppid = 0;
        process_table[i].state = PROCESS_TERMINATED;
        process_table[i].stack_ptr = NULL;
        process_table[i].priority = 0;
        process_table[i].exit_status = 0;
        process_table[i].wait_target = 0;
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
    process->ppid = current_process ? current_process->pid : 0;
    process->state = PROCESS_READY;
    process->priority = priority;
    process->exit_status = 0;
    process->wait_target = 0;
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
 * CPU context structure for context switching
 */
typedef struct {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;
} cpu_context_t;

/* Context storage for each process */
static cpu_context_t process_contexts[MAX_PROCESSES];

/**
 * Get process index in table
 */
static int get_process_index(process_t* process) {
    if (!process) return -1;
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (&process_table[i] == process) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * Save current CPU context to process
 */
static void save_context(process_t* process) {
    if (!process) return;
    
    int idx = get_process_index(process);
    if (idx < 0) return;
    
    cpu_context_t* ctx = &process_contexts[idx];
    
    /* Save general purpose registers one at a time */
    __asm__ volatile("movl %%eax, %0" : "=m"(ctx->eax));
    __asm__ volatile("movl %%ebx, %0" : "=m"(ctx->ebx));
    __asm__ volatile("movl %%ecx, %0" : "=m"(ctx->ecx));
    __asm__ volatile("movl %%edx, %0" : "=m"(ctx->edx));
    __asm__ volatile("movl %%esi, %0" : "=m"(ctx->esi));
    __asm__ volatile("movl %%edi, %0" : "=m"(ctx->edi));
    __asm__ volatile("movl %%ebp, %0" : "=m"(ctx->ebp));
    __asm__ volatile("movl %%esp, %0" : "=m"(ctx->esp));
    
    /* Save flags */
    __asm__ volatile("pushfl; popl %0" : "=m"(ctx->eflags));
}

/**
 * Restore CPU context from process
 */
static void restore_context(process_t* process) {
    if (!process) return;
    
    int idx = get_process_index(process);
    if (idx < 0) return;
    
    cpu_context_t* ctx = &process_contexts[idx];
    
    /* Restore flags */
    __asm__ volatile("pushl %0; popfl" : : "m"(ctx->eflags));
    
    /* Restore general purpose registers one at a time */
    __asm__ volatile("movl %0, %%edi" : : "m"(ctx->edi));
    __asm__ volatile("movl %0, %%esi" : : "m"(ctx->esi));
    __asm__ volatile("movl %0, %%edx" : : "m"(ctx->edx));
    __asm__ volatile("movl %0, %%ecx" : : "m"(ctx->ecx));
    __asm__ volatile("movl %0, %%ebx" : : "m"(ctx->ebx));
    __asm__ volatile("movl %0, %%ebp" : : "m"(ctx->ebp));
    __asm__ volatile("movl %0, %%eax" : : "m"(ctx->eax));
}

/**
 * Context switch helper - saves old context and restores new context
 */
static void switch_context(process_t* from, process_t* to) {
    if (!to) {
        return;
    }
    
    /* Save context of current process */
    if (from && from->state != PROCESS_TERMINATED) {
        save_context(from);
        /* Store current stack pointer in process control block */
        __asm__ volatile("movl %%esp, %0" : "=m"(from->stack_ptr));
    }
    
    /* Switch to new process */
    current_process = to;
    to->state = PROCESS_RUNNING;
    
    /* Restore stack pointer from process control block */
    if (to->stack_ptr) {
        __asm__ volatile("movl %0, %%esp" : : "m"(to->stack_ptr));
        
        /* Restore context of previously-run process */
        /* For new processes, the context was initialized at creation time */
        int idx = get_process_index(to);
        if (idx >= 0 && process_contexts[idx].esp != 0) {
            restore_context(to);
        }
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

/**
 * Get current running process
 */
process_t* process_get_current(void) {
    return current_process;
}

/**
 * Find process by PID
 */
process_t* process_find_by_pid(uint32_t pid) {
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid) {
            return &process_table[i];
        }
    }
    return NULL;
}

/**
 * Wait for child process to terminate
 * @param pid Process ID to wait for (0 = any child)
 * @param status Pointer to store exit status
 * @return PID of terminated child, or -1 on error
 */
int32_t process_wait(uint32_t pid, int32_t* status) {
    if (!current_process) {
        return -1;
    }
    
    /* Find a terminated child process */
    process_t* child = NULL;
    
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].ppid == current_process->pid) {
            if (pid == 0 || process_table[i].pid == pid) {
                /* Found a child process */
                if (process_table[i].state == PROCESS_TERMINATED) {
                    /* Child already terminated, harvest it */
                    child = &process_table[i];
                    break;
                }
            }
        }
    }
    
    if (child) {
        /* Return exit status */
        if (status) {
            *status = child->exit_status;
        }
        uint32_t child_pid = child->pid;
        
        /* Clean up the child process entry */
        child->pid = 0;
        child->ppid = 0;
        child->exit_status = 0;
        
        return (int32_t)child_pid;
    }
    
    /* No terminated child found, check if any child exists */
    int has_children = 0;
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].ppid == current_process->pid && 
            process_table[i].pid != 0 &&
            (pid == 0 || process_table[i].pid == pid)) {
            has_children = 1;
            break;
        }
    }
    
    if (!has_children) {
        /* No children to wait for */
        return -1;
    }
    
    /* Block current process until a child terminates */
    current_process->state = PROCESS_WAITING;
    current_process->wait_target = pid;
    
    /* Yield to let other processes run */
    scheduler_schedule();
    
    /* After waking up, try to find the terminated child again */
    for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].ppid == current_process->pid) {
            if (pid == 0 || process_table[i].pid == pid) {
                if (process_table[i].state == PROCESS_TERMINATED) {
                    child = &process_table[i];
                    break;
                }
            }
        }
    }
    
    if (child) {
        if (status) {
            *status = child->exit_status;
        }
        uint32_t child_pid = child->pid;
        
        child->pid = 0;
        child->ppid = 0;
        child->exit_status = 0;
        
        return (int32_t)child_pid;
    }
    
    return -1;
}

/**
 * Execute a program (replaces current process image)
 * @param path Path to executable
 * @param argv Argument vector (NULL-terminated)
 * @return -1 on error (doesn't return on success)
 */
int32_t process_exec(const char* path, char* const argv[]) {
    (void)argv;  /* Arguments not used in simple implementation */
    
    if (!path || !current_process) {
        return -1;
    }
    
    /* In a full implementation, this would:
     * 1. Load executable from filesystem
     * 2. Parse ELF/binary format
     * 3. Set up new address space
     * 4. Copy arguments to new stack
     * 5. Jump to entry point
     *
     * For now, we just validate the path exists and return error
     * since we don't have a full executable loader
     */
    
    /* Check if path is valid (non-empty) */
    if (path[0] == '\0') {
        return -1;
    }
    
    /* Return error - exec not fully implemented */
    /* A real implementation would not return here */
    return -1;
}
