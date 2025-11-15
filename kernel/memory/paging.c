/**
 * Aurora OS - Paging Implementation
 * 
 * Advanced memory management with demand paging
 */

#include "paging.h"
#include "memory.h"
#include <stddef.h>

/* Current page directory */
static page_directory_t* kernel_directory = NULL;
static page_directory_t* current_directory = NULL;

/* Page swap storage (simplified - in real OS this would be disk) */
#define SWAP_PAGES 256
static struct {
    uint32_t virt_addr;
    uint8_t data[PAGE_SIZE];
    int used;
} swap_storage[SWAP_PAGES];

/**
 * Get page table index from virtual address
 */
static inline uint32_t get_page_table_index(uint32_t virt_addr) {
    return (virt_addr >> 12) & 0x3FF;
}

/**
 * Get page directory index from virtual address
 */
static inline uint32_t get_page_directory_index(uint32_t virt_addr) {
    return (virt_addr >> 22) & 0x3FF;
}

/**
 * Allocate a page table
 */
static page_table_t* alloc_page_table(void) {
    page_table_t* table = (page_table_t*)kmalloc(sizeof(page_table_t));
    if (table) {
        for (uint32_t i = 0; i < ENTRIES_PER_TABLE; i++) {
            (*table)[i] = 0;
        }
    }
    return table;
}

/**
 * Initialize paging subsystem
 */
void paging_init(void) {
    /* Initialize swap storage */
    for (uint32_t i = 0; i < SWAP_PAGES; i++) {
        swap_storage[i].virt_addr = 0;
        swap_storage[i].used = 0;
    }
    
    /* Create kernel page directory */
    kernel_directory = paging_create_directory();
    current_directory = kernel_directory;
    
    /* Identity map first 4MB for kernel */
    for (uint32_t i = 0; i < 0x400000; i += PAGE_SIZE) {
        paging_map_page(kernel_directory, i, i, PAGE_PRESENT | PAGE_WRITE);
    }
}

/**
 * Enable paging
 */
void paging_enable(void) {
    if (!current_directory) {
        return;
    }
    
    /* Load page directory into CR3 */
    __asm__ volatile("mov %0, %%cr3" : : "r"(current_directory));
    
    /* Enable paging by setting bit 31 in CR0 */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

/**
 * Disable paging
 */
void paging_disable(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
}

/**
 * Create new page directory
 */
page_directory_t* paging_create_directory(void) {
    page_directory_t* dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!dir) {
        return NULL;
    }
    
    /* Initialize all entries to not present */
    for (uint32_t i = 0; i < ENTRIES_PER_TABLE; i++) {
        (*dir)[i] = 0;
    }
    
    return dir;
}

/**
 * Destroy page directory
 */
void paging_destroy_directory(page_directory_t* dir) {
    if (!dir || dir == kernel_directory) {
        return;
    }
    
    /* Free all page tables */
    for (uint32_t i = 0; i < ENTRIES_PER_TABLE; i++) {
        if ((*dir)[i] & PAGE_PRESENT) {
            page_table_t* table = (page_table_t*)((*dir)[i] & ~0xFFF);
            kfree(table);
        }
    }
    
    kfree(dir);
}

/**
 * Switch page directory
 */
void paging_switch_directory(page_directory_t* dir) {
    if (!dir) {
        return;
    }
    
    current_directory = dir;
    __asm__ volatile("mov %0, %%cr3" : : "r"(dir));
}

/**
 * Get current page directory
 */
page_directory_t* paging_get_current_directory(void) {
    return current_directory;
}

/**
 * Map virtual page to physical page
 */
int paging_map_page(page_directory_t* dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    if (!dir) {
        return -1;
    }
    
    uint32_t pd_index = get_page_directory_index(virt_addr);
    uint32_t pt_index = get_page_table_index(virt_addr);
    
    /* Get or create page table */
    page_table_t* table;
    if (!((*dir)[pd_index] & PAGE_PRESENT)) {
        table = alloc_page_table();
        if (!table) {
            return -1;
        }
        (*dir)[pd_index] = ((uint32_t)table) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    } else {
        table = (page_table_t*)((*dir)[pd_index] & ~0xFFF);
    }
    
    /* Map page */
    (*table)[pt_index] = (phys_addr & ~0xFFF) | (flags & 0xFFF);
    
    /* Flush TLB for this page */
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
    
    return 0;
}

/**
 * Unmap virtual page
 */
int paging_unmap_page(page_directory_t* dir, uint32_t virt_addr) {
    if (!dir) {
        return -1;
    }
    
    uint32_t pd_index = get_page_directory_index(virt_addr);
    uint32_t pt_index = get_page_table_index(virt_addr);
    
    if (!((*dir)[pd_index] & PAGE_PRESENT)) {
        return -1;
    }
    
    page_table_t* table = (page_table_t*)((*dir)[pd_index] & ~0xFFF);
    (*table)[pt_index] = 0;
    
    /* Flush TLB for this page */
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
    
    return 0;
}

/**
 * Get physical address from virtual address
 */
