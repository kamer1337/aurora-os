/**
 * Aurora OS - Hardware Driver Manager Implementation
 */

#include "driver_manager.h"
#include <stddef.h>

/* Maximum registered drivers */
#define MAX_DRIVERS 64

/* Global driver registry */
static driver_info_t drivers[MAX_DRIVERS];
static int driver_count = 0;

/* Helper function to find driver by name */
static int find_driver_index(const char* name) {
    for (int i = 0; i < driver_count; i++) {
        const char* driver_name = drivers[i].name;
        const char* search_name = name;
        
        /* Compare strings */
        while (*driver_name && *search_name) {
            if (*driver_name != *search_name) {
                break;
            }
            driver_name++;
            search_name++;
        }
        
        if (*driver_name == *search_name) {
            return i;
        }
    }
    return -1;
}

/* Forward declarations for driver init functions */
extern void keyboard_init(void);
extern void mouse_init(void);
extern void timer_init(void);
extern void pci_init(void);
extern void storage_init(void);
extern void network_init(void);
extern int gpu_driver_init(void);
extern int gpu_intel_hd_init(void);
extern int gpu_nvidia_init(void);
extern int gpu_amd_init(void);

/* Wrapper functions for drivers without return values */
static int keyboard_init_wrapper(void) {
    keyboard_init();
    return 0;
}

static int mouse_init_wrapper(void) {
    mouse_init();
    return 0;
}

static int timer_init_wrapper(void) {
    timer_init();
    return 0;
}

static int pci_init_wrapper(void) {
    pci_init();
    return 0;
}

static int storage_init_wrapper(void) {
    storage_init();
    return 0;
}

static int network_init_wrapper(void) {
    network_init();
    return 0;
}

/* Default driver configuration */
static driver_config_t default_driver_config = {
    .enabled = 1,
    .auto_init = 1,
    .power_save_mode = 0,
    .debug_level = 0,
    .timeout_ms = 5000,
    .buffer_size = 4096,
    .max_retries = 3,
    .custom_config = NULL
};

/**
 * Initialize driver manager and register built-in drivers
 */
