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
    
    /* Setup stack pointer (stack grows downward) - 64-bit aligned */
    uint64_t* stack_top = (uint64_t*)((uint8_t*)stack + PROCESS_STACK_SIZE);
    
    /* Push initial context onto stack - 64-bit entry point */
    stack_top--;
    *stack_top = (uint64_t)(uintptr_t)entry; /* Entry point */
    
    process->stack_ptr = (void*)stack_top;
    
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
    
    /* Save the parent PID before clearing */
    uint32_t parent_pid = process->ppid;
    
    /* Free process resources */
    if (process->stack_ptr) {
        /* Calculate stack base from stack pointer - 64-bit compatible */
        uintptr_t stack_addr = (uintptr_t)process->stack_ptr;
        void* stack_base = (void*)(stack_addr & ~((uintptr_t)PROCESS_STACK_SIZE - 1));
        kfree(stack_base);
    }
    
    /* Mark process as terminated (but keep pid/ppid for wait() to collect) */
    process->state = PROCESS_TERMINATED;
    process->stack_ptr = NULL;
    process->next = NULL;
    
    /* Wake up parent if it's waiting for this child */
    if (parent_pid != 0) {
        for (uint32_t i = 0; i < MAX_PROCESSES; i++) {
            if (process_table[i].pid == parent_pid && 
                process_table[i].state == PROCESS_WAITING) {
                /* Check if parent is waiting for this specific child or any child */
                if (process_table[i].wait_target == 0 || 
                    process_table[i].wait_target == pid) {
                    /* Wake up the parent */
                    process_table[i].state = PROCESS_READY;
                    process_table[i].wait_target = 0;
                    enqueue_process(&process_table[i]);
                    break;
                }
            }
        }
    }
    
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
 * Uses 64-bit registers for long mode compatibility
 */
typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t rflags;
    uint8_t initialized;  /* Flag to indicate context has been saved */
} cpu_context_t;

/* Context storage for each process
 * Note: Array is zero-initialized, so 'initialized' flag is 0 for new processes.
 * This means new processes will not have their context restored on first run,
 * they will execute from their entry point on the stack.
 */
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
    
    /* Save general purpose registers for 64-bit mode */
    __asm__ volatile("movq %%rax, %0" : "=m"(ctx->rax));
    __asm__ volatile("movq %%rbx, %0" : "=m"(ctx->rbx));
    __asm__ volatile("movq %%rcx, %0" : "=m"(ctx->rcx));
    __asm__ volatile("movq %%rdx, %0" : "=m"(ctx->rdx));
    __asm__ volatile("movq %%rsi, %0" : "=m"(ctx->rsi));
    __asm__ volatile("movq %%rdi, %0" : "=m"(ctx->rdi));
    __asm__ volatile("movq %%rbp, %0" : "=m"(ctx->rbp));
    __asm__ volatile("movq %%rsp, %0" : "=m"(ctx->rsp));
    __asm__ volatile("movq %%r8, %0" : "=m"(ctx->r8));
    __asm__ volatile("movq %%r9, %0" : "=m"(ctx->r9));
    __asm__ volatile("movq %%r10, %0" : "=m"(ctx->r10));
    __asm__ volatile("movq %%r11, %0" : "=m"(ctx->r11));
    __asm__ volatile("movq %%r12, %0" : "=m"(ctx->r12));
    __asm__ volatile("movq %%r13, %0" : "=m"(ctx->r13));
    __asm__ volatile("movq %%r14, %0" : "=m"(ctx->r14));
    __asm__ volatile("movq %%r15, %0" : "=m"(ctx->r15));
    
    /* Save flags */
    __asm__ volatile("pushfq; popq %0" : "=m"(ctx->rflags));
    
    /* Mark context as initialized */
    ctx->initialized = 1;
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
    __asm__ volatile("pushq %0; popfq" : : "m"(ctx->rflags));
    
    /* Restore general purpose registers for 64-bit mode */
    __asm__ volatile("movq %0, %%r15" : : "m"(ctx->r15));
    __asm__ volatile("movq %0, %%r14" : : "m"(ctx->r14));
    __asm__ volatile("movq %0, %%r13" : : "m"(ctx->r13));
    __asm__ volatile("movq %0, %%r12" : : "m"(ctx->r12));
    __asm__ volatile("movq %0, %%r11" : : "m"(ctx->r11));
    __asm__ volatile("movq %0, %%r10" : : "m"(ctx->r10));
    __asm__ volatile("movq %0, %%r9" : : "m"(ctx->r9));
    __asm__ volatile("movq %0, %%r8" : : "m"(ctx->r8));
    __asm__ volatile("movq %0, %%rdi" : : "m"(ctx->rdi));
    __asm__ volatile("movq %0, %%rsi" : : "m"(ctx->rsi));
    __asm__ volatile("movq %0, %%rdx" : : "m"(ctx->rdx));
    __asm__ volatile("movq %0, %%rcx" : : "m"(ctx->rcx));
    __asm__ volatile("movq %0, %%rbx" : : "m"(ctx->rbx));
    __asm__ volatile("movq %0, %%rbp" : : "m"(ctx->rbp));
    __asm__ volatile("movq %0, %%rax" : : "m"(ctx->rax));
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
        __asm__ volatile("movq %%rsp, %0" : "=m"(from->stack_ptr));
    }
    
    /* Switch to new process */
    current_process = to;
    to->state = PROCESS_RUNNING;
    
    /* Restore stack pointer from process control block */
    if (to->stack_ptr) {
        __asm__ volatile("movq %0, %%rsp" : : "m"(to->stack_ptr));
        
        /* Restore context of previously-run process */
        /* For new processes, the context was initialized at creation time */
        int idx = get_process_index(to);
        if (idx >= 0 && process_contexts[idx].initialized) {
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
