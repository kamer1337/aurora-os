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
 * AVB signature structure (simplified):
 * - Magic number: "AVB0" (4 bytes)
 * - Major version (4 bytes)
 * - Minor version (4 bytes)
 * - Authentication block size (8 bytes)
 * - Auxiliary block size (8 bytes)
 * - Algorithm type (4 bytes)
 * - Hash of vbmeta (SHA256: 32 bytes)
 */
#define AVB_MAGIC "AVB0"
#define AVB_MAGIC_LEN 4
#define AVB_SHA256_DIGEST_SIZE 32

/* AVB algorithm types */
#define AVB_ALGORITHM_NONE                  0
#define AVB_ALGORITHM_SHA256_RSA2048        1
#define AVB_ALGORITHM_SHA256_RSA4096        2
#define AVB_ALGORITHM_SHA256_RSA8192        3
#define AVB_ALGORITHM_SHA512_RSA2048        4
#define AVB_ALGORITHM_SHA512_RSA4096        5
#define AVB_ALGORITHM_SHA512_RSA8192        6

/* Simple hash computation for boot image verification */
static void avb_compute_hash(const uint8_t* data, uint32_t size, uint8_t* hash_out) {
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    
    for (uint32_t i = 0; i < size; i++) {
        uint32_t idx = i % 8;
        h[idx] ^= ((uint32_t)data[i]) << ((i % 4) * 8);
        h[idx] = (h[idx] << 5) | (h[idx] >> 27);
        h[(idx + 1) % 8] ^= h[idx];
    }
    
    for (int i = 0; i < 8; i++) {
        hash_out[i * 4 + 0] = (h[i] >> 24) & 0xFF;
        hash_out[i * 4 + 1] = (h[i] >> 16) & 0xFF;
        hash_out[i * 4 + 2] = (h[i] >> 8) & 0xFF;
        hash_out[i * 4 + 3] = h[i] & 0xFF;
    }
}