void driver_manager_init(void) {
    driver_count = 0;
    
    /* Register basic hardware drivers */
    static driver_info_t keyboard_driver = {
        .name = "keyboard",
        .description = "PS/2 Keyboard Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_INPUT,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_INTERRUPT,
        .priority = 10,
        .init = keyboard_init_wrapper,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    keyboard_driver.config = default_driver_config;
    driver_register(&keyboard_driver);
    
    static driver_info_t mouse_driver = {
        .name = "mouse",
        .description = "PS/2 Mouse Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_INPUT,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_INTERRUPT,
        .priority = 10,
        .init = mouse_init_wrapper,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    mouse_driver.config = default_driver_config;
    driver_register(&mouse_driver);
    
    static driver_info_t timer_driver = {
        .name = "timer",
        .description = "System Timer Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_INPUT,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_INTERRUPT,
        .priority = 1,  /* High priority */
        .init = timer_init_wrapper,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    timer_driver.config = default_driver_config;
    driver_register(&timer_driver);
    
    static driver_info_t pci_driver = {
        .name = "pci",
        .description = "PCI Bus Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_STORAGE,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_HOTPLUG,
        .priority = 5,
        .init = pci_init_wrapper,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    pci_driver.config = default_driver_config;
    driver_register(&pci_driver);
    
    static driver_info_t storage_driver = {
        .name = "storage",
        .description = "Storage Controller Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_STORAGE,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_DMA | DRIVER_CAP_INTERRUPT,
        .priority = 20,
        .init = storage_init_wrapper,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    storage_driver.config = default_driver_config;
    driver_register(&storage_driver);
    
    static driver_info_t network_driver = {
        .name = "network",
        .description = "Network Interface Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_NETWORK,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_DMA | DRIVER_CAP_INTERRUPT | DRIVER_CAP_POWERSAVE,
        .priority = 30,
        .init = network_init_wrapper,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    network_driver.config = default_driver_config;
    driver_register(&network_driver);
    
    /* Register graphics drivers */
    static driver_info_t gpu_driver = {
        .name = "gpu",
        .description = "Generic GPU Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_DMA | DRIVER_CAP_POWERSAVE,
        .priority = 15,
        .init = gpu_driver_init,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    gpu_driver.config = default_driver_config;
    driver_register(&gpu_driver);
    
    static driver_info_t intel_gpu_driver = {
        .name = "intel_hd",
        .description = "Intel HD Graphics Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_DMA | DRIVER_CAP_POWERSAVE,
        .priority = 15,
        .init = gpu_intel_hd_init,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    intel_gpu_driver.config = default_driver_config;
    driver_register(&intel_gpu_driver);
    
    static driver_info_t nvidia_gpu_driver = {
        .name = "nvidia",
        .description = "NVIDIA GPU Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_DMA | DRIVER_CAP_POWERSAVE,
        .priority = 15,
        .init = gpu_nvidia_init,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    nvidia_gpu_driver.config = default_driver_config;
    driver_register(&nvidia_gpu_driver);
    
    static driver_info_t amd_gpu_driver = {
        .name = "amd",
        .description = "AMD GPU Driver",
        .version = "1.0.0",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .capabilities = DRIVER_CAP_DMA | DRIVER_CAP_POWERSAVE,
        .priority = 15,
        .init = gpu_amd_init,
        .cleanup = NULL,
        .suspend = NULL,
        .resume = NULL,
        .ioctl = NULL
    };
    amd_gpu_driver.config = default_driver_config;
    driver_register(&amd_gpu_driver);
}

/**
 * Register a driver
 */
int driver_register(const driver_info_t* driver) {
    if (driver == NULL || driver_count >= MAX_DRIVERS) {
        return -1;
    }
    
    /* Check if driver already registered */
    if (find_driver_index(driver->name) >= 0) {
        return -2;  /* Already registered */
    }
    
    /* Copy driver info */
    drivers[driver_count] = *driver;
    driver_count++;
    
    return 0;
}

/**
 * Unregister a driver
 */
int driver_unregister(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;  /* Not found */
    }
    
    /* Shift drivers down */
    for (int i = index; i < driver_count - 1; i++) {
        drivers[i] = drivers[i + 1];
    }
    
    driver_count--;
    return 0;
}

/**
 * Initialize all drivers
 */
int driver_init_all(void) {
    int failed_count = 0;
    
    for (int i = 0; i < driver_count; i++) {
        driver_info_t* driver = &drivers[i];
        
        if (driver->status == DRIVER_STATUS_INITIALIZED) {
            continue;  /* Already initialized */
        }
        
        if (driver->init != NULL) {
            int result = driver->init();
            if (result == 0) {
                driver->status = DRIVER_STATUS_INITIALIZED;
            } else {
                driver->status = DRIVER_STATUS_FAILED;
                failed_count++;
            }
        }
    }
    
    return failed_count;
}

/**
 * Initialize drivers by type
 */
int driver_init_by_type(uint8_t type) {
    int failed_count = 0;
    
    for (int i = 0; i < driver_count; i++) {
        driver_info_t* driver = &drivers[i];
        
        if (driver->type != type) {
            continue;
        }
        
        if (driver->status == DRIVER_STATUS_INITIALIZED) {
            continue;  /* Already initialized */
        }
        
        if (driver->init != NULL) {
            int result = driver->init();
            if (result == 0) {
                driver->status = DRIVER_STATUS_INITIALIZED;
            } else {
                driver->status = DRIVER_STATUS_FAILED;
                failed_count++;
            }
        }
    }
    
    return failed_count;
}

/**
 * Get driver status
 */
int driver_get_status(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;  /* Not found */
    }
    
    return drivers[index].status;
}

/**
 * Get driver information
 */
const driver_info_t* driver_get_info(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return NULL;
    }
    
    return &drivers[index];
}

/**
 * Get driver count
 */
int driver_get_count(void) {
    return driver_count;
}

/**
 * Get driver by index
 */
const driver_info_t* driver_get_by_index(int index) {
    if (index < 0 || index >= driver_count) {
        return NULL;
    }
    return &drivers[index];
}

/**
 * Initialize a specific driver by name
 */
int driver_init_by_name(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;  /* Not found */
    }
    
    driver_info_t* driver = &drivers[index];
    
    if (driver->status == DRIVER_STATUS_INITIALIZED) {
        return 0;  /* Already initialized */
    }
    
    if (!driver->config.enabled) {
        return -2;  /* Driver disabled */
    }
    
    if (driver->init != NULL) {
        int result = driver->init();
        if (result == 0) {
            driver->status = DRIVER_STATUS_INITIALIZED;
        } else {
            driver->status = DRIVER_STATUS_FAILED;
            return -3;
        }
    }
    
    return 0;
}

/**
 * Set driver configuration
 */
int driver_set_config(const char* name, const driver_config_t* config) {
    if (!name || !config) {
        return -1;
    }
    
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    drivers[index].config = *config;
    return 0;
}

/**
 * Get driver configuration
 */
int driver_get_config(const char* name, driver_config_t* config) {
    if (!name || !config) {
        return -1;
    }
    
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    *config = drivers[index].config;
    return 0;
}

/**
 * Get driver statistics
 */
int driver_get_stats(const char* name, driver_stats_t* stats) {
    if (!name || !stats) {
        return -1;
    }
    
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    *stats = drivers[index].stats;
    return 0;
}

/**
 * Reset driver statistics
 */
void driver_reset_stats(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return;
    }
    
    drivers[index].stats.bytes_read = 0;
    drivers[index].stats.bytes_written = 0;
    drivers[index].stats.operations = 0;
    drivers[index].stats.errors = 0;
    drivers[index].stats.interrupts = 0;
    drivers[index].stats.avg_latency_us = 0;
    drivers[index].stats.max_latency_us = 0;
}

/**
 * Suspend a driver
 */
int driver_suspend(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    driver_info_t* driver = &drivers[index];
    
    if (driver->status != DRIVER_STATUS_INITIALIZED) {
        return -1;  /* Not initialized */
    }
    
    if (driver->suspend != NULL) {
        int result = driver->suspend();
        if (result == 0) {
            driver->status = DRIVER_STATUS_SUSPENDED;
        }
        return result;
    }
    
    /* No suspend function - mark as suspended anyway */
    driver->status = DRIVER_STATUS_SUSPENDED;
    return 0;
}

/**
 * Resume a driver
 */
int driver_resume(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    driver_info_t* driver = &drivers[index];
    
    if (driver->status != DRIVER_STATUS_SUSPENDED) {
        return -1;  /* Not suspended */
    }
    
    if (driver->resume != NULL) {
        int result = driver->resume();
        if (result == 0) {
            driver->status = DRIVER_STATUS_INITIALIZED;
        }
        return result;
    }
    
    /* No resume function - mark as initialized anyway */
    driver->status = DRIVER_STATUS_INITIALIZED;
    return 0;
}

/**
 * Suspend all drivers
 */
int driver_suspend_all(void) {
    int failed_count = 0;
    
    /* Suspend in reverse priority order */
    for (int i = driver_count - 1; i >= 0; i--) {
        if (drivers[i].status == DRIVER_STATUS_INITIALIZED) {
            if (driver_suspend(drivers[i].name) != 0) {
                failed_count++;
            }
        }
    }
    
    return failed_count;
}

/**
 * Resume all drivers
 */
int driver_resume_all(void) {
    int failed_count = 0;
    
    /* Resume in priority order */
    for (int i = 0; i < driver_count; i++) {
        if (drivers[i].status == DRIVER_STATUS_SUSPENDED) {
            if (driver_resume(drivers[i].name) != 0) {
                failed_count++;
            }
        }
    }
    
    return failed_count;
}

/**
 * Enable a driver
 */
int driver_enable(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    drivers[index].config.enabled = 1;
    
    /* If auto_init is set and driver is not initialized, initialize it */
    if (drivers[index].config.auto_init && 
        drivers[index].status == DRIVER_STATUS_UNINITIALIZED) {
        return driver_init_by_name(name);
    }
    
    return 0;
}

/**
 * Disable a driver
 */
int driver_disable(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    driver_info_t* driver = &drivers[index];
    
    /* Cleanup if initialized */
    if (driver->status == DRIVER_STATUS_INITIALIZED && driver->cleanup != NULL) {
        driver->cleanup();
    }
    
    driver->config.enabled = 0;
    driver->status = DRIVER_STATUS_DISABLED;
    return 0;
}

/**
 * Reload a driver (cleanup and reinitialize)
 */
int driver_reload(const char* name) {
    int index = find_driver_index(name);
    if (index < 0) {
        return -1;
    }
    
    driver_info_t* driver = &drivers[index];
    
    /* Cleanup if initialized */
    if (driver->status == DRIVER_STATUS_INITIALIZED && driver->cleanup != NULL) {
        driver->cleanup();
    }
    
    driver->status = DRIVER_STATUS_UNINITIALIZED;
    
    /* Reinitialize */
    return driver_init_by_name(name);
}

/**
 * List all drivers (for debugging)
 */
void driver_list_all(void) {
    /* In a real implementation, would print to console */
    /* For now, this is a no-op */
}

/**
 * List drivers by type
 */
int driver_list_by_type(uint8_t type, const driver_info_t** list, int max_count) {
    if (!list || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < driver_count && count < max_count; i++) {
        if (drivers[i].type == type) {
            list[count++] = &drivers[i];
        }
    }
    
    return count;
}
