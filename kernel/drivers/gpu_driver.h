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

/* GPU power states */
typedef enum {
    GPU_POWER_OFF = 0,
    GPU_POWER_STANDBY = 1,
    GPU_POWER_IDLE = 2,
    GPU_POWER_ACTIVE = 3,
    GPU_POWER_BOOST = 4
} gpu_power_state_t;

/* GPU feature flags */
#define GPU_FEATURE_2D_ACCEL     0x0001
#define GPU_FEATURE_3D_ACCEL     0x0002
#define GPU_FEATURE_VIDEO_DECODE 0x0004
#define GPU_FEATURE_VIDEO_ENCODE 0x0008
#define GPU_FEATURE_COMPUTE      0x0010
#define GPU_FEATURE_RAYTRACING   0x0020
#define GPU_FEATURE_TENSOR_CORES 0x0040
#define GPU_FEATURE_FREESYNC     0x0080
#define GPU_FEATURE_GSYNC        0x0100
#define GPU_FEATURE_HDR          0x0200
#define GPU_FEATURE_OPENGL       0x0400
#define GPU_FEATURE_VULKAN       0x0800
#define GPU_FEATURE_4K           0x1000
#define GPU_FEATURE_8K           0x2000

/* GPU configuration */
typedef struct {
    uint8_t vsync_enabled;         /* Enable vertical sync */
    uint8_t triple_buffering;      /* Enable triple buffering */
    uint8_t anti_aliasing;         /* Anti-aliasing level (0-16) */
    uint8_t anisotropic_filter;    /* Anisotropic filtering level (0-16) */
    uint8_t texture_quality;       /* Texture quality (0=low, 1=medium, 2=high, 3=ultra) */
    uint8_t shader_quality;        /* Shader quality (0=low, 1=medium, 2=high) */
    uint8_t power_mode;            /* Power mode (0=balanced, 1=performance, 2=power_save) */
    uint8_t max_frame_rate;        /* Maximum frame rate (0=unlimited) */
    uint32_t vram_budget_mb;       /* VRAM budget in MB */
} gpu_config_t;

/* GPU information */
typedef struct {
    char device_name[64];          /* GPU device name */
    char driver_version[32];       /* Driver version string */
    gpu_vendor_t vendor;           /* GPU vendor */
    uint32_t device_id;            /* PCI device ID */
    uint32_t vram_size_mb;         /* Total VRAM in MB */
    uint32_t vram_used_mb;         /* Used VRAM in MB */
    uint32_t core_clock_mhz;       /* Core clock speed */
    uint32_t memory_clock_mhz;     /* Memory clock speed */
    uint16_t features;             /* Feature flags */
    uint8_t num_display_outputs;   /* Number of display outputs */
    gpu_power_state_t power_state; /* Current power state */
} gpu_info_t;

typedef struct {
    int initialized;
    gpu_vendor_t vendor;
    int hardware_acceleration;
    uint32_t vram_size_mb;
    gpu_config_t config;
    gpu_info_t info;
} gpu_driver_state_t;

/* Core functions */
int gpu_driver_init(void);
int gpu_intel_hd_init(void);
int gpu_nvidia_init(void);
int gpu_amd_init(void);
void gpu_driver_shutdown(void);

/* Vendor and info */
gpu_vendor_t gpu_get_vendor(void);
int gpu_get_info(gpu_info_t* info);

/* Configuration */
int gpu_set_config(const gpu_config_t* config);
int gpu_get_config(gpu_config_t* config);

/* Power management */
int gpu_set_power_state(gpu_power_state_t state);
gpu_power_state_t gpu_get_power_state(void);

/* Memory management */
uint32_t gpu_get_vram_free(void);
int gpu_allocate_vram(uint32_t size_bytes, uint32_t* handle);
int gpu_free_vram(uint32_t handle);

/* Display control */
int gpu_set_display_mode(uint32_t width, uint32_t height, uint32_t refresh_rate);
int gpu_get_display_mode(uint32_t* width, uint32_t* height, uint32_t* refresh_rate);

/* OpenGL/Vulkan support */
int gpu_opengl_init(void);
int gpu_vulkan_init(void);
int gpu_create_context(void);
int gpu_destroy_context(void);
int gpu_submit_command_buffer(void* commands, uint32_t size);

/* 4K/8K display support */
int gpu_enable_4k_mode(void);
int gpu_enable_8k_mode(void);
int gpu_check_uhd_support(void);

#endif // GPU_DRIVER_H
