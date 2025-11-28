/**
 * @file boot_protocol.h
 * @brief Boot Protocol Header for Android and Linux VM Support
 *
 * Defines interfaces for Android boot.img v3/v4 and Linux boot protocol
 */

#ifndef BOOT_PROTOCOL_H
#define BOOT_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
struct AndroidVM;
struct LinuxVM;

/* ============================================================================
 * ANDROID BOOT PROTOCOL
 * ============================================================================ */

/* Boot image versions */
#define BOOT_IMG_VERSION_3      3   /* Android 11+ GKI */
#define BOOT_IMG_VERSION_4      4   /* Android 12+ with signature */

/**
 * Load Android boot image (v3/v4)
 * @param vm Android VM instance
 * @param boot_image Boot image data
 * @param size Boot image size
 * @return 0 on success, -1 on failure
 */
int android_boot_load_kernel(struct AndroidVM* vm, const uint8_t* boot_image, uint32_t size);

/**
 * Setup ARM64 boot protocol registers
 * @param vm Android VM instance
 * @param dtb_addr Device tree blob address
 * @return 0 on success, -1 on failure
 */
int android_boot_setup_arm64(struct AndroidVM* vm, uint32_t dtb_addr);

/**
 * Get parsed boot image version
 * @return Boot image version (3 or 4)
 */
uint32_t android_boot_get_version(void);

/**
 * Check if image is valid Android boot image
 * @param image Image data
 * @param size Image size
 * @return true if valid boot image
 */
bool android_boot_is_valid_image(const uint8_t* image, uint32_t size);

/* ============================================================================
 * LINUX BOOT PROTOCOL
 * ============================================================================ */

/* Linux boot protocol versions */
#define LINUX_BOOT_PROTOCOL_2_00    0x0200  /* Old protocol */
#define LINUX_BOOT_PROTOCOL_2_06    0x0206  /* KASLR support */
#define LINUX_BOOT_PROTOCOL_2_15    0x020F  /* Latest */

/**
 * Load Linux kernel with boot protocol
 * @param vm Linux VM instance
 * @param bzimage Kernel image data
 * @param size Image size
 * @return 0 on success, -1 on failure
 */
int linux_boot_load_kernel(struct LinuxVM* vm, const uint8_t* bzimage, uint32_t size);

/**
 * Setup Linux boot parameters
 * @param vm Linux VM instance
 * @param cmdline Kernel command line
 * @return 0 on success, -1 on failure
 */
int linux_boot_setup_params(struct LinuxVM* vm, const char* cmdline);

/**
 * Setup CPU state for Linux boot
 * @param vm Linux VM instance
 * @return 0 on success, -1 on failure
 */
int linux_boot_setup_cpu(struct LinuxVM* vm);

/**
 * Get Linux boot protocol version
 * @return Boot protocol version
 */
uint16_t linux_boot_get_version(void);

/**
 * Check if image is valid Linux bzImage
 * @param image Image data
 * @param size Image size
 * @return true if valid bzImage
 */
bool linux_boot_is_bzimage(const uint8_t* image, uint32_t size);

/* ============================================================================
 * COMMON BOOT UTILITIES
 * ============================================================================ */

/**
 * Calculate CRC32 checksum
 * @param data Data to checksum
 * @param len Data length
 * @return CRC32 value
 */
uint32_t boot_crc32(const uint8_t* data, uint32_t len);

/**
 * Get boot protocol version string
 * @return Version string
 */
const char* boot_protocol_get_version(void);

/**
 * Reset all boot state
 */
void boot_protocol_reset(void);

/* ============================================================================
 * EXTENDED LINUX BOOT PROTOCOL FUNCTIONS
 * ============================================================================ */

/**
 * Add E820 memory map entry
 * @param addr Start address
 * @param size Size in bytes
 * @param type Memory type (E820_RAM, E820_RESERVED, etc.)
 * @return Entry index or -1 on error
 */
