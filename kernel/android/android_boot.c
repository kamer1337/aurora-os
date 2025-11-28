/**
 * Aurora OS - Android Boot Protocol Implementation
 * 
 * Implements boot.img v0-v4 parsing and kernel/ramdisk extraction
 */

#include "android_boot.h"
#include "../memory/memory.h"
#include "../drivers/vga.h"
#include "../drivers/storage.h"

/* Global Android boot state */
static int g_android_boot_initialized = 0;

/* Internal helper functions */
static int mem_compare(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

static void mem_copy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static void mem_set(void* dest, int val, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = (uint8_t)val;
    }
}

static size_t str_len(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

static void str_copy(char* dest, const char* src, size_t n) {
    size_t i = 0;
    while (i < n - 1 && src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* SHA-1 implementation for boot image verification */
typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    uint8_t buffer[64];
} sha1_ctx_t;

static void sha1_init(sha1_ctx_t* ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = 0;
    ctx->count[1] = 0;
}

#define SHA1_ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static void sha1_transform(sha1_ctx_t* ctx, const uint8_t* data) {
    uint32_t a, b, c, d, e;
    uint32_t w[80];
    
    /* Convert data to big-endian words */
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    
    /* Extend words */
    for (int i = 16; i < 80; i++) {
        w[i] = SHA1_ROL(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);
    }
    
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    
    /* Main loop */
    for (int i = 0; i < 80; i++) {
        uint32_t f, k;
        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        
        uint32_t temp = SHA1_ROL(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }
    
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

static void sha1_update(sha1_ctx_t* ctx, const uint8_t* data, size_t len) {
    size_t i = 0;
    size_t j = (ctx->count[0] >> 3) & 63;
    
    ctx->count[0] += (uint32_t)(len << 3);
    if (ctx->count[0] < (len << 3)) {
        ctx->count[1]++;
    }
    ctx->count[1] += (uint32_t)(len >> 29);
    
    if ((j + len) > 63) {
        i = 64 - j;
        mem_copy(&ctx->buffer[j], data, i);
        sha1_transform(ctx, ctx->buffer);
        
        for (; i + 63 < len; i += 64) {
            sha1_transform(ctx, &data[i]);
        }
        j = 0;
    }
    
    mem_copy(&ctx->buffer[j], &data[i], len - i);
}

static void sha1_final(sha1_ctx_t* ctx, uint8_t* digest) {
    uint8_t finalcount[8];
    
    for (int i = 0; i < 8; i++) {
        finalcount[i] = (uint8_t)((ctx->count[(i >= 4) ? 0 : 1] >> 
                                  ((3 - (i & 3)) * 8)) & 255);
    }
    
    uint8_t pad = 0x80;
    sha1_update(ctx, &pad, 1);
    
    while ((ctx->count[0] & 504) != 448) {
        pad = 0;
        sha1_update(ctx, &pad, 1);
    }
    
    sha1_update(ctx, finalcount, 8);
    
    for (int i = 0; i < 20; i++) {
        digest[i] = (uint8_t)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 255);
    }
}

/**
 * Initialize Android boot subsystem
 */
int android_boot_init(void) {
    if (g_android_boot_initialized) {
        return 0;
    }
    
    g_android_boot_initialized = 1;
    return 0;
}

/**
 * Calculate page-aligned size
 */
uint32_t android_boot_page_align(uint32_t size, uint32_t page_size) {
    if (page_size == 0) {
        page_size = 4096;
    }
    return ((size + page_size - 1) / page_size) * page_size;
}

/**
 * Decode OS version from packed format
 */
void android_boot_decode_os_version(uint32_t os_version, 
                                     uint32_t* major, 
                                     uint32_t* minor, 
                                     uint32_t* patch) {
    /* OS version is encoded in upper 11 bits of os_version field */
    uint32_t version = (os_version >> 11) & 0x7FF;
    if (major) *major = (version >> 14) & 0x7F;
    if (minor) *minor = (version >> 7) & 0x7F;
    if (patch) *patch = version & 0x7F;
}

/**
 * Decode patch level from packed format
 */
void android_boot_decode_patch_level(uint32_t os_version,
                                      uint32_t* year,
                                      uint32_t* month) {
    /* Patch level is encoded in lower 11 bits */
    uint32_t patch_level = os_version & 0x7FF;
    if (year) *year = (patch_level >> 4) + 2000;
    if (month) *month = patch_level & 0xF;
}

/**
 * Get header version from boot image data
 */
int android_boot_image_get_version(const void* data, size_t size) {
    if (!data || size < sizeof(boot_img_hdr_v0_t)) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    const boot_img_hdr_v0_t* hdr = (const boot_img_hdr_v0_t*)data;
    
    /* Validate magic */
    if (mem_compare(hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        return BOOT_PARSE_INVALID_MAGIC;
    }
    
    return (int)hdr->header_version;
}

/**
 * Parse boot.img v0/v1/v2 format
 */
static int parse_boot_v0_v1_v2(const void* data, size_t size, android_boot_info_t* info) {
    const boot_img_hdr_v0_t* hdr = (const boot_img_hdr_v0_t*)data;
    const uint8_t* img_data = (const uint8_t*)data;
    
    info->header_version = hdr->header_version;
    info->page_size = hdr->page_size ? hdr->page_size : 2048;
    
    /* Kernel info */
    info->kernel_size = hdr->kernel_size;
    info->kernel_addr = hdr->kernel_addr;
    
    /* Ramdisk info */
    info->ramdisk_size = hdr->ramdisk_size;
    info->ramdisk_addr = hdr->ramdisk_addr;
    
    /* Second stage bootloader (v0-v2) */
    info->second_size = hdr->second_size;
    info->second_addr = hdr->second_addr;
    
    /* Calculate offsets */
    uint32_t page_size = info->page_size;
    uint32_t kernel_pages = android_boot_page_align(hdr->kernel_size, page_size) / page_size;
    uint32_t ramdisk_pages = android_boot_page_align(hdr->ramdisk_size, page_size) / page_size;
    uint32_t second_pages = android_boot_page_align(hdr->second_size, page_size) / page_size;
    
    /* Kernel starts at page 1 (after header) */
    uint32_t kernel_offset = page_size;
    uint32_t ramdisk_offset = kernel_offset + (kernel_pages * page_size);
    uint32_t second_offset = ramdisk_offset + (ramdisk_pages * page_size);
    
    /* Validate sizes */
    if (kernel_offset + hdr->kernel_size > size ||
        ramdisk_offset + hdr->ramdisk_size > size ||
        (hdr->second_size > 0 && second_offset + hdr->second_size > size)) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Set data pointers */
    info->kernel_data = (void*)(img_data + kernel_offset);
    info->ramdisk_data = (void*)(img_data + ramdisk_offset);
    if (hdr->second_size > 0) {
        info->second_data = (void*)(img_data + second_offset);
    }
    
    /* v1+ fields */
    if (hdr->header_version >= 1) {
        info->recovery_dtbo_size = hdr->recovery_dtbo_size;
        info->recovery_dtbo_offset = hdr->recovery_dtbo_offset;
        if (hdr->recovery_dtbo_size > 0 && hdr->recovery_dtbo_offset < size) {
            info->recovery_dtbo_data = (void*)(img_data + hdr->recovery_dtbo_offset);
        }
    }
    
    /* v2+ fields */
    if (hdr->header_version >= 2) {
        info->dtb_size = hdr->dtb_size;
        info->dtb_addr = hdr->dtb_addr;
        
        uint32_t dtb_offset = second_offset + (second_pages * page_size);
        if (hdr->dtb_size > 0 && dtb_offset + hdr->dtb_size <= size) {
            info->dtb_data = (void*)(img_data + dtb_offset);
        }
    }
    
    /* Copy command line */
    str_copy(info->cmdline, (const char*)hdr->cmdline, BOOT_ARGS_SIZE);
    size_t cmdline_len = str_len(info->cmdline);
    if (cmdline_len < BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE - 1) {
        str_copy(info->cmdline + cmdline_len, (const char*)hdr->extra_cmdline, 
                 BOOT_EXTRA_ARGS_SIZE);
    }
    
    /* Copy name */
    str_copy(info->name, (const char*)hdr->name, BOOT_NAME_SIZE);
    
    /* Copy ID (SHA-1 hash) */
    for (int i = 0; i < 8; i++) {
        info->id[i] = hdr->id[i];
    }
    
    /* Decode OS version */
    android_boot_decode_os_version(hdr->os_version, 
                                    &info->os_version_major,
                                    &info->os_version_minor,
                                    &info->os_version_patch);
    android_boot_decode_patch_level(hdr->os_version,
                                     &info->os_patch_level_year,
                                     &info->os_patch_level_month);
    
    info->valid = 1;
    return BOOT_PARSE_SUCCESS;
}

/**
 * Parse boot.img v3 format
 */
static int parse_boot_v3(const void* data, size_t size, android_boot_info_t* info) {
    const boot_img_hdr_v3_t* hdr = (const boot_img_hdr_v3_t*)data;
    const uint8_t* img_data = (const uint8_t*)data;
    
    info->header_version = 3;
    info->page_size = 4096;  /* v3 uses fixed 4K pages */
    
    /* Kernel info */
    info->kernel_size = hdr->kernel_size;
    info->kernel_addr = 0;  /* Address from vendor_boot */
    
    /* Ramdisk info */
    info->ramdisk_size = hdr->ramdisk_size;
    info->ramdisk_addr = 0;  /* Address from vendor_boot */
    
    /* Calculate offsets - v3 is simpler */
    uint32_t page_size = 4096;
    uint32_t kernel_offset = android_boot_page_align(hdr->header_size, page_size);
    uint32_t ramdisk_offset = kernel_offset + android_boot_page_align(hdr->kernel_size, page_size);
    
    /* Validate sizes */
    if (kernel_offset + hdr->kernel_size > size ||
        ramdisk_offset + hdr->ramdisk_size > size) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Set data pointers */
    info->kernel_data = (void*)(img_data + kernel_offset);
    info->ramdisk_data = (void*)(img_data + ramdisk_offset);
    
    /* Copy command line */
    str_copy(info->cmdline, (const char*)hdr->cmdline, 
             BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE);
    
    /* Decode OS version */
    android_boot_decode_os_version(hdr->os_version,
                                    &info->os_version_major,
                                    &info->os_version_minor,
                                    &info->os_version_patch);
    android_boot_decode_patch_level(hdr->os_version,
                                     &info->os_patch_level_year,
                                     &info->os_patch_level_month);
    
    info->valid = 1;
    return BOOT_PARSE_SUCCESS;
}

/**
 * Parse boot.img v4 format
 */
static int parse_boot_v4(const void* data, size_t size, android_boot_info_t* info) {
    const boot_img_hdr_v4_t* hdr = (const boot_img_hdr_v4_t*)data;
    const uint8_t* img_data = (const uint8_t*)data;
    
    info->header_version = 4;
    info->page_size = 4096;  /* v4 uses fixed 4K pages */
    
    /* Kernel info */
    info->kernel_size = hdr->kernel_size;
    info->kernel_addr = 0;  /* Address from vendor_boot */
    
    /* Ramdisk info */
    info->ramdisk_size = hdr->ramdisk_size;
    info->ramdisk_addr = 0;  /* Address from vendor_boot */
    
    /* Signature info */
    info->signature_size = hdr->signature_size;
    
    /* Calculate offsets */
    uint32_t page_size = 4096;
    uint32_t kernel_offset = android_boot_page_align(hdr->header_size, page_size);
    uint32_t ramdisk_offset = kernel_offset + android_boot_page_align(hdr->kernel_size, page_size);
    uint32_t signature_offset = ramdisk_offset + android_boot_page_align(hdr->ramdisk_size, page_size);
    
    /* Validate sizes */
    if (kernel_offset + hdr->kernel_size > size ||
        ramdisk_offset + hdr->ramdisk_size > size) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Set data pointers */
    info->kernel_data = (void*)(img_data + kernel_offset);
    info->ramdisk_data = (void*)(img_data + ramdisk_offset);
    
    if (hdr->signature_size > 0 && signature_offset + hdr->signature_size <= size) {
        info->signature_data = (void*)(img_data + signature_offset);
    }
    
    /* Copy command line */
    str_copy(info->cmdline, (const char*)hdr->cmdline,
             BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE);
    
    /* Decode OS version */
    android_boot_decode_os_version(hdr->os_version,
                                    &info->os_version_major,
                                    &info->os_version_minor,
                                    &info->os_version_patch);
    android_boot_decode_patch_level(hdr->os_version,
                                     &info->os_patch_level_year,
                                     &info->os_patch_level_month);
    
    info->valid = 1;
    return BOOT_PARSE_SUCCESS;
}

/**
 * Parse boot.img header and validate format
 */
int android_boot_parse(const void* data, size_t size, android_boot_info_t* info) {
    if (!data || !info) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    if (size < sizeof(boot_img_hdr_v0_t)) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Clear info structure */
    mem_set(info, 0, sizeof(android_boot_info_t));
    
    /* Validate magic */
    if (mem_compare(data, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        return BOOT_PARSE_INVALID_MAGIC;
    }
    
    /* Get header version */
    int version = android_boot_image_get_version(data, size);
    if (version < 0) {
        return version;
    }
    
    /* Parse based on version */
    switch (version) {
        case 0:
        case 1:
        case 2:
            return parse_boot_v0_v1_v2(data, size, info);
        case 3:
            return parse_boot_v3(data, size, info);
        case 4:
            return parse_boot_v4(data, size, info);
        default:
            return BOOT_PARSE_UNSUPPORTED_VERSION;
    }
}

/**
 * Parse vendor_boot.img and merge with boot info
 */
int android_boot_parse_vendor(const void* data, size_t size, android_boot_info_t* info) {
    if (!data || !info || size < sizeof(vendor_boot_img_hdr_v3_t)) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Validate vendor boot magic */
    if (mem_compare(data, VENDOR_BOOT_MAGIC, VENDOR_BOOT_MAGIC_SIZE) != 0) {
        return BOOT_PARSE_INVALID_MAGIC;
    }
    
    const vendor_boot_img_hdr_v3_t* hdr = (const vendor_boot_img_hdr_v3_t*)data;
    const uint8_t* img_data = (const uint8_t*)data;
    
    /* Update boot info with vendor data */
    info->page_size = hdr->page_size ? hdr->page_size : 4096;
    info->kernel_addr = hdr->kernel_addr;
    info->ramdisk_addr = hdr->ramdisk_addr;
    info->dtb_size = hdr->dtb_size;
    info->dtb_addr = hdr->dtb_addr;
    
    /* Calculate vendor ramdisk offset */
    uint32_t page_size = info->page_size;
    uint32_t vendor_ramdisk_offset = android_boot_page_align(hdr->header_size, page_size);
    uint32_t dtb_offset = vendor_ramdisk_offset + 
                          android_boot_page_align(hdr->vendor_ramdisk_size, page_size);
    
    /* Set vendor ramdisk data if present */
    if (hdr->vendor_ramdisk_size > 0 && 
        vendor_ramdisk_offset + hdr->vendor_ramdisk_size <= size) {
        /* Note: vendor ramdisk should be concatenated with boot ramdisk */
        /* For now, we just note its location */
    }
    
    /* Set DTB data */
    if (hdr->dtb_size > 0 && dtb_offset + hdr->dtb_size <= size) {
        info->dtb_data = (void*)(img_data + dtb_offset);
    }
    
    /* Append vendor command line */
    size_t current_len = str_len(info->cmdline);
    if (current_len > 0 && current_len < sizeof(info->cmdline) - 2) {
        info->cmdline[current_len] = ' ';
        current_len++;
    }
    str_copy(info->cmdline + current_len, (const char*)hdr->cmdline,
             sizeof(info->cmdline) - current_len);
    
    /* Copy vendor boot name */
    str_copy(info->name, (const char*)hdr->name, VENDOR_BOOT_NAME_SIZE);
    
    /* Handle v4 specific fields */
    if (hdr->header_version >= 4) {
        const vendor_boot_img_hdr_v4_t* hdr_v4 = (const vendor_boot_img_hdr_v4_t*)data;
        
        info->vendor_ramdisk_count = hdr_v4->vendor_ramdisk_table_entry_num;
        info->bootconfig_size = hdr_v4->bootconfig_size;
        
        /* Parse vendor ramdisk table if present */
        if (hdr_v4->vendor_ramdisk_table_entry_num > 0) {
            uint32_t table_offset = dtb_offset + 
                                   android_boot_page_align(hdr->dtb_size, page_size);
            
            if (table_offset + hdr_v4->vendor_ramdisk_table_size <= size) {
                /* Allocate and copy vendor ramdisk table */
                size_t table_size = hdr_v4->vendor_ramdisk_table_entry_num * 
                                   sizeof(vendor_ramdisk_table_entry_t);
                info->vendor_ramdisks = (vendor_ramdisk_table_entry_t*)kmalloc(table_size);
                if (info->vendor_ramdisks) {
                    mem_copy(info->vendor_ramdisks, img_data + table_offset, table_size);
                }
            }
        }
        
        /* Parse bootconfig if present */
        if (hdr_v4->bootconfig_size > 0) {
            uint32_t bootconfig_offset = dtb_offset + 
                                        android_boot_page_align(hdr->dtb_size, page_size) +
                                        android_boot_page_align(hdr_v4->vendor_ramdisk_table_size, page_size);
            
            if (bootconfig_offset + hdr_v4->bootconfig_size <= size) {
                info->bootconfig_data = (char*)kmalloc(hdr_v4->bootconfig_size + 1);
                if (info->bootconfig_data) {
                    mem_copy(info->bootconfig_data, img_data + bootconfig_offset, 
                            hdr_v4->bootconfig_size);
                    info->bootconfig_data[hdr_v4->bootconfig_size] = '\0';
                }
            }
        }
    }
    
    info->has_vendor_boot = 1;
    return BOOT_PARSE_SUCCESS;
}

/**
 * Extract kernel from boot image
 */
int android_boot_extract_kernel(const android_boot_info_t* info, void* buffer, size_t buffer_size) {
    if (!info || !info->valid || !buffer) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    if (info->kernel_size == 0 || !info->kernel_data) {
        return 0;
    }
    
    if (buffer_size < info->kernel_size) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    mem_copy(buffer, info->kernel_data, info->kernel_size);
    return (int)info->kernel_size;
}

/**
 * Extract ramdisk from boot image
 */
int android_boot_extract_ramdisk(const android_boot_info_t* info, void* buffer, size_t buffer_size) {
    if (!info || !info->valid || !buffer) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    if (info->ramdisk_size == 0 || !info->ramdisk_data) {
        return 0;
    }
    
    if (buffer_size < info->ramdisk_size) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    mem_copy(buffer, info->ramdisk_data, info->ramdisk_size);
    return (int)info->ramdisk_size;
}

/**
 * Extract device tree blob from boot image
 */
int android_boot_extract_dtb(const android_boot_info_t* info, void* buffer, size_t buffer_size) {
    if (!info || !info->valid || !buffer) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    if (info->dtb_size == 0 || !info->dtb_data) {
        return 0;
    }
    
    if (buffer_size < info->dtb_size) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    mem_copy(buffer, info->dtb_data, info->dtb_size);
    return (int)info->dtb_size;
}

/**
 * Get command line from boot image
 */
int android_boot_get_cmdline(const android_boot_info_t* info, char* buffer, size_t buffer_size) {
    if (!info || !info->valid || !buffer || buffer_size == 0) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    size_t cmdline_len = str_len(info->cmdline);
    if (cmdline_len >= buffer_size) {
        cmdline_len = buffer_size - 1;
    }
    
    str_copy(buffer, info->cmdline, cmdline_len + 1);
    return (int)cmdline_len;
}

/**
 * Validate boot image checksum (v0-v2)
 */
int android_boot_validate_checksum(const void* data, size_t size, const android_boot_info_t* info) {
    if (!data || !info || !info->valid) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Only v0-v2 have SHA-1 hash */
    if (info->header_version > 2) {
        return BOOT_PARSE_SUCCESS;  /* No checksum in v3+ */
    }
    
    sha1_ctx_t ctx;
    uint8_t digest[20];
    
    sha1_init(&ctx);
    
    /* Hash kernel */
    if (info->kernel_data && info->kernel_size > 0) {
        sha1_update(&ctx, (const uint8_t*)info->kernel_data, info->kernel_size);
    }
    
    /* Hash ramdisk */
    if (info->ramdisk_data && info->ramdisk_size > 0) {
        sha1_update(&ctx, (const uint8_t*)info->ramdisk_data, info->ramdisk_size);
    }
    
    /* Hash second stage bootloader */
    if (info->second_data && info->second_size > 0) {
        sha1_update(&ctx, (const uint8_t*)info->second_data, info->second_size);
    }
    
    /* Hash DTB (v2) */
    if (info->header_version >= 2 && info->dtb_data && info->dtb_size > 0) {
        sha1_update(&ctx, (const uint8_t*)info->dtb_data, info->dtb_size);
    }
    
    sha1_final(&ctx, digest);
    
    /* Compare with stored hash */
    const uint32_t* stored_id = info->id;
    for (int i = 0; i < 5; i++) {
        uint32_t computed = ((uint32_t)digest[i*4] << 24) |
                           ((uint32_t)digest[i*4+1] << 16) |
                           ((uint32_t)digest[i*4+2] << 8) |
                           ((uint32_t)digest[i*4+3]);
        if (computed != stored_id[i]) {
            return BOOT_PARSE_CHECKSUM_ERROR;
        }
    }
    
    return BOOT_PARSE_SUCCESS;
}

/**
 * Verify boot image signature (v4)
 */
int android_boot_verify_signature(const android_boot_info_t* info) {
    if (!info || !info->valid) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Only v4 has boot signature */
    if (info->header_version != 4) {
        return BOOT_PARSE_SUCCESS;  /* No signature in v0-v3 */
    }
    
    if (info->signature_size == 0 || !info->signature_data) {
        return BOOT_PARSE_SUCCESS;  /* No signature present */
    }
    
    /* TODO: Implement AVB signature verification */
    /* This would require implementing Android Verified Boot (AVB) */
    /* For now, we just check that signature data exists */
    
    return BOOT_PARSE_SUCCESS;
}

/**
 * Free resources allocated during boot image parsing
 */
void android_boot_free(android_boot_info_t* info) {
    if (!info) {
        return;
    }
    
    if (info->vendor_ramdisks) {
        kfree(info->vendor_ramdisks);
        info->vendor_ramdisks = NULL;
    }
    
    if (info->bootconfig_data) {
        kfree(info->bootconfig_data);
        info->bootconfig_data = NULL;
    }
    
    mem_set(info, 0, sizeof(android_boot_info_t));
}

/**
 * Print boot image information for debugging
 */
void android_boot_print_info(const android_boot_info_t* info) {
    if (!info || !info->valid) {
        vga_write("Invalid boot image info\n");
        return;
    }
    
    vga_write("\n=== Android Boot Image Info ===\n");
    
    vga_write("Header Version: ");
    vga_write_dec(info->header_version);
    vga_write("\n");
    
    vga_write("Page Size: ");
    vga_write_dec(info->page_size);
    vga_write("\n");
    
    vga_write("Kernel Size: ");
    vga_write_dec(info->kernel_size);
    vga_write(" bytes at 0x");
    vga_write_hex((uint32_t)info->kernel_addr);
    vga_write("\n");
    
    vga_write("Ramdisk Size: ");
    vga_write_dec(info->ramdisk_size);
    vga_write(" bytes at 0x");
    vga_write_hex((uint32_t)info->ramdisk_addr);
    vga_write("\n");
    
    if (info->dtb_size > 0) {
        vga_write("DTB Size: ");
        vga_write_dec(info->dtb_size);
        vga_write(" bytes at 0x");
        vga_write_hex((uint32_t)info->dtb_addr);
        vga_write("\n");
    }
    
    vga_write("OS Version: ");
    vga_write_dec(info->os_version_major);
    vga_write(".");
    vga_write_dec(info->os_version_minor);
    vga_write(".");
    vga_write_dec(info->os_version_patch);
    vga_write("\n");
    
    vga_write("Patch Level: ");
    vga_write_dec(info->os_patch_level_year);
    vga_write("-");
    if (info->os_patch_level_month < 10) vga_write("0");
    vga_write_dec(info->os_patch_level_month);
    vga_write("\n");
    
    if (info->name[0]) {
        vga_write("Name: ");
        vga_write(info->name);
        vga_write("\n");
    }
    
    if (info->cmdline[0]) {
        vga_write("Cmdline: ");
        vga_write(info->cmdline);
        vga_write("\n");
    }
    
    if (info->has_vendor_boot) {
        vga_write("Has Vendor Boot: Yes\n");
    }
    
    if (info->header_version == 4 && info->signature_size > 0) {
        vga_write("Signature Size: ");
        vga_write_dec(info->signature_size);
        vga_write(" bytes\n");
    }
    
    vga_write("================================\n\n");
}

/**
 * Load Android boot image from storage device
 */
int android_boot_load_from_device(const char* device_name, 
                                   const char* partition_name,
                                   android_boot_info_t* info) {
    (void)device_name;
    (void)partition_name;
    
    if (!info) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* TODO: Implement actual device loading using storage driver */
    /* This would require:
     * 1. Opening the block device
     * 2. Reading the partition table to find boot partition
     * 3. Reading boot.img data from partition
     * 4. Calling android_boot_parse()
     */
    
    return BOOT_PARSE_SUCCESS;
}

/**
 * Boot Android kernel with ramdisk
 */
int android_boot_start(const android_boot_info_t* info) {
    if (!info || !info->valid) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    if (!info->kernel_data || info->kernel_size == 0) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    vga_write("Starting Android kernel...\n");
    vga_write("Kernel at: 0x");
    vga_write_hex((uint32_t)(uintptr_t)info->kernel_data);
    vga_write(", size: ");
    vga_write_dec(info->kernel_size);
    vga_write("\n");
    
    /* TODO: Implement actual kernel boot sequence:
     * 1. Copy kernel to target address
     * 2. Set up boot parameters (ATAG or DTB)
     * 3. Set up ramdisk
     * 4. Jump to kernel entry point
     */
    
    /* For ARM:
     * - r0 = 0
     * - r1 = machine type
     * - r2 = ATAG/DTB address
     */
    
    /* For x86:
     * - Set up multiboot structures
     * - Jump to kernel entry
     */
    
    return BOOT_PARSE_SUCCESS;
}
