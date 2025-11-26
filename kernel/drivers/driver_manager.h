/**
 * Aurora OS - Hardware Driver Manager
 * 
 * Ensures all basic hardware drivers are initialized
 */

#ifndef AURORA_DRIVER_MANAGER_H
#define AURORA_DRIVER_MANAGER_H

#include <stdint.h>

/* Driver status */
#define DRIVER_STATUS_UNINITIALIZED 0
#define DRIVER_STATUS_INITIALIZED   1
#define DRIVER_STATUS_FAILED        2
#define DRIVER_STATUS_DISABLED      3
#define DRIVER_STATUS_SUSPENDED     4

/* Driver types */
#define DRIVER_TYPE_STORAGE    1
#define DRIVER_TYPE_NETWORK    2
#define DRIVER_TYPE_GRAPHICS   3
#define DRIVER_TYPE_INPUT      4
#define DRIVER_TYPE_AUDIO      5
#define DRIVER_TYPE_USB        6
#define DRIVER_TYPE_BLUETOOTH  7
#define DRIVER_TYPE_SERIAL     8
#define DRIVER_TYPE_POWER      9
#define DRIVER_TYPE_SENSOR     10

/* Driver capability flags */
#define DRIVER_CAP_HOTPLUG     0x01  /* Supports hot-plug */
#define DRIVER_CAP_POWERSAVE   0x02  /* Supports power saving */
#define DRIVER_CAP_DMA         0x04  /* Supports DMA */
#define DRIVER_CAP_INTERRUPT   0x08  /* Uses interrupts */
#define DRIVER_CAP_POLLED      0x10  /* Uses polling */
#define DRIVER_CAP_MULTITHREAD 0x20  /* Thread-safe */

/* Driver configuration structure */
typedef struct {
    uint8_t enabled;           /* Driver enabled flag */
    uint8_t auto_init;         /* Auto-initialize on boot */
    uint8_t power_save_mode;   /* Power saving level (0-3) */
    uint8_t debug_level;       /* Debug verbosity (0-3) */
    uint32_t timeout_ms;       /* Operation timeout in milliseconds */
    uint32_t buffer_size;      /* I/O buffer size */
    uint32_t max_retries;      /* Maximum operation retries */
    void* custom_config;       /* Driver-specific configuration */
} driver_config_t;

/* Driver statistics */
typedef struct {
    uint64_t bytes_read;       /* Total bytes read */
    uint64_t bytes_written;    /* Total bytes written */
    uint64_t operations;       /* Total operations */
    uint64_t errors;           /* Total errors */
    uint64_t interrupts;       /* Total interrupts handled */
    uint32_t avg_latency_us;   /* Average operation latency */
    uint32_t max_latency_us;   /* Maximum operation latency */
    uint64_t uptime_ms;        /* Driver uptime in milliseconds */
} driver_stats_t;

/* Driver information */
typedef struct {
    const char* name;
    const char* description;   /* Human-readable description */
    const char* version;       /* Driver version string */
    uint8_t type;
    uint8_t status;
    uint8_t capabilities;      /* Driver capability flags */
    uint8_t priority;          /* Initialization priority (0=highest) */
    driver_config_t config;    /* Driver configuration */
    driver_stats_t stats;      /* Driver statistics */
    int (*init)(void);
    void (*cleanup)(void);
    int (*suspend)(void);      /* Power management: suspend */
    int (*resume)(void);       /* Power management: resume */
    int (*ioctl)(uint32_t cmd, void* arg); /* Driver-specific control */
} driver_info_t;

/* Initialize driver manager */
void driver_manager_init(void);

/* Register drivers */
int driver_register(const driver_info_t* driver);
int driver_unregister(const char* name);

/* Initialize all drivers */
int driver_init_all(void);
int driver_init_by_type(uint8_t type);
int driver_init_by_name(const char* name);

/* Get driver status */
int driver_get_status(const char* name);
const driver_info_t* driver_get_info(const char* name);
int driver_get_count(void);
const driver_info_t* driver_get_by_index(int index);

/* Driver configuration */
int driver_set_config(const char* name, const driver_config_t* config);
int driver_get_config(const char* name, driver_config_t* config);

/* Driver statistics */
int driver_get_stats(const char* name, driver_stats_t* stats);
void driver_reset_stats(const char* name);

/* Power management */
int driver_suspend(const char* name);
int driver_resume(const char* name);
int driver_suspend_all(void);
int driver_resume_all(void);

/* Driver control */
int driver_enable(const char* name);
int driver_disable(const char* name);
int driver_reload(const char* name);

/* List drivers */
void driver_list_all(void);
int driver_list_by_type(uint8_t type, const driver_info_t** list, int max_count);

#endif /* AURORA_DRIVER_MANAGER_H */
