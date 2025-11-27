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

#endif /* BOOT_PROTOCOL_H */
