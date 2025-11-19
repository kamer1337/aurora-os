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

/**
 * Initialize driver manager and register built-in drivers
 */
void driver_manager_init(void) {
    driver_count = 0;
    
    /* Register basic hardware drivers */
    static driver_info_t keyboard_driver = {
        .name = "keyboard",
        .type = DRIVER_TYPE_INPUT,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = keyboard_init_wrapper,
        .cleanup = NULL
    };
    driver_register(&keyboard_driver);
    
    static driver_info_t mouse_driver = {
        .name = "mouse",
        .type = DRIVER_TYPE_INPUT,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = mouse_init_wrapper,
        .cleanup = NULL
    };
    driver_register(&mouse_driver);
    
    static driver_info_t timer_driver = {
        .name = "timer",
        .type = DRIVER_TYPE_INPUT,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = timer_init_wrapper,
        .cleanup = NULL
    };
    driver_register(&timer_driver);
    
    static driver_info_t pci_driver = {
        .name = "pci",
        .type = DRIVER_TYPE_STORAGE,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = pci_init_wrapper,
        .cleanup = NULL
    };
    driver_register(&pci_driver);
    
    static driver_info_t storage_driver = {
        .name = "storage",
        .type = DRIVER_TYPE_STORAGE,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = storage_init_wrapper,
        .cleanup = NULL
    };
    driver_register(&storage_driver);
    
    static driver_info_t network_driver = {
        .name = "network",
        .type = DRIVER_TYPE_NETWORK,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = network_init_wrapper,
        .cleanup = NULL
    };
    driver_register(&network_driver);
    
    /* Register graphics drivers */
    static driver_info_t gpu_driver = {
        .name = "gpu",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = gpu_driver_init,
        .cleanup = NULL
    };
    driver_register(&gpu_driver);
    
    static driver_info_t intel_gpu_driver = {
        .name = "intel_hd",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = gpu_intel_hd_init,
        .cleanup = NULL
    };
    driver_register(&intel_gpu_driver);
    
    static driver_info_t nvidia_gpu_driver = {
        .name = "nvidia",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = gpu_nvidia_init,
        .cleanup = NULL
    };
    driver_register(&nvidia_gpu_driver);
    
    static driver_info_t amd_gpu_driver = {
        .name = "amd",
        .type = DRIVER_TYPE_GRAPHICS,
        .status = DRIVER_STATUS_UNINITIALIZED,
        .init = gpu_amd_init,
        .cleanup = NULL
    };
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
 * List all drivers (for debugging)
 */
void driver_list_all(void) {
    /* In a real implementation, would print to console */
    /* For now, this is a no-op */
}
