/**
 * Aurora OS - 64-bit Architecture Support Implementation
 */

#include "arch64.h"
#include <stddef.h>

#if AURORA_ARCH_64BIT

/* Global page tables for 64-bit mode */
static page_table_64_t pml4_table __attribute__((aligned(4096)));
static page_table_64_t pdp_tables[512] __attribute__((aligned(4096)));
static page_table_64_t pd_tables[512] __attribute__((aligned(4096)));
static page_table_64_t pt_tables[512] __attribute__((aligned(4096)));

/* Page table allocation tracking */
static uint8_t pdp_allocated[512];
static uint8_t pd_allocated[512];
static uint8_t pt_allocated[512];

/* Helper function to compare strings */
static int arch64_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

/**
 * Initialize 64-bit mode
 */
void arch64_init(void) {
    /* Initialize page tables for 64-bit mode */
    for (int i = 0; i < 512; i++) {
        pml4_table.entries[i] = 0;
        pdp_allocated[i] = 0;
        pd_allocated[i] = 0;
        pt_allocated[i] = 0;
    }
    
    /* Initialize all PDP, PD, and PT tables */
    for (int j = 0; j < 512; j++) {
        for (int i = 0; i < 512; i++) {
            pdp_tables[j].entries[i] = 0;
            pd_tables[j].entries[i] = 0;
            pt_tables[j].entries[i] = 0;
        }
    }
    
    /* Set up initial identity mapping for first 2MB using 4KB pages */
    pml4_table.entries[0] = ((uint64_t)&pdp_tables[0]) | PTE_PRESENT | PTE_WRITABLE;
    pdp_tables[0].entries[0] = ((uint64_t)&pd_tables[0]) | PTE_PRESENT | PTE_WRITABLE;
    pd_tables[0].entries[0] = ((uint64_t)&pt_tables[0]) | PTE_PRESENT | PTE_WRITABLE;
    
    pdp_allocated[0] = 1;
    pd_allocated[0] = 1;
    pt_allocated[0] = 1;
    
    /* Map first 2MB as identity mapping */
    for (int i = 0; i < 512; i++) {
        pt_tables[0].entries[i] = (uint64_t)(i * PAGE_SIZE_4K) | PTE_PRESENT | PTE_WRITABLE;
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
    
    /* Ensure PML4 entry exists */
    if (!(pml4_table.entries[pml4_index] & PTE_PRESENT)) {
        /* Find free PDP table */
        int pdp_idx = -1;
        for (int i = 0; i < 512; i++) {
            if (!pdp_allocated[i]) {
                pdp_idx = i;
                pdp_allocated[i] = 1;
                break;
            }
        }
        if (pdp_idx < 0) return -1; /* No free PDP tables */
        
        /* Clear the PDP table */
        for (int i = 0; i < 512; i++) {
            pdp_tables[pdp_idx].entries[i] = 0;
        }
        
        pml4_table.entries[pml4_index] = ((uint64_t)&pdp_tables[pdp_idx]) | PTE_PRESENT | PTE_WRITABLE;
    }
    
    /* Get PDP table address */
    page_table_64_t* pdp = (page_table_64_t*)(pml4_table.entries[pml4_index] & ~0xFFFULL);
    
    /* Ensure PDP entry exists */
    if (!(pdp->entries[pdp_index] & PTE_PRESENT)) {
        /* Find free PD table */
        int pd_idx = -1;
        for (int i = 0; i < 512; i++) {
            if (!pd_allocated[i]) {
                pd_idx = i;
                pd_allocated[i] = 1;
                break;
            }
        }
        if (pd_idx < 0) return -1; /* No free PD tables */
        
        /* Clear the PD table */
        for (int i = 0; i < 512; i++) {
            pd_tables[pd_idx].entries[i] = 0;
        }
        
        pdp->entries[pdp_index] = ((uint64_t)&pd_tables[pd_idx]) | PTE_PRESENT | PTE_WRITABLE;
    }
    
    /* Get PD table address */
    page_table_64_t* pd = (page_table_64_t*)(pdp->entries[pdp_index] & ~0xFFFULL);
    
    /* Ensure PD entry exists */
    if (!(pd->entries[pd_index] & PTE_PRESENT)) {
        /* Find free PT table */
        int pt_idx = -1;
        for (int i = 0; i < 512; i++) {
            if (!pt_allocated[i]) {
                pt_idx = i;
                pt_allocated[i] = 1;
                break;
            }
        }
        if (pt_idx < 0) return -1; /* No free PT tables */
        
        /* Clear the PT table */
        for (int i = 0; i < 512; i++) {
            pt_tables[pt_idx].entries[i] = 0;
        }
        
        pd->entries[pd_index] = ((uint64_t)&pt_tables[pt_idx]) | PTE_PRESENT | PTE_WRITABLE;
    }
    
    /* Get PT table address */
    page_table_64_t* pt = (page_table_64_t*)(pd->entries[pd_index] & ~0xFFFULL);
    
    /* Map the page */
    pt->entries[pt_index] = (physical & ~0xFFFULL) | (flags & 0xFFF) | PTE_PRESENT;
    
    /* Invalidate TLB for this page */
    __asm__ volatile("invlpg (%0)" : : "r"(virtual) : "memory");
    
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
    
    /* Check if PML4 entry exists */
    if (!(pml4_table.entries[pml4_index] & PTE_PRESENT)) {
        return -1; /* Page not mapped */
    }
    
    /* Get PDP table */
    page_table_64_t* pdp = (page_table_64_t*)(pml4_table.entries[pml4_index] & ~0xFFFULL);
    
    /* Check if PDP entry exists */
    if (!(pdp->entries[pdp_index] & PTE_PRESENT)) {
        return -1; /* Page not mapped */
    }
    
    /* Get PD table */
    page_table_64_t* pd = (page_table_64_t*)(pdp->entries[pdp_index] & ~0xFFFULL);
    
    /* Check if PD entry exists */
    if (!(pd->entries[pd_index] & PTE_PRESENT)) {
        return -1; /* Page not mapped */
    }
    
    /* Get PT table */
    page_table_64_t* pt = (page_table_64_t*)(pd->entries[pd_index] & ~0xFFFULL);
    
    /* Check if page is mapped */
    if (!(pt->entries[pt_index] & PTE_PRESENT)) {
        return -1; /* Page not mapped */
    }
    
    /* Clear the page table entry */
    pt->entries[pt_index] = 0;
    
    /* Invalidate TLB for this page */
    __asm__ volatile("invlpg (%0)" : : "r"(virtual) : "memory");
    
    return 0;
}

/**
 * Get physical address from virtual address
 */
uint64_t arch64_get_physical_address(uint64_t virtual) {
    /* Extract page table indices from virtual address */
    uint64_t pml4_index = (virtual >> 39) & 0x1FF;
    uint64_t pdp_index = (virtual >> 30) & 0x1FF;
    uint64_t pd_index = (virtual >> 21) & 0x1FF;
    uint64_t pt_index = (virtual >> 12) & 0x1FF;
    uint64_t page_offset = virtual & 0xFFF;
    
    /* Walk page tables */
    if (!(pml4_table.entries[pml4_index] & PTE_PRESENT)) {
        return virtual; /* Return identity mapping if not present */
    }
    
    page_table_64_t* pdp = (page_table_64_t*)(pml4_table.entries[pml4_index] & ~0xFFFULL);
    
    if (!(pdp->entries[pdp_index] & PTE_PRESENT)) {
        return virtual;
    }
    
    /* Check for 1GB huge page */
    if (pdp->entries[pdp_index] & PTE_HUGE) {
        return (pdp->entries[pdp_index] & ~0x3FFFFFFFULL) | (virtual & 0x3FFFFFFFULL);
    }
    
    page_table_64_t* pd = (page_table_64_t*)(pdp->entries[pdp_index] & ~0xFFFULL);
    
    if (!(pd->entries[pd_index] & PTE_PRESENT)) {
        return virtual;
    }
    
    /* Check for 2MB huge page */
    if (pd->entries[pd_index] & PTE_HUGE) {
        return (pd->entries[pd_index] & ~0x1FFFFFULL) | (virtual & 0x1FFFFFULL);
    }
    
    page_table_64_t* pt = (page_table_64_t*)(pd->entries[pd_index] & ~0xFFFULL);
    
    if (!(pt->entries[pt_index] & PTE_PRESENT)) {
        return virtual;
    }
    
    return (pt->entries[pt_index] & ~0xFFFULL) | page_offset;
}

/**
 * Check if CPU has a specific feature using CPUID
 */
int arch64_has_feature(const char* feature) {
    if (!feature) {
        return 0;
    }
    
    uint32_t eax, ebx, ecx, edx;
    
    /* Check standard features (CPUID leaf 1) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
    
    /* SSE features */
    if (arch64_strcmp(feature, "sse") == 0) {
        return (edx >> 25) & 1;
    }
    if (arch64_strcmp(feature, "sse2") == 0) {
        return (edx >> 26) & 1;
    }
    if (arch64_strcmp(feature, "sse3") == 0) {
        return ecx & 1;
    }
    if (arch64_strcmp(feature, "ssse3") == 0) {
        return (ecx >> 9) & 1;
    }
    if (arch64_strcmp(feature, "sse4.1") == 0 || arch64_strcmp(feature, "sse4_1") == 0) {
        return (ecx >> 19) & 1;
    }
    if (arch64_strcmp(feature, "sse4.2") == 0 || arch64_strcmp(feature, "sse4_2") == 0) {
        return (ecx >> 20) & 1;
    }
    
    /* AVX */
    if (arch64_strcmp(feature, "avx") == 0) {
        return (ecx >> 28) & 1;
    }
    
    /* FPU */
    if (arch64_strcmp(feature, "fpu") == 0) {
        return edx & 1;
    }
    
    /* MMX */
    if (arch64_strcmp(feature, "mmx") == 0) {
        return (edx >> 23) & 1;
    }
    
    /* APIC */
    if (arch64_strcmp(feature, "apic") == 0) {
        return (edx >> 9) & 1;
    }
    
    /* Check extended features (CPUID leaf 7) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
    
    if (arch64_strcmp(feature, "avx2") == 0) {
        return (ebx >> 5) & 1;
    }
    if (arch64_strcmp(feature, "avx512f") == 0) {
        return (ebx >> 16) & 1;
    }
    
    /* Check extended CPUID features (leaf 0x80000001) */
    __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000001));
    
    if (arch64_strcmp(feature, "nx") == 0) {
        return (edx >> 20) & 1;
    }
    if (arch64_strcmp(feature, "lm") == 0 || arch64_strcmp(feature, "long_mode") == 0) {
        return (edx >> 29) & 1;
    }
    
    /* Unknown feature - return 0 */
    return 0;
}

