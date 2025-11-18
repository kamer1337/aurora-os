/**
 * Aurora OS - ACPI Power Management Header
 */

#ifndef ACPI_SUPPORT_H
#define ACPI_SUPPORT_H

#include <stdint.h>

typedef struct {
    int initialized;
    int power_management;
    int sleep_states_available;
} acpi_state_t;

// Function prototypes
int acpi_support_init(void);
int acpi_enable_power_management(void);
int acpi_enter_sleep_state(uint32_t state);

#endif // ACPI_SUPPORT_H
