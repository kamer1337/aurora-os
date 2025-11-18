/**
 * Aurora OS - Hardware Support Header
 */

#ifndef HARDWARE_SUPPORT_H
#define HARDWARE_SUPPORT_H

#include <stdint.h>

typedef struct {
    int intel_optimization;
    int amd_support;
    int multicore_scaling;
    uint32_t detected_cores;
} hardware_support_state_t;

// Function prototypes
int hardware_support_init(void);
int intel_processor_optimization_init(void);
int amd_processor_support_init(void);
int multicore_scaling_init(uint32_t num_cores);
uint32_t hardware_get_core_count(void);

#endif // HARDWARE_SUPPORT_H
