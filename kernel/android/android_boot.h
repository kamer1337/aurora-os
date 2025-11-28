/**
 * Aurora OS - Android Boot Protocol Implementation
 * 
 * Supports boot.img v0, v1, v2, v3, and v4 formats
 * Reference: https://source.android.com/docs/core/architecture/bootloader/boot-image-header
 */

#ifndef AURORA_ANDROID_BOOT_H
#define AURORA_ANDROID_BOOT_H

#include <stdint.h>
#include <stddef.h>

/* Boot image magic string */
#define BOOT_MAGIC "ANDROID!"
#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512
#define BOOT_EXTRA_ARGS_SIZE 1024

/* Boot image header versions */
#define BOOT_HEADER_VERSION_ZERO 0
#define BOOT_HEADER_VERSION_ONE 1
#define BOOT_HEADER_VERSION_TWO 2
#define BOOT_HEADER_VERSION_THREE 3
#define BOOT_HEADER_VERSION_FOUR 4

/* Vendor boot image magic */
#define VENDOR_BOOT_MAGIC "VNDRBOOT"
#define VENDOR_BOOT_MAGIC_SIZE 8
#define VENDOR_BOOT_ARGS_SIZE 2048
#define VENDOR_BOOT_NAME_SIZE 16

/* Boot ramdisk types for v4 */
#define VENDOR_RAMDISK_TYPE_NONE 0
#define VENDOR_RAMDISK_TYPE_PLATFORM 1
#define VENDOR_RAMDISK_TYPE_RECOVERY 2
#define VENDOR_RAMDISK_TYPE_DLKM 3

/* Boot image signature types */
#define BOOT_SIGNATURE_NONE 0
#define BOOT_SIGNATURE_AVB 1
#define BOOT_SIGNATURE_VERITY 2

/**
 * Boot image header v0/v1/v2 (legacy)
 * Used for Android 8 and earlier, partially for Android 9
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[BOOT_MAGIC_SIZE];
    uint32_t kernel_size;
    uint32_t kernel_addr;
    uint32_t ramdisk_size;
    uint32_t ramdisk_addr;
    uint32_t second_size;           /* Second stage bootloader size */
    uint32_t second_addr;
    uint32_t tags_addr;
    uint32_t page_size;
    uint32_t header_version;        /* 0 for v0, 1 for v1, 2 for v2 */
    uint32_t os_version;            /* (major << 14) | (minor << 7) | patch */
    uint8_t name[BOOT_NAME_SIZE];
    uint8_t cmdline[BOOT_ARGS_SIZE];
    uint32_t id[8];                 /* SHA-1 hash of kernel + ramdisk */
    uint8_t extra_cmdline[BOOT_EXTRA_ARGS_SIZE];
    /* v1+ fields */
    uint32_t recovery_dtbo_size;    /* v1+ */
    uint64_t recovery_dtbo_offset;  /* v1+ */
    uint32_t header_size;           /* v1+ */
    /* v2+ fields */
    uint32_t dtb_size;              /* v2+ */
    uint64_t dtb_addr;              /* v2+ */
} boot_img_hdr_v0_t;

/**
 * Boot image header v3
 * Used for Android 11+
 * Kernel/ramdisk are page-aligned, simpler structure
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[BOOT_MAGIC_SIZE];
    uint32_t kernel_size;
    uint32_t ramdisk_size;
    uint32_t os_version;
    uint32_t header_size;
    uint32_t reserved[4];
    uint32_t header_version;        /* Must be 3 */
    uint8_t cmdline[BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE];
} boot_img_hdr_v3_t;

/**
 * Boot image header v4
 * Used for Android 12+
 * Adds signature size field
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[BOOT_MAGIC_SIZE];
    uint32_t kernel_size;
    uint32_t ramdisk_size;
    uint32_t os_version;
    uint32_t header_size;
    uint32_t reserved[4];
    uint32_t header_version;        /* Must be 4 */
    uint8_t cmdline[BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE];
    uint32_t signature_size;        /* v4 only - Boot signature size */
} boot_img_hdr_v4_t;