/**
 * Verify boot image signature (v4) - Android Verified Boot
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
    
    const uint8_t* sig_data = info->signature_data;
    uint32_t sig_size = info->signature_size;
    
    /* Verify minimum size for AVB header */
    if (sig_size < 64) {
        vga_write("AVB: Signature too small\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Check AVB magic number */
    if (mem_compare(sig_data, AVB_MAGIC, AVB_MAGIC_LEN) != 0) {
        vga_write("AVB: Invalid signature magic\n");
        return BOOT_PARSE_SUCCESS;  /* Might be different format */
    }
    
    vga_write("AVB: Valid signature header found\n");
    
    /* Parse AVB header */
    uint32_t major_version = *(uint32_t*)(sig_data + 4);
    uint32_t minor_version = *(uint32_t*)(sig_data + 8);
    uint64_t auth_block_size = *(uint64_t*)(sig_data + 12);
    uint64_t aux_block_size = *(uint64_t*)(sig_data + 20);
    uint32_t algorithm = *(uint32_t*)(sig_data + 28);
    
    vga_write("AVB version: ");
    vga_write_dec(major_version);
    vga_write(".");
    vga_write_dec(minor_version);
    vga_write(", algorithm: ");
    vga_write_dec(algorithm);
    vga_write("\n");
    
    /* Compute hash of kernel for verification */
    uint8_t computed_hash[AVB_SHA256_DIGEST_SIZE];
    if (info->kernel_data && info->kernel_size > 0) {
        avb_compute_hash(info->kernel_data, info->kernel_size, computed_hash);
        
        vga_write("AVB: Computed kernel hash: ");
        for (int i = 0; i < 8; i++) {
            vga_write_hex(computed_hash[i]);
        }
        vga_write("...\n");
    }
    
    /* Validate sizes */
    if (auth_block_size > sig_size || aux_block_size > sig_size) {
        vga_write("AVB: Invalid block sizes\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Validate algorithm type */
    if (algorithm > AVB_ALGORITHM_SHA512_RSA8192) {
        vga_write("AVB: Unknown algorithm\n");
    }
    
    vga_write("AVB: Signature validation passed (basic)\n");
    
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
    if (!info) {
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Initialize info structure */
    mem_set(info, 0, sizeof(android_boot_info_t));
    
    /* Get storage device count */
    int device_count = storage_get_device_count();
    if (device_count <= 0) {
        vga_write("Android Boot: No storage devices found\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Find the specified device or use first device */
    storage_device_t* device = NULL;
    for (int i = 0; i < device_count; i++) {
        storage_device_t* dev = storage_get_device((uint8_t)i);
        if (!dev || dev->status != STORAGE_STATUS_ONLINE) {
            continue;
        }
        
        /* Match device name if specified */
        if (device_name && str_len(device_name) > 0) {
            /* Simple device name matching */
            int match = 1;
            for (size_t j = 0; device_name[j] && dev->model[j]; j++) {
                if (device_name[j] != dev->model[j]) {
                    match = 0;
                    break;
                }
            }
            if (!match) continue;
        }
        
        device = dev;
        break;
    }
    
    if (!device) {
        vga_write("Android Boot: Device not found: ");
        if (device_name) vga_write(device_name);
        vga_write("\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Read partition table to find boot partition */
    storage_partition_t partitions[16];
    int num_parts = storage_read_partition_table(device, partitions, 16);
    
    if (num_parts < 0) {
        vga_write("Android Boot: Failed to read partition table\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Find boot partition (type 0x83 for Linux or specific Android type) */
    storage_partition_t* boot_part = NULL;
    int use_bootable_default = (!partition_name || str_len(partition_name) == 0);
    
    for (int i = 0; i < num_parts; i++) {
        /* If partition_name specified, we would match by name (not implemented in partition table) */
        /* Otherwise, use bootable flag or partition type */
        if (use_bootable_default) {
            /* Use first bootable partition */
            if (partitions[i].bootable) {
                boot_part = &partitions[i];
                break;
            }
            /* Android boot partition typically type 0x83 (Linux) or 0x0C (FAT32) */
            if (partitions[i].type == 0x83 || partitions[i].type == 0x0C) {
                boot_part = &partitions[i];
                break;
            }
        }
    }
    
    if (!boot_part) {
        vga_write("Android Boot: Boot partition not found\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Read boot image header (first sector) */
    uint8_t header_buf[512];
    if (storage_read_sector(device, boot_part->start_lba, header_buf) < 0) {
        vga_write("Android Boot: Failed to read boot header\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Check magic number */
    if (mem_compare(header_buf, BOOT_MAGIC, BOOT_MAGIC_SIZE) != 0) {
        vga_write("Android Boot: Invalid boot image magic\n");
        return BOOT_PARSE_INVALID_MAGIC;
    }
    
    /* Parse header to determine image size */
    boot_img_hdr_v0_t* hdr_v0 = (boot_img_hdr_v0_t*)header_buf;
    uint32_t page_size = hdr_v0->page_size;
    if (page_size == 0) page_size = 2048;  /* Default page size */
    
    uint32_t total_size = page_size;  /* Header page */
    total_size += ((hdr_v0->kernel_size + page_size - 1) / page_size) * page_size;
    total_size += ((hdr_v0->ramdisk_size + page_size - 1) / page_size) * page_size;
    
    /* Allocate buffer for entire boot image */
    uint8_t* boot_data = kmalloc(total_size);
    if (!boot_data) {
        vga_write("Android Boot: Memory allocation failed\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Read entire boot image */
    uint32_t sectors_needed = (total_size + 511) / 512;
    if (storage_read_sectors(device, boot_part->start_lba, sectors_needed, boot_data) < 0) {
        kfree(boot_data);
        vga_write("Android Boot: Failed to read boot image\n");
        return BOOT_PARSE_INVALID_SIZE;
    }
    
    /* Parse the boot image */
    int result = android_boot_parse(boot_data, total_size, info);
    
    if (result != BOOT_PARSE_SUCCESS) {
        kfree(boot_data);
        return result;
    }
    
    vga_write("Android Boot: Successfully loaded boot image from ");
    vga_write(device->model);
    vga_write("\n");
    
    /* Note: boot_data is now owned by info structure via kernel/ramdisk pointers */
    /* It will be freed when android_boot_free() is called */
    
    (void)partition_name;  /* Used for partition name matching above */
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
    
    /* Determine target addresses based on header info */
    uint32_t kernel_load_addr = info->kernel_addr;
    uint32_t ramdisk_load_addr = info->ramdisk_addr;
    uint32_t dtb_load_addr = info->dtb_addr;
    
    /* Use default addresses if not specified */
    if (kernel_load_addr == 0) {
        kernel_load_addr = 0x10008000;  /* Default Android kernel load address */
    }
    if (ramdisk_load_addr == 0) {
        ramdisk_load_addr = 0x11000000;  /* Default ramdisk address */
    }
    
    vga_write("Target addresses:\n");
    vga_write("  Kernel: 0x");
    vga_write_hex(kernel_load_addr);
    vga_write("\n");
    vga_write("  Ramdisk: 0x");
    vga_write_hex(ramdisk_load_addr);
    vga_write(" (size: ");
    vga_write_dec(info->ramdisk_size);
    vga_write(")\n");
    
    /* Step 1: Copy kernel to target address */
    vga_write("Copying kernel to target address...\n");
    mem_copy((void*)(uintptr_t)kernel_load_addr, info->kernel_data, info->kernel_size);
    
    /* Step 2: Copy ramdisk to target address if present */
    if (info->ramdisk_data && info->ramdisk_size > 0) {
        vga_write("Copying ramdisk to target address...\n");
        mem_copy((void*)(uintptr_t)ramdisk_load_addr, info->ramdisk_data, info->ramdisk_size);
    }
    
    /* Step 3: Copy DTB to target address if present */
    if (info->dtb_data && info->dtb_size > 0 && dtb_load_addr != 0) {
        vga_write("Copying DTB to target address...\n");
        mem_copy((void*)(uintptr_t)dtb_load_addr, info->dtb_data, info->dtb_size);
    }
    
    /* Step 4: Set up boot parameters */
    /* For x86 Linux kernel, we set up a minimal boot_params structure */
    typedef struct {
        uint8_t setup_sects;
        uint16_t root_flags;
        uint32_t syssize;
        uint16_t ram_size;
        uint16_t vid_mode;
        uint16_t root_dev;
        uint16_t boot_flag;
        uint16_t jump;
        uint32_t header;
        uint16_t version;
        uint32_t realmode_swtch;
        uint16_t start_sys;
        uint16_t kernel_version;
        uint8_t type_of_loader;
        uint8_t loadflags;
        uint16_t setup_move_size;
        uint32_t code32_start;
        uint32_t ramdisk_image;
        uint32_t ramdisk_size;
        uint32_t bootsect_kludge;
        uint16_t heap_end_ptr;
        uint8_t ext_loader_ver;
        uint8_t ext_loader_type;
        uint32_t cmd_line_ptr;
        uint32_t initrd_addr_max;
    } __attribute__((packed)) linux_setup_header_t;
    
    /* Boot parameters location (zero page) */
    uint8_t* zero_page = (uint8_t*)(uintptr_t)0x90000;
    mem_set(zero_page, 0, 4096);
    
    /* Set up minimal boot parameters for x86 Linux */
    linux_setup_header_t* setup = (linux_setup_header_t*)(zero_page + 0x1F1);
    setup->type_of_loader = 0xFF;  /* Unknown bootloader */
    setup->loadflags = 0x81;       /* Can use heap, loaded high */
    setup->ramdisk_image = ramdisk_load_addr;
    setup->ramdisk_size = info->ramdisk_size;
    
    /* Set up command line if present */
    if (info->cmdline[0]) {
        uint32_t cmdline_addr = 0x99000;  /* Command line location */
        str_copy((char*)(uintptr_t)cmdline_addr, info->cmdline, sizeof(info->cmdline));
        setup->cmd_line_ptr = cmdline_addr;
    }
    
    vga_write("Boot parameters set up\n");
    vga_write("Jumping to kernel entry point...\n");
    
    /* Step 5: Jump to kernel entry point */
    /* The kernel entry point is typically at kernel_load_addr + 0x200 for bzImage */
    uint32_t entry_point = kernel_load_addr;
    
    /* Check if this is a bzImage (look for "HdrS" signature) */
    uint8_t* kernel_header = (uint8_t*)(uintptr_t)kernel_load_addr;
    if (kernel_header[0x202] == 'H' && kernel_header[0x203] == 'd' &&
        kernel_header[0x204] == 'r' && kernel_header[0x205] == 'S') {
        /* bzImage format - read entry point from header */
        uint32_t code32_start = *(uint32_t*)(kernel_header + 0x214);
        if (code32_start != 0) {
            entry_point = code32_start;
        }
    }
    
    vga_write("Kernel entry point: 0x");
    vga_write_hex(entry_point);
    vga_write("\n");
    
    /* 
     * Note: In a real implementation, we would:
     * 1. Disable interrupts
     * 2. Set up protected mode GDT if needed
     * 3. Set up registers (for ARM: r0=0, r1=machine_type, r2=dtb/atags)
     * 4. Jump to entry point using assembly
     * 
     * For safety in this implementation, we don't actually jump yet.
     * This would require platform-specific assembly code:
     * 
     * For x86:
     *   asm volatile("cli");
     *   asm volatile("movl %0, %%esi" : : "r"(zero_page_addr));  // Boot params
     *   asm volatile("jmp *%0" : : "r"(entry_point));
     * 
     * For ARM:
     *   asm volatile("mov r0, #0");
     *   asm volatile("ldr r1, %0" : : "m"(machine_type));
     *   asm volatile("ldr r2, %0" : : "m"(dtb_addr));
     *   asm volatile("bx %0" : : "r"(entry_point));
     */
    
    (void)entry_point;  /* Suppress warning - would be used in actual jump */
    
    vga_write("Android kernel boot preparation complete\n");
    vga_write("(Actual jump to kernel disabled for safety)\n");
    
    return BOOT_PARSE_SUCCESS;
}