/**
 * Enable a CPU feature
 */
void arch64_enable_feature(const char* feature) {
    if (!feature) {
        return;
    }
    
    uint64_t cr0, cr4;
    
    /* Read current control registers */
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile("mov %%cr4, %0" : "=r"(cr4));
    
    if (arch64_strcmp(feature, "sse") == 0 || arch64_strcmp(feature, "sse2") == 0) {
        /* Enable SSE: CR0.EM = 0, CR4.OSFXSR = 1, CR4.OSXMMEXCPT = 1 */
        cr0 &= ~(1ULL << 2);  /* Clear EM */
        cr4 |= (1ULL << 9);   /* Set OSFXSR */
        cr4 |= (1ULL << 10);  /* Set OSXMMEXCPT */
        __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    }
    else if (arch64_strcmp(feature, "avx") == 0 || arch64_strcmp(feature, "avx2") == 0) {
        /* Enable AVX: Also need to set XCR0 */
        /* First enable OSXSAVE in CR4 */
        cr4 |= (1ULL << 18);  /* Set OSXSAVE */
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
        
        /* Then enable AVX in XCR0 */
        uint32_t xcr0_low = 7;  /* Enable X87, SSE, and AVX state */
        uint32_t xcr0_high = 0;
        __asm__ volatile("xsetbv" : : "a"(xcr0_low), "d"(xcr0_high), "c"(0));
    }
    else if (arch64_strcmp(feature, "pge") == 0) {
        /* Enable Page Global Enable */
        cr4 |= (1ULL << 7);  /* Set PGE */
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    }
    else if (arch64_strcmp(feature, "pse") == 0) {
        /* Enable Page Size Extensions (for 4MB pages) */
        cr4 |= (1ULL << 4);  /* Set PSE */
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    }
    else if (arch64_strcmp(feature, "pae") == 0) {
        /* Enable Physical Address Extension */
        cr4 |= (1ULL << 5);  /* Set PAE */
        __asm__ volatile("mov %0, %%cr4" : : "r"(cr4));
    }
    else if (arch64_strcmp(feature, "nx") == 0) {
        /* Enable No-Execute bit via EFER MSR */
        uint64_t efer = arch64_read_msr(0xC0000080);  /* EFER MSR */
        efer |= (1ULL << 11);  /* Set NXE */
        arch64_write_msr(0xC0000080, efer);
    }
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

/* MSR addresses for syscall configuration */
#define MSR_STAR    0xC0000081  /* Segment selector MSR */
#define MSR_LSTAR   0xC0000082  /* 64-bit syscall handler address */
#define MSR_CSTAR   0xC0000083  /* 32-bit compatibility mode syscall */
#define MSR_SFMASK  0xC0000084  /* Syscall flag mask */

/* Flag to track if syscall instruction is properly configured */
static int syscall_configured = 0;

/**
 * Configure syscall/sysret instruction support
 * This must be called before using arch64_syscall with hardware syscall
 * handler_addr: Address of the syscall handler function
 */
void arch64_syscall_init(uint64_t handler_addr) {
    if (!handler_addr) {
        syscall_configured = 0;
        return;
    }
    
    /* Set up STAR MSR: Ring 0 and Ring 3 segment selectors */
    /* Kernel CS is at 0x08, User CS is at 0x18 (with +16 for 64-bit) */
    uint64_t star = ((uint64_t)0x0008 << 32) | ((uint64_t)0x0018 << 48);
    arch64_write_msr(MSR_STAR, star);
    
    /* Set up LSTAR MSR: 64-bit syscall handler address */
    arch64_write_msr(MSR_LSTAR, handler_addr);
    
    /* Set up SFMASK: Clear interrupt flag on syscall */
    arch64_write_msr(MSR_SFMASK, 0x200);  /* Clear IF (bit 9) */
    
    /* Enable syscall/sysret via EFER MSR */
    uint64_t efer = arch64_read_msr(0xC0000080);
    efer |= 1;  /* Set SCE (bit 0) - Syscall Enable */
    arch64_write_msr(0xC0000080, efer);
    
    syscall_configured = 1;
}

/**
 * 64-bit system call
 * 
 * This function can use either:
 * 1. The syscall instruction (if arch64_syscall_init was called with valid handler)
 * 2. Software interrupt fallback (int 0x80)
 * 
 * Note: For the syscall instruction to work, the following MSRs must be configured:
 * - STAR (0xC0000081): Contains segment selectors
 * - LSTAR (0xC0000082): Contains the syscall handler entry point
 * - SFMASK (0xC0000084): Contains flags to clear on syscall
 */
arch_ptr_t arch64_syscall(arch_ptr_t syscall_number, arch_ptr_t arg1,
                          arch_ptr_t arg2, arch_ptr_t arg3) {
    arch_ptr_t result;
    
    if (syscall_configured) {
        /* Use hardware syscall instruction */
        __asm__ volatile(
            "syscall"
            : "=a"(result)
            : "a"(syscall_number), "D"(arg1), "S"(arg2), "d"(arg3)
            : "rcx", "r11", "memory"
        );
    } else {
        /* Fallback to software interrupt */
        __asm__ volatile(
            "int $0x80"
            : "=a"(result)
            : "a"(syscall_number), "b"(arg1), "c"(arg2), "d"(arg3)
            : "memory"
        );
    }
    
    return result;
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
