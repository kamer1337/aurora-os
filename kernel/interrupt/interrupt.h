/**
 * Aurora OS - Interrupt Handling Header
 * 
 * Interrupt descriptor table and interrupt service routines
 */

#ifndef AURORA_INTERRUPT_H
#define AURORA_INTERRUPT_H

#include <stdint.h>

/* IDT entry structure */
typedef struct idt_entry {
    uint16_t base_low;
    uint16_t selector;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed)) idt_entry_t;

/* IDT pointer structure */
typedef struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

/* Interrupt numbers */
#define IRQ0  32
#define IRQ1  33
#define IRQ2  34
#define IRQ3  35
#define IRQ4  36
#define IRQ5  37
#define IRQ6  38
#define IRQ7  39
#define IRQ8  40
#define IRQ9  41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

/* Interrupt handler type */
typedef void (*interrupt_handler_t)(void);

/* Interrupt management functions */
void interrupt_init(void);
void interrupt_enable(void);
void interrupt_disable(void);
void register_interrupt_handler(uint8_t num, interrupt_handler_t handler);

/* System call interface */
void syscall_init(void);
int syscall_handler(uint32_t syscall_num, uint32_t arg1, uint32_t arg2, uint32_t arg3);

#endif /* AURORA_INTERRUPT_H */
