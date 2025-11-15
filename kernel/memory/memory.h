/**
 * Aurora OS - Memory Management Header
 * 
 * Memory allocation and virtual memory management
 */

#ifndef AURORA_MEMORY_H
#define AURORA_MEMORY_H

#include <stdint.h>
#include <stddef.h>

/* Page size (4KB) */
#define PAGE_SIZE 4096

/* Memory allocation flags */
#define MEM_KERNEL  0x01
#define MEM_USER    0x02
#define MEM_ZERO    0x04

/* Memory management functions */
void memory_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

/* Virtual memory functions */
void* vm_alloc(size_t size, uint32_t flags);
void vm_free(void* ptr);

#endif /* AURORA_MEMORY_H */