/**
 * Vendor boot image header v3
 * Separates vendor-specific boot data
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[VENDOR_BOOT_MAGIC_SIZE];
    uint32_t header_version;
    uint32_t page_size;
    uint32_t kernel_addr;
    uint32_t ramdisk_addr;
    uint32_t vendor_ramdisk_size;
    uint8_t cmdline[VENDOR_BOOT_ARGS_SIZE];
    uint32_t tags_addr;
    uint8_t name[VENDOR_BOOT_NAME_SIZE];
    uint32_t header_size;
    uint32_t dtb_size;
    uint64_t dtb_addr;
} vendor_boot_img_hdr_v3_t;

/**
 * Vendor boot image header v4
 * Adds vendor ramdisk table support
 */
typedef struct __attribute__((packed)) {
    uint8_t magic[VENDOR_BOOT_MAGIC_SIZE];
    uint32_t header_version;
    uint32_t page_size;
    uint32_t kernel_addr;
    uint32_t ramdisk_addr;
    uint32_t vendor_ramdisk_size;
    uint8_t cmdline[VENDOR_BOOT_ARGS_SIZE];
    uint32_t tags_addr;
    uint8_t name[VENDOR_BOOT_NAME_SIZE];
    uint32_t header_size;
    uint32_t dtb_size;
    uint64_t dtb_addr;
    uint32_t vendor_ramdisk_table_size;
    uint32_t vendor_ramdisk_table_entry_num;
    uint32_t vendor_ramdisk_table_entry_size;
    uint32_t bootconfig_size;
} vendor_boot_img_hdr_v4_t;

/**
 * Vendor ramdisk table entry (v4)
 */
typedef struct __attribute__((packed)) {
    uint32_t ramdisk_size;
    uint32_t ramdisk_offset;
    uint32_t ramdisk_type;
    uint8_t ramdisk_name[32];
    uint8_t board_id[16 * sizeof(uint32_t)];
} vendor_ramdisk_table_entry_t;

/**
 * Unified boot image information structure
 * Populated after parsing any boot image version
 */
typedef struct {
    uint32_t header_version;
    uint32_t page_size;
    
    /* Kernel info */
    uint32_t kernel_size;
    uint64_t kernel_addr;
    void* kernel_data;
    
    /* Ramdisk info */
    uint32_t ramdisk_size;
    uint64_t ramdisk_addr;
    void* ramdisk_data;
    
    /* Second stage bootloader (v0-v2) */
    uint32_t second_size;
    uint64_t second_addr;
    void* second_data;
    
    /* Device tree blob */
    uint32_t dtb_size;
    uint64_t dtb_addr;
    void* dtb_data;
    
    /* Recovery DTBO (v1-v2) */
    uint32_t recovery_dtbo_size;
    uint64_t recovery_dtbo_offset;
    void* recovery_dtbo_data;
    
    /* Boot signature (v4) */
    uint32_t signature_size;
    void* signature_data;
    
    /* Command line */
    char cmdline[BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE + VENDOR_BOOT_ARGS_SIZE];
    
    /* OS version */
    uint32_t os_version_major;
    uint32_t os_version_minor;
    uint32_t os_version_patch;
    uint32_t os_patch_level_year;
    uint32_t os_patch_level_month;
    
    /* Device name */
    char name[BOOT_NAME_SIZE + 1];
    
    /* SHA-1 hash (v0-v2) */
    uint32_t id[8];
    
    /* Vendor ramdisks (v4) */
    uint32_t vendor_ramdisk_count;
    vendor_ramdisk_table_entry_t* vendor_ramdisks;
    
    /* Bootconfig (v4) */
    uint32_t bootconfig_size;
    char* bootconfig_data;
    
    /* Parsing status */
    int valid;
    int has_vendor_boot;
} android_boot_info_t;

/**
 * Boot image parsing status codes
 */
#define BOOT_PARSE_SUCCESS 0
#define BOOT_PARSE_INVALID_MAGIC -1
#define BOOT_PARSE_UNSUPPORTED_VERSION -2
#define BOOT_PARSE_INVALID_SIZE -3
#define BOOT_PARSE_MEMORY_ERROR -4
#define BOOT_PARSE_CHECKSUM_ERROR -5

/**
 * Initialize Android boot subsystem
 * @return 0 on success, negative error code on failure
 */
int android_boot_init(void);

/**
 * Parse boot.img header and validate format
 * @param data Pointer to boot.img data
 * @param size Size of boot.img data
 * @param info Output boot info structure
 * @return BOOT_PARSE_SUCCESS or negative error code
 */
