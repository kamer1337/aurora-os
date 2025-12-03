/**
 * Aurora OS - Boot Time Reduction Header
 */

#ifndef BOOT_OPTIMIZATION_H
#define BOOT_OPTIMIZATION_H

#include <stdint.h>

typedef struct {
    int parallel_init;
    int lazy_loading;
    int boot_cache;
    int fast_boot_mode;
    int aggressive_preload;
    int early_kmsg_suppress;
    int skip_initramfs_unpack;
    uint32_t current_boot_time_ms;
    uint32_t target_boot_time_ms;
} boot_opt_config_t;

// Function prototypes
int boot_optimization_init(void);
void boot_enable_parallel_init(void);
void boot_enable_lazy_loading(void);
void boot_enable_boot_cache(void);
void boot_enable_fast_boot(void);
void boot_enable_aggressive_optimization(void);
void boot_skip_delays(void);
uint32_t boot_get_time_ms(void);
int boot_measure_time(void);

#endif // BOOT_OPTIMIZATION_H
