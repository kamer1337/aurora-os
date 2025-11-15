/**
 * Aurora OS - SMP Implementation
 * 
 * Multi-core CPU support and management
 */

#include "smp.h"
#include "../core/kernel.h"
#include <stddef.h>

/* Per-CPU data */
static cpu_info_t cpu_info[MAX_CPUS];
static uint32_t cpu_count = 1; /* At least one CPU (BSP) */
static uint32_t cpus_online = 0;

/* APIC base address (typically at 0xFEE00000) */
#define APIC_BASE 0xFEE00000
#define APIC_ID_REG 0x20
#define APIC_EOI_REG 0xB0
#define APIC_SPURIOUS_REG 0xF0
#define APIC_ICR_LOW 0x300
#define APIC_ICR_HIGH 0x310

/* APIC enable bit */
#define APIC_ENABLE 0x100

/**
 * Read from APIC register
 */
static inline uint32_t apic_read(uint32_t reg) {
    return *(volatile uint32_t*)(APIC_BASE + reg);
}

/**
 * Write to APIC register
 */
static inline void apic_write(uint32_t reg, uint32_t value) {
    *(volatile uint32_t*)(APIC_BASE + reg) = value;
}

/**
 * Get current CPU's APIC ID
 */
static uint32_t get_apic_id(void) {
    return (apic_read(APIC_ID_REG) >> 24) & 0xFF;
}

/**
 * Initialize APIC for current CPU
 */
void apic_init(void) {
    /* Enable APIC by setting spurious interrupt vector */
    apic_write(APIC_SPURIOUS_REG, APIC_ENABLE | 0xFF);
}

/**
 * Send End-Of-Interrupt signal to APIC
 */
void apic_eoi(void) {
    apic_write(APIC_EOI_REG, 0);
}

/**
 * Send Inter-Processor Interrupt
 */
void apic_send_ipi(uint32_t dest_cpu, uint32_t vector) {
    /* Get destination APIC ID */
    uint32_t dest_apic_id = cpu_info[dest_cpu].apic_id;
    
    /* Wait for ICR to be ready */
    while (apic_read(APIC_ICR_LOW) & (1 << 12)) {
        /* Wait */
    }
    
    /* Set destination */
    apic_write(APIC_ICR_HIGH, dest_apic_id << 24);
    
    /* Send IPI */
    apic_write(APIC_ICR_LOW, vector);
}

/**
 * Initialize spinlock
 */
void spinlock_init(spinlock_t* lock) {
    lock->lock = 0;
}

/**
 * Acquire spinlock
 */
void spinlock_acquire(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->lock, 1)) {
        while (lock->lock) {
            /* Spin with pause instruction for better performance */
            __asm__ volatile("pause");
        }
    }
}

/**
 * Release spinlock
 */
void spinlock_release(spinlock_t* lock) {
    __sync_lock_release(&lock->lock);
}

/**
 * Detect number of CPUs
 */
static void detect_cpus(void) {
    /* In a real implementation, we would parse ACPI MADT table
     * or use MP specification to detect CPUs.
     * For now, we'll assume single CPU and mark as detected.
     */
    cpu_count = 1;
    
    /* Initialize BSP (Bootstrap Processor) */
    cpu_info[0].cpu_id = 0;
    cpu_info[0].state = CPU_STATE_ONLINE;
    cpu_info[0].apic_id = get_apic_id();
    cpu_info[0].stack_ptr = NULL;
    cpu_info[0].current_process = NULL;
    cpu_info[0].ticks = 0;
    
    cpus_online = 1;
}

/**
 * Get current CPU ID
 */
uint32_t smp_get_current_cpu_id(void) {
    /* In a real implementation, we would use APIC ID or CPU-local storage
     * For now, return 0 (BSP)
     */
    return 0;
}

/**
 * Get CPU info structure
 */
cpu_info_t* smp_get_cpu_info(uint32_t cpu_id) {
    if (cpu_id >= cpu_count) {
        return NULL;
    }
    return &cpu_info[cpu_id];
}

/**
 * Get number of CPUs
 */
uint32_t smp_get_cpu_count(void) {
    return cpu_count;
}

/**
 * Start application processor (AP)
 */
void smp_start_cpu(uint32_t cpu_id) {
    if (cpu_id >= cpu_count || cpu_id == 0) {
        return; /* Invalid CPU or BSP */
    }
    
    if (cpu_info[cpu_id].state != CPU_STATE_OFFLINE) {
        return; /* Already started */
    }
    
    /* Set state to initializing */
    cpu_info[cpu_id].state = CPU_STATE_INITIALIZING;
    
    /* In a real implementation, we would:
     * 1. Allocate stack for AP
     * 2. Send INIT IPI
     * 3. Send STARTUP IPI with trampoline code address
     * 4. Wait for AP to come online
     */
    
    /* For now, just mark as online */
    cpu_info[cpu_id].state = CPU_STATE_ONLINE;
    cpus_online++;
}

/**
 * Halt CPU
 */
void smp_halt_cpu(uint32_t cpu_id) {
    if (cpu_id >= cpu_count) {
        return;
    }
    
    cpu_info[cpu_id].state = CPU_STATE_HALTED;
    cpus_online--;
}

/**
 * Initialize SMP subsystem
 */
void smp_init(void) {
    /* Initialize all CPU entries */
    for (uint32_t i = 0; i < MAX_CPUS; i++) {
        cpu_info[i].cpu_id = i;
        cpu_info[i].state = CPU_STATE_OFFLINE;
        cpu_info[i].apic_id = 0;
        cpu_info[i].stack_ptr = NULL;
        cpu_info[i].current_process = NULL;
        cpu_info[i].ticks = 0;
    }
    
    /* Initialize APIC */
    apic_init();
    
    /* Detect CPUs */
    detect_cpus();
}
