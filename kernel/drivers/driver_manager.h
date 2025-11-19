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

/* Driver types */
#define DRIVER_TYPE_STORAGE    1
#define DRIVER_TYPE_NETWORK    2
#define DRIVER_TYPE_GRAPHICS   3
#define DRIVER_TYPE_INPUT      4
#define DRIVER_TYPE_AUDIO      5
#define DRIVER_TYPE_USB        6

/* Driver information */
typedef struct {
    const char* name;
    uint8_t type;
    uint8_t status;
    int (*init)(void);
    void (*cleanup)(void);
} driver_info_t;

/* Initialize driver manager */
void driver_manager_init(void);

/* Register drivers */
int driver_register(const driver_info_t* driver);
int driver_unregister(const char* name);

/* Initialize all drivers */
int driver_init_all(void);
int driver_init_by_type(uint8_t type);

/* Get driver status */
int driver_get_status(const char* name);
const driver_info_t* driver_get_info(const char* name);
int driver_get_count(void);

/* List drivers */
void driver_list_all(void);

#endif /* AURORA_DRIVER_MANAGER_H */