int android_boot_parse(const void* data, size_t size, android_boot_info_t* info);

/**
 * Parse vendor_boot.img and merge with boot info
 * @param data Pointer to vendor_boot.img data
 * @param size Size of vendor_boot.img data
 * @param info Boot info structure to update
 * @return BOOT_PARSE_SUCCESS or negative error code
 */
int android_boot_parse_vendor(const void* data, size_t size, android_boot_info_t* info);

/**
 * Extract kernel from boot image
 * @param info Boot info structure
 * @param buffer Output buffer for kernel data
 * @param buffer_size Size of output buffer
 * @return Actual kernel size or negative error code
 */
int android_boot_extract_kernel(const android_boot_info_t* info, void* buffer, size_t buffer_size);

/**
 * Extract ramdisk from boot image
 * @param info Boot info structure
 * @param buffer Output buffer for ramdisk data
 * @param buffer_size Size of output buffer
 * @return Actual ramdisk size or negative error code
 */
int android_boot_extract_ramdisk(const android_boot_info_t* info, void* buffer, size_t buffer_size);

/**
 * Extract device tree blob from boot image
 * @param info Boot info structure
 * @param buffer Output buffer for DTB data
 * @param buffer_size Size of output buffer
 * @return Actual DTB size or negative error code
 */
int android_boot_extract_dtb(const android_boot_info_t* info, void* buffer, size_t buffer_size);

/**
 * Get command line from boot image
 * @param info Boot info structure
 * @param buffer Output buffer for command line
 * @param buffer_size Size of output buffer
 * @return Length of command line or negative error code
 */
int android_boot_get_cmdline(const android_boot_info_t* info, char* buffer, size_t buffer_size);

/**
 * Verify boot image signature (v4)
 * @param info Boot info structure
 * @return 0 if valid, negative error code if invalid
 */
int android_boot_verify_signature(const android_boot_info_t* info);

/**
 * Calculate page-aligned size
 * @param size Original size
 * @param page_size Page size (typically 4096)
 * @return Page-aligned size
 */
uint32_t android_boot_page_align(uint32_t size, uint32_t page_size);

/**
 * Decode OS version from packed format
 * @param os_version Packed OS version value
 * @param major Output major version
 * @param minor Output minor version
 * @param patch Output patch level
 */
void android_boot_decode_os_version(uint32_t os_version, 
                                     uint32_t* major, 
                                     uint32_t* minor, 
                                     uint32_t* patch);

/**
 * Decode patch level from packed format
 * @param os_version Packed OS version value (contains patch level)
 * @param year Output patch year
 * @param month Output patch month
 */
void android_boot_decode_patch_level(uint32_t os_version,
                                      uint32_t* year,
                                      uint32_t* month);

/**
 * Load Android boot image from storage device
 * @param device_name Device name (e.g., "sda", "mmcblk0")
 * @param partition_name Partition name (e.g., "boot", "boot_a")
 * @param info Output boot info structure
 * @return 0 on success, negative error code on failure
 */
int android_boot_load_from_device(const char* device_name, 
                                   const char* partition_name,
                                   android_boot_info_t* info);

/**
 * Boot Android kernel with ramdisk
 * @param info Boot info structure containing kernel and ramdisk
 * @return Does not return on success, negative error code on failure
 */
int android_boot_start(const android_boot_info_t* info);

/**
 * Free resources allocated during boot image parsing
 * @param info Boot info structure to free
 */
void android_boot_free(android_boot_info_t* info);

/**
 * Print boot image information for debugging
 * @param info Boot info structure
 */
void android_boot_print_info(const android_boot_info_t* info);

/**
 * Get header version from boot image data
 * @param data Pointer to boot.img data
 * @param size Size of boot.img data
 * @return Header version (0-4) or negative error code
 */
int android_boot_get_version(const void* data, size_t size);

/**
 * Validate boot image checksum (v0-v2)
 * @param data Pointer to boot.img data
 * @param size Size of boot.img data
 * @param info Boot info structure
 * @return 0 if valid, negative error code if invalid
 */
int android_boot_validate_checksum(const void* data, size_t size, const android_boot_info_t* info);

#endif /* AURORA_ANDROID_BOOT_H */
