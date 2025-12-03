/**
 * Aurora OS - ARM Architecture Support Header
 * 
 * Provides support for ARM processors (ARMv7-A and ARMv8-A/AArch64)
 */

#ifndef ARM_SUPPORT_H
#define ARM_SUPPORT_H

#include <stdint.h>

/* ARM architecture detection */
#if defined(__arm__) || defined(__aarch64__)
#define AURORA_ARM_SUPPORT 1
#else
#define AURORA_ARM_SUPPORT 0
#endif

#if defined(__aarch64__)
#define AURORA_ARM64 1
#else
#define AURORA_ARM64 0
#endif

/* ARM processor features */
#define ARM_FEATURE_NEON        0x0001  /* NEON SIMD */
#define ARM_FEATURE_VFP         0x0002  /* Vector Floating Point */
#define ARM_FEATURE_THUMB       0x0004  /* Thumb instruction set */
#define ARM_FEATURE_TRUSTZONE   0x0008  /* TrustZone security */
#define ARM_FEATURE_CRYPTO      0x0010  /* Cryptography extensions */
#define ARM_FEATURE_SVE         0x0020  /* Scalable Vector Extensions (ARMv8.2+) */
#define ARM_FEATURE_PAC         0x0040  /* Pointer Authentication */
#define ARM_FEATURE_BTI         0x0080  /* Branch Target Identification */

/* ARM CPU state structure */
typedef struct {
    uint64_t regs[31];      /* x0-x30 (AArch64) or r0-r14 (AArch32) */
    uint64_t sp;            /* Stack pointer */
    uint64_t pc;            /* Program counter */
    uint64_t pstate;        /* Processor state (CPSR/SPSR) */
    uint64_t elr_el1;       /* Exception link register */
    uint64_t spsr_el1;      /* Saved program status register */
} arm_cpu_state_t;

/* ARM system registers (AArch64) */
typedef struct {
    uint64_t sctlr_el1;     /* System control register */
    uint64_t tcr_el1;       /* Translation control register */
    uint64_t ttbr0_el1;     /* Translation table base register 0 */
    uint64_t ttbr1_el1;     /* Translation table base register 1 */
    uint64_t mair_el1;      /* Memory attribute indirection register */
    uint64_t vbar_el1;      /* Vector base address register */
} arm_system_regs_t;

/* ARM MMU page table entry (AArch64) */
typedef struct {
    uint64_t entries[512];
} arm_page_table_t;

/* ARM cache operations */
#define ARM_CACHE_CLEAN         0
#define ARM_CACHE_INVALIDATE    1
#define ARM_CACHE_CLEAN_INV     2

/* Core ARM functions */
int arm_support_init(void);
int arm_detect_processor(void);
uint32_t arm_get_features(void);
void arm_enable_mmu(void);
void arm_disable_mmu(void);

/* Cache management */
void arm_dcache_clean(void);
void arm_dcache_invalidate(void);
void arm_dcache_clean_invalidate(void);
void arm_icache_invalidate(void);

/* Page table management */
int arm_create_page_table(arm_page_table_t** table);
int arm_map_page(arm_page_table_t* table, uint64_t virt, uint64_t phys, uint32_t flags);
void arm_set_ttbr0(uint64_t table_addr);
void arm_set_ttbr1(uint64_t table_addr);

/* Context switching */
void arm_save_context(arm_cpu_state_t* state);
void arm_restore_context(const arm_cpu_state_t* state);

/* Interrupt management */
void arm_enable_interrupts(void);
void arm_disable_interrupts(void);
int arm_in_interrupt_context(void);

/* System calls */
void arm_syscall_init(void);
int arm_syscall_handler(uint32_t syscall_num, uint64_t* args);

/* Performance monitoring */
void arm_pmu_init(void);
uint64_t arm_read_cycle_counter(void);
uint64_t arm_read_instruction_counter(void);

/* GIC (Generic Interrupt Controller) support */
int arm_gic_init(void);
void arm_gic_enable_irq(uint32_t irq);
void arm_gic_disable_irq(uint32_t irq);
void arm_gic_set_priority(uint32_t irq, uint8_t priority);

/* Timer functions */
void arm_timer_init(void);
uint64_t arm_timer_get_ticks(void);
void arm_timer_set_alarm(uint64_t ticks);

#endif /* ARM_SUPPORT_H */
