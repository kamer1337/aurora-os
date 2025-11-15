/**
 * Aurora OS - Interrupt Handling Implementation
 * 
 * IDT setup and interrupt service routines
 */

#include "interrupt.h"
#include <stddef.h>

/* IDT with 256 entries */
#define IDT_ENTRIES 256
static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idt_ptr;

/* Interrupt handler table */
static interrupt_handler_t interrupt_handlers[IDT_ENTRIES];

/**
 * Set an entry in the IDT
 */
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector = selector;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/**
 * Load the IDT
 */
static void idt_load(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * IDT_ENTRIES - 1;
    idt_ptr.base = (uint32_t)&idt;
    
    __asm__ volatile("lidt %0" : : "m"(idt_ptr));
}

/**
 * Default interrupt handler
 */
static void default_handler(void) {
    /* Do nothing */
}

/**
 * Initialize interrupt descriptor table
 */
void interrupt_init(void) {
    /* Clear IDT */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
        interrupt_handlers[i] = default_handler;
    }
    
    /* Load IDT */
    idt_load();
    
    /* Initialize system call interface */
    syscall_init();
}

/**
 * Enable interrupts
 */
void interrupt_enable(void) {
    __asm__ volatile("sti");
}

/**
 * Disable interrupts
 */
void interrupt_disable(void) {
    __asm__ volatile("cli");
}

/**
 * Register an interrupt handler
 */
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler) {
    if (num < IDT_ENTRIES && handler) {
        interrupt_handlers[num] = handler;
    }
}

/**
 * Common interrupt handler entry point
 */
void interrupt_dispatch(uint8_t num) {
    if (num < IDT_ENTRIES && interrupt_handlers[num]) {
        interrupt_handlers[num]();
    }
}

/* System call definitions */
#define SYSCALL_EXIT    0
#define SYSCALL_FORK    1
#define SYSCALL_READ    2
#define SYSCALL_WRITE   3
#define SYSCALL_OPEN    4
#define SYSCALL_CLOSE   5
#define SYSCALL_WAIT    6
#define SYSCALL_EXEC    7
#define SYSCALL_YIELD   8

/**
 * Initialize system call interface
 */
void syscall_init(void) {
    /* System calls use interrupt 0x80 */
    /* In a real implementation, would set up IDT entry for INT 0x80 */
}

/**
 * System call handler
 */
int syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
    (void)arg1;
    (void)arg2;
    (void)arg3;
    
    switch (syscall_num) {
        case SYSCALL_EXIT:
            /* Exit process */
            return 0;
            
        case SYSCALL_FORK:
            /* Fork process */
            return -1; /* Not implemented */
            
        case SYSCALL_READ:
            /* Read from file descriptor */
            return -1; /* Not implemented */
            
        case SYSCALL_WRITE:
            /* Write to file descriptor */
            return -1; /* Not implemented */
            
        case SYSCALL_OPEN:
            /* Open file */
            return -1; /* Not implemented */
            
        case SYSCALL_CLOSE:
            /* Close file descriptor */
            return -1; /* Not implemented */
            
        case SYSCALL_WAIT:
            /* Wait for child process */
            return -1; /* Not implemented */
            
        case SYSCALL_EXEC:
            /* Execute program */
            return -1; /* Not implemented */
            
        case SYSCALL_YIELD:
            /* Yield CPU */
            return 0;
            
        default:
            return -1; /* Unknown system call */
    }
}
