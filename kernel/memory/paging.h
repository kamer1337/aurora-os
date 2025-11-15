/**
 * Aurora OS - Paging Support Header
 * 
 * Advanced memory management with demand paging
 */

#ifndef AURORA_PAGING_H
#define AURORA_PAGING_H

#include <stdint.h>
#include "memory.h"

/* Page table entry flags */
#define PAGE_PRESENT    0x001
#define PAGE_WRITE      0x002
#define PAGE_USER       0x004
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_COW        0x200  /* Copy-on-write flag (available bit) */

/* Page directory/table entries per directory/table */
#define ENTRIES_PER_TABLE 1024

/* Virtual memory structures */
typedef uint32_t page_directory_t[ENTRIES_PER_TABLE];
typedef uint32_t page_table_t[ENTRIES_PER_TABLE];

/* Memory region structure */
typedef struct vm_region {
    uint32_t start_addr;
    uint32_t end_addr;
    uint32_t flags;
    struct vm_region* next;
} vm_region_t;

/* Page fault information */
typedef struct {
    uint32_t fault_addr;
    uint32_t error_code;
    uint32_t present;
    uint32_t write;
    uint32_t user;
} page_fault_info_t;

/* Paging initialization and management */
void paging_init(void);
void paging_enable(void);
void paging_disable(void);

/* Page directory management */
page_directory_t* paging_create_directory(void);
void paging_destroy_directory(page_directory_t* dir);
void paging_switch_directory(page_directory_t* dir);
page_directory_t* paging_get_current_directory(void);

/* Page mapping functions */
int paging_map_page(page_directory_t* dir, uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
int paging_unmap_page(page_directory_t* dir, uint32_t virt_addr);
uint32_t paging_get_physical_address(page_directory_t* dir, uint32_t virt_addr);

/* Page fault handler */
void page_fault_handler(uint32_t fault_addr, uint32_t error_code);

/* Copy-on-write support */
int paging_mark_cow(page_directory_t* dir, uint32_t virt_addr);
int paging_handle_cow(page_directory_t* dir, uint32_t virt_addr);

/* Page cache for swapping */
void page_cache_init(void);
int page_swap_out(uint32_t virt_addr);
int page_swap_in(uint32_t virt_addr);

#endif /* AURORA_PAGING_H */