uint32_t paging_get_physical_address(page_directory_t* dir, uint32_t virt_addr) {
    if (!dir) {
        return 0;
    }
    
    uint32_t pd_index = get_page_directory_index(virt_addr);
    uint32_t pt_index = get_page_table_index(virt_addr);
    
    if (!((*dir)[pd_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_table_t* table = (page_table_t*)((*dir)[pd_index] & ~0xFFF);
    if (!((*table)[pt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    uint32_t phys_addr = (*table)[pt_index] & ~0xFFF;
    uint32_t offset = virt_addr & 0xFFF;
    
    return phys_addr | offset;
}

/**
 * Mark page as copy-on-write
 */
int paging_mark_cow(page_directory_t* dir, uint32_t virt_addr) {
    if (!dir) {
        return -1;
    }
    
    uint32_t pd_index = get_page_directory_index(virt_addr);
    uint32_t pt_index = get_page_table_index(virt_addr);
    
    if (!((*dir)[pd_index] & PAGE_PRESENT)) {
        return -1;
    }
    
    page_table_t* table = (page_table_t*)((*dir)[pd_index] & ~0xFFF);
    if (!((*table)[pt_index] & PAGE_PRESENT)) {
        return -1;
    }
    
    /* Mark as COW and read-only */
    (*table)[pt_index] |= PAGE_COW;
    (*table)[pt_index] &= ~PAGE_WRITE;
    
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
    
    return 0;
}

/**
 * Handle copy-on-write page fault
 */
int paging_handle_cow(page_directory_t* dir, uint32_t virt_addr) {
    if (!dir) {
        return -1;
    }
    
    uint32_t pd_index = get_page_directory_index(virt_addr);
    uint32_t pt_index = get_page_table_index(virt_addr);
    
    if (!((*dir)[pd_index] & PAGE_PRESENT)) {
        return -1;
    }
    
    page_table_t* table = (page_table_t*)((*dir)[pd_index] & ~0xFFF);
    uint32_t pte = (*table)[pt_index];
    
    if (!(pte & PAGE_COW)) {
        return -1;
    }
    
    /* Allocate new physical page */
    void* new_page = vm_alloc(PAGE_SIZE, MEM_KERNEL);
    if (!new_page) {
        return -1;
    }
    
    /* Copy old page to new page */
    uint32_t old_phys = pte & ~0xFFF;
    uint8_t* src = (uint8_t*)old_phys;
    uint8_t* dst = (uint8_t*)new_page;
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        dst[i] = src[i];
    }
    
    /* Update page table entry */
    (*table)[pt_index] = ((uint32_t)new_page) | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    
    __asm__ volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
    
    return 0;
}

/**
 * Page fault handler
 */
void page_fault_handler(uint32_t fault_addr, uint32_t error_code) {
    int present = error_code & 0x1;
    int write = error_code & 0x2;
    int user = error_code & 0x4;
    
    (void)user; /* Unused for now */
    
    /* Handle COW fault */
    if (present && write) {
        if (paging_handle_cow(current_directory, fault_addr) == 0) {
            return; /* COW handled successfully */
        }
    }
    
    /* Handle page not present - swap in */
    if (!present) {
        if (page_swap_in(fault_addr) == 0) {
            return; /* Page swapped in successfully */
        }
    }
    
    /* Unhandled page fault - in real OS would kill process */
}

/**
 * Initialize page cache
 */
void page_cache_init(void) {
    /* Swap storage already initialized in paging_init */
}

/**
 * Swap page out to storage
 */
int page_swap_out(uint32_t virt_addr) {
    /* Find free swap slot */
    int slot = -1;
    for (uint32_t i = 0; i < SWAP_PAGES; i++) {
        if (!swap_storage[i].used) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        return -1; /* No swap space */
    }
    
    /* Get physical address */
    uint32_t phys_addr = paging_get_physical_address(current_directory, virt_addr);
    if (!phys_addr) {
        return -1;
    }
    
    /* Copy page to swap */
    uint8_t* src = (uint8_t*)phys_addr;
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        swap_storage[slot].data[i] = src[i];
    }
    
    swap_storage[slot].virt_addr = virt_addr;
    swap_storage[slot].used = 1;
    
    /* Unmap page */
    paging_unmap_page(current_directory, virt_addr);
    
    return 0;
}

/**
 * Swap page in from storage
 */
int page_swap_in(uint32_t virt_addr) {
    /* Find page in swap */
    int slot = -1;
    for (uint32_t i = 0; i < SWAP_PAGES; i++) {
        if (swap_storage[i].used && swap_storage[i].virt_addr == virt_addr) {
            slot = i;
            break;
        }
    }
    
    if (slot < 0) {
        return -1; /* Page not in swap */
    }
    
    /* Allocate physical page */
    void* phys_page = vm_alloc(PAGE_SIZE, MEM_KERNEL);
    if (!phys_page) {
        return -1;
    }
    
    /* Copy from swap to physical page */
    uint8_t* dst = (uint8_t*)phys_page;
    for (uint32_t i = 0; i < PAGE_SIZE; i++) {
        dst[i] = swap_storage[slot].data[i];
    }
    
    /* Map page */
    paging_map_page(current_directory, virt_addr, (uint32_t)phys_page, 
                    PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    
    /* Free swap slot */
    swap_storage[slot].used = 0;
    
    return 0;
}