int linux_boot_add_e820_entry(uint64_t addr, uint64_t size, uint32_t type);

/**
 * Get E820 memory map entry count
 * @return Number of E820 entries
 */
uint8_t linux_boot_get_e820_count(void);

/**
 * Get E820 memory map entry by index
 * @param idx Entry index
 * @param addr Output start address
 * @param size Output size
 * @param type Output type
 * @return 0 on success, -1 on error
 */
int linux_boot_get_e820_entry(uint8_t idx, uint64_t* addr, uint64_t* size, uint32_t* type);

/**
 * Setup KASLR with provided entropy
 * @param vm Linux VM instance
 * @param entropy Random entropy value
 * @return 0 on success, -1 on error
 */
int linux_boot_setup_kaslr(struct LinuxVM* vm, uint32_t entropy);

/**
 * Get kernel load address (with KASLR if enabled)
 * @return Kernel load address
 */
uint32_t linux_boot_get_kernel_addr(void);

/**
 * Set initrd address and size
 * @param vm Linux VM instance
 * @param addr Initrd address
 * @param size Initrd size
 * @return 0 on success, -1 on error
 */
int linux_boot_set_initrd(struct LinuxVM* vm, uint32_t addr, uint32_t size);

/**
 * Get initrd information
 * @param addr Output address
 * @param size Output size
 * @return 0 on success
 */
int linux_boot_get_initrd(uint32_t* addr, uint32_t* size);

/**
 * Set command line for Linux boot
 * @param cmdline Command line string
 * @return 0 on success, -1 on error
 */
int linux_boot_set_cmdline(const char* cmdline);

/**
 * Get command line
 * @return Command line string
 */
const char* linux_boot_get_cmdline(void);

/**
 * Append to command line
 * @param append String to append
 * @return 0 on success, -1 on error
 */
int linux_boot_append_cmdline(const char* append);

/**
 * Setup EFI boot for UEFI systems
 * @param vm Linux VM instance
 * @param system_table EFI system table address
 * @return 0 on success, -1 on error
 */
int linux_boot_setup_efi(struct LinuxVM* vm, uint64_t system_table);

/**
 * Check if EFI boot is configured
 * @return true if EFI boot is configured
 */
bool linux_boot_is_efi(void);

/**
 * Get EFI system table address
 * @return EFI system table address
 */
uint64_t linux_boot_get_efi_system_table(void);

/* ============================================================================
 * EXTENDED ANDROID BOOT PROTOCOL FUNCTIONS
 * ============================================================================ */

/**
 * Load vendor boot image
 * @param vm Android VM instance
 * @param vendor_image Vendor boot image data
 * @param size Vendor image size
 * @return 0 on success, -1 on error
 */
int android_boot_load_vendor(struct AndroidVM* vm, const uint8_t* vendor_image, uint32_t size);

/**
 * Get Android boot command line from protocol state
 * @return Command line string
 */
const char* boot_protocol_get_android_cmdline(void);

/**
 * Set Android boot command line in protocol state
 * @param cmdline Command line string
 * @return 0 on success, -1 on error
 */
int boot_protocol_set_android_cmdline(const char* cmdline);

/**
 * Append to Android boot command line in protocol state
 * @param append String to append
 * @return 0 on success, -1 on error
 */
int boot_protocol_append_android_cmdline(const char* append);

/**
 * Get DTB information from protocol state
 * @param addr Output DTB address
 * @param size Output DTB size
 * @return 0 on success
 */
int boot_protocol_get_android_dtb(uint32_t* addr, uint32_t* size);

/**
 * Verify Android boot image signature (AVB)
 * @param image Boot image data
 * @param size Image size
 * @param key Verification key
 * @param key_size Key size
 * @return 0 if signature valid, -1 on error
 */
int boot_protocol_verify_android_signature(const uint8_t* image, uint32_t size, const uint8_t* key, uint32_t key_size);

#endif /* BOOT_PROTOCOL_H */
