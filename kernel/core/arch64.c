/**
 * Aurora OS - 64-bit Architecture Support Implementation
 */

#include "arch64.h"
#include <stddef.h>

#if AURORA_ARCH_64BIT

/* Global page tables for 64-bit mode */
static page_table_64_t pml4_table __attribute__((aligned(4096)));
static page_table_64_t pdp_table __attribute__((aligned(4096)));
static page_table_64_t pd_table __attribute__((aligned(4096)));
static page_table_64_t pt_table __attribute__((aligned(4096)));

/**
 * Initialize 64-bit mode
 */
void arch64_init(void) {
    /* Initialize page tables for 64-bit mode */
    for (int i = 0; i < 512; i++) {
        pml4_table.entries[i] = 0;
        pdp_table.entries[i] = 0;
        pd_table.entries[i] = 0;
        pt_table.entries[i] = 0;
    }
    
    /* Set up initial identity mapping for first 1GB */
    /* This is simplified; real implementation would be more complex */
    pml4_table.entries[0] = ((uint64_t)&pdp_table) | PTE_PRESENT | PTE_WRITABLE;
    pdp_table.entries[0] = ((uint64_t)&pd_table) | PTE_PRESENT | PTE_WRITABLE;
    
    for (int i = 0; i < 512; i++) {
        pd_table.entries[i] = ((uint64_t)&pt_table) | PTE_PRESENT | PTE_WRITABLE;
    }
}

/**
 * Map a page in 64-bit mode
 */
int arch64_map_page(uint64_t physical, uint64_t virtual, uint64_t flags) {
    /* Extract page table indices from virtual address */
    uint64_t pml4_index = (virtual >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual >> 30) & 0x1FF;
    uint64_t pd_index = (virtual >> 21) & 0x1FF;
    uint64_t pt_index = (virtual >> 12) & 0x1FF;
    
    /* In a real implementation, would walk page tables and create entries as needed */
    /* For now, this is a simplified placeholder */
    
    (void)pml4_index;
    (void)pdp_index;
    (void)pd_index;
    (void)pt_index;
    (void)physical;
    (void)flags;
    
    return 0;
}

/**
 * Unmap a page in 64-bit mode
 */
int arch64_unmap_page(uint64_t virtual) {
    /* Extract page table indices from virtual address */
    uint64_t pml4_index = (virtual >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual >> 30) & 0x1FF;
    uint64_t pd_index = (virtual >> 21) & 0x1FF;
    uint64_t pt_index = (virtual >> 12) & 0x1FF;
    
    /* In a real implementation, would clear page table entry */
    
    (void)pml4_index;
    (void)pdp_index;
    (void)pd_index;
    (void)pt_index;
    
    return 0;
}

/**
 * Get physical address from virtual address
 */
uint64_t arch64_get_physical_address(uint64_t virtual) {
    /* In a real implementation, would walk page tables */
    /* For now, return identity mapping */
    return virtual;
}

/**
 * Check if CPU has a specific feature
 */
int arch64_has_feature(const char* feature) {
    /* In a real implementation, would check CPUID */
    (void)feature;
    return 1;  /* Assume feature is supported */
}

/**
 * Enable a CPU feature
 */
void arch64_enable_feature(const char* feature) {
    /* In a real implementation, would set appropriate CPU flags */
    (void)feature;
}

/**
 * Read MSR (Model Specific Register)
 */
uint64_t arch64_read_msr(uint32_t msr) {
    uint32_t low, high;
    __asm__ volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

/**
 * Write MSR (Model Specific Register)
 */
void arch64_write_msr(uint32_t msr, uint64_t value) {
    uint32_t low = value & 0xFFFFFFFF;
    uint32_t high = value >> 32;
    __asm__ volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr));
}

/**
 * 64-bit system call
 */
arch_ptr_t arch64_syscall(arch_ptr_t syscall_number, arch_ptr_t arg1,
                          arch_ptr_t arg2, arch_ptr_t arg3) {
    /* In a real implementation, would use syscall instruction */
    (void)syscall_number;
    (void)arg1;
    (void)arg2;
    (void)arg3;
    return 0;
}

#endif /* AURORA_ARCH_64BIT */

/**
 * Get architecture bit width
 */
int arch_get_bits(void) {
    return ARCH_BITS;
}

/**
 * Get stack pointer
 */
arch_ptr_t arch_get_stack_pointer(void) {
#if AURORA_ARCH_64BIT
    arch_ptr_t sp;
    __asm__ volatile("mov %%rsp, %0" : "=r"(sp));
    return sp;
#else
    arch_ptr_t sp;
    __asm__ volatile("mov %%esp, %0" : "=r"(sp));
    return sp;
#endif
}

/**
 * Get instruction pointer
 */
arch_ptr_t arch_get_instruction_pointer(void) {
#if AURORA_ARCH_64BIT
    arch_ptr_t ip;
    __asm__ volatile("lea (%%rip), %0" : "=r"(ip));
    return ip;
#else
    arch_ptr_t ip;
    __asm__ volatile("call 1f; 1: pop %0" : "=r"(ip));
    return ip;
#endif
}
