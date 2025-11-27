/**
 * Aurora OS - 64-bit Architecture Support
 * 
 * Provides support for 64-bit x86-64 architecture
 */

#ifndef AURORA_ARCH64_H
#define AURORA_ARCH64_H

#include <stdint.h>

/* Architecture detection */
#ifdef __x86_64__
#define AURORA_ARCH_64BIT 1
#define AURORA_ARCH_32BIT 0
#else
#define AURORA_ARCH_64BIT 0
#define AURORA_ARCH_32BIT 1
#endif

/* Architecture-specific types */
#if AURORA_ARCH_64BIT
typedef uint64_t arch_ptr_t;
typedef int64_t arch_ssize_t;
#define ARCH_BITS 64
#else
typedef uint32_t arch_ptr_t;
typedef int32_t arch_ssize_t;
#define ARCH_BITS 32
#endif

/* Page size definitions */
#define PAGE_SIZE_4K   0x1000
#define PAGE_SIZE_2M   0x200000
#define PAGE_SIZE_1G   0x40000000

/* 64-bit specific features */
#if AURORA_ARCH_64BIT

/* Long mode page table entry flags */
#define PTE_PRESENT     (1ULL << 0)
#define PTE_WRITABLE    (1ULL << 1)
#define PTE_USER        (1ULL << 2)
#define PTE_WRITE_THROUGH (1ULL << 3)
#define PTE_CACHE_DISABLE (1ULL << 4)
#define PTE_ACCESSED    (1ULL << 5)
#define PTE_DIRTY       (1ULL << 6)
#define PTE_HUGE        (1ULL << 7)
#define PTE_GLOBAL      (1ULL << 8)
#define PTE_NO_EXECUTE  (1ULL << 63)

/* Page table structures for 64-bit */
typedef struct {
    uint64_t entries[512];
} page_table_64_t;

/* Initialize 64-bit mode */
void arch64_init(void);

/* 64-bit memory management */
int arch64_map_page(uint64_t physical, uint64_t virtual, uint64_t flags);
int arch64_unmap_page(uint64_t virtual);
uint64_t arch64_get_physical_address(uint64_t virtual);

/* 64-bit CPU features */
int arch64_has_feature(const char* feature);
void arch64_enable_feature(const char* feature);

/* MSR (Model Specific Register) operations */
uint64_t arch64_read_msr(uint32_t msr);
void arch64_write_msr(uint32_t msr, uint64_t value);

/* Syscall/sysret configuration */
void arch64_syscall_init(uint64_t handler_addr);

/* 64-bit specific system calls */
arch_ptr_t arch64_syscall(arch_ptr_t syscall_number, arch_ptr_t arg1, 
                          arch_ptr_t arg2, arch_ptr_t arg3);

#endif /* AURORA_ARCH_64BIT */

/* Generic architecture functions that work for both 32-bit and 64-bit */
int arch_get_bits(void);
arch_ptr_t arch_get_stack_pointer(void);
arch_ptr_t arch_get_instruction_pointer(void);

#endif /* AURORA_ARCH64_H */
