/**
 * Aurora OS - Memory Allocator Performance Tuning Header
 */

#ifndef MEMORY_OPTIMIZATION_H
#define MEMORY_OPTIMIZATION_H

#include <stdint.h>

typedef struct {
    int slab_allocator_enabled;
    int buddy_system_enabled;
    int defragmentation_enabled;
    int cache_line_alignment;
    uint32_t allocation_pools_count;
} memory_opt_config_t;

typedef struct {
    uint64_t total_allocations;
    uint64_t total_deallocations;
    uint64_t peak_usage;
    uint32_t fragmentation_ratio;
} memory_stats_t;

// Function prototypes
int memory_optimization_init(void);
void memory_enable_slab_allocator(void);
void memory_enable_buddy_system(void);
void memory_enable_defragmentation(void);
void memory_get_stats(memory_stats_t *stats);

#endif // MEMORY_OPTIMIZATION_H
