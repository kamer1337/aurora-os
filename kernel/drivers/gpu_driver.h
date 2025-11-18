/**
 * Aurora OS - GPU Driver Framework Header
 */

#ifndef GPU_DRIVER_H
#define GPU_DRIVER_H

#include <stdint.h>

typedef enum {
    GPU_VENDOR_UNKNOWN = 0,
    GPU_VENDOR_INTEL = 1,
    GPU_VENDOR_NVIDIA = 2,
    GPU_VENDOR_AMD = 3
} gpu_vendor_t;

typedef struct {
    int initialized;
    gpu_vendor_t vendor;
    int hardware_acceleration;
    uint32_t vram_size_mb;
} gpu_driver_state_t;

// Function prototypes
int gpu_driver_init(void);
int gpu_intel_hd_init(void);
int gpu_nvidia_init(void);
int gpu_amd_init(void);
gpu_vendor_t gpu_get_vendor(void);

#endif // GPU_DRIVER_H
