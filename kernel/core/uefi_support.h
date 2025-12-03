/**
 * Aurora OS - UEFI Boot Support Header
 */

#ifndef UEFI_SUPPORT_H
#define UEFI_SUPPORT_H

#include <stdint.h>

/* UEFI secure boot key types */
typedef enum {
    UEFI_KEY_PK = 0,    /* Platform Key */
    UEFI_KEY_KEK = 1,   /* Key Exchange Key */
    UEFI_KEY_DB = 2,    /* Authorized Signature Database */
    UEFI_KEY_DBX = 3    /* Forbidden Signature Database */
} uefi_key_type_t;

/* UEFI secure boot status */
typedef enum {
    UEFI_SECUREBOOT_DISABLED = 0,
    UEFI_SECUREBOOT_SETUP_MODE = 1,
    UEFI_SECUREBOOT_USER_MODE = 2,
    UEFI_SECUREBOOT_AUDIT_MODE = 3,
    UEFI_SECUREBOOT_DEPLOYED_MODE = 4
} uefi_secureboot_status_t;

typedef struct {
    int enabled;
    int secure_boot;
    int gop_available;
    uefi_secureboot_status_t secureboot_status;
    uint32_t platform_key_present;
    uint32_t kek_count;
    uint32_t db_count;
    uint32_t dbx_count;
    uint64_t system_table_addr;
    uint64_t runtime_services_addr;
} uefi_state_t;

/* UEFI certificate structure */
typedef struct {
    uint8_t signature[256];
    uint32_t signature_size;
    uint8_t owner_guid[16];
    uint32_t cert_type;
} uefi_certificate_t;

// Function prototypes
int uefi_support_init(void);
int uefi_enable_secure_boot(void);
int uefi_is_available(void);
int uefi_verify_signature(const uint8_t* data, uint32_t data_size, 
                          const uint8_t* signature, uint32_t sig_size);
int uefi_load_certificates(void);
int uefi_check_secureboot_status(void);
int uefi_enroll_key(uefi_key_type_t key_type, const uefi_certificate_t* cert);
int uefi_verify_bootloader(const void* bootloader, uint32_t size);
int uefi_setup_memory_protection(void);

#endif // UEFI_SUPPORT_H
