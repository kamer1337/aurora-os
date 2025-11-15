/**
 * Aurora OS - SMP (Symmetric Multiprocessing) Support Header
 * 
 * Multi-core CPU support and management
 */

#ifndef AURORA_SMP_H
#define AURORA_SMP_H

#include <stdint.h>

/* Maximum number of CPUs supported */
#define MAX_CPUS 16

/* CPU states */
typedef enum {
    CPU_STATE_OFFLINE,
    CPU_STATE_INITIALIZING,
    CPU_STATE_ONLINE,
    CPU_STATE_HALTED
} cpu_state_t;

/* Per-CPU data structure */
typedef struct {
    uint32_t cpu_id;
    cpu_state_t state;
    uint32_t apic_id;
    void* stack_ptr;
    void* current_process;
    uint32_t ticks;
} cpu_info_t;

/* Spinlock for multi-core synchronization */
typedef struct {
    volatile uint32_t lock;
} spinlock_t;

/* SMP initialization functions */
void smp_init(void);
uint32_t smp_get_cpu_count(void);
uint32_t smp_get_current_cpu_id(void);
cpu_info_t* smp_get_cpu_info(uint32_t cpu_id);

/* CPU control functions */
void smp_start_cpu(uint32_t cpu_id);
void smp_halt_cpu(uint32_t cpu_id);

/* Spinlock functions */
void spinlock_init(spinlock_t* lock);
void spinlock_acquire(spinlock_t* lock);
void spinlock_release(spinlock_t* lock);

/* APIC functions */
void apic_init(void);
void apic_send_ipi(uint32_t dest_cpu, uint32_t vector);
void apic_eoi(void);

#endif /* AURORA_SMP_H */
