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
    uint32_t current_boot_time_ms;
} boot_opt_config_t;

// Function prototypes
int boot_optimization_init(void);
void boot_enable_parallel_init(void);
void boot_enable_lazy_loading(void);
void boot_enable_boot_cache(void);
void boot_enable_fast_boot(void);
uint32_t boot_get_time_ms(void);

#endif // BOOT_OPTIMIZATION_H
