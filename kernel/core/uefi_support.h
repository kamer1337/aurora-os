/**
 * Aurora OS - UEFI Boot Support Header
 */

#ifndef UEFI_SUPPORT_H
#define UEFI_SUPPORT_H

typedef struct {
    int enabled;
    int secure_boot;
    int gop_available;
} uefi_state_t;

// Function prototypes
int uefi_support_init(void);
int uefi_enable_secure_boot(void);
int uefi_is_available(void);

#endif // UEFI_SUPPORT_H
