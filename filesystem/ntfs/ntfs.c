/**
 * Aurora OS - NTFS Read-Only File System Driver Implementation
 * 
 * NTFS read support for dual-boot scenarios with Windows
 * Note: This is a READ-ONLY implementation for compatibility
 */

#include "ntfs.h"
#include "../vfs/vfs.h"
#include "../../kernel/memory/memory.h"
#include <stddef.h>
#include <stdbool.h>

/* Global NTFS mount information */
static ntfs_mount_t g_ntfs_mount = {0};
static bool g_ntfs_mounted = false;

/* Buffer for sector reads */
static uint8_t sector_buffer[NTFS_SECTOR_SIZE];

/* Forward declarations */
static int ntfs_mount(const char* device);
static int ntfs_unmount(void);
static inode_t* ntfs_lookup(const char* path);
static int ntfs_readdir(inode_t* dir, dirent_t* entry, uint32_t index);

/* File system operations (read-only) */
static fs_ops_t ntfs_ops = {
    .mount = ntfs_mount,
    .unmount = ntfs_unmount,
    .lookup = ntfs_lookup,
    .create = NULL,     /* Read-only */
    .unlink = NULL,     /* Read-only */
    .readdir = ntfs_readdir,
    .mkdir = NULL,      /* Read-only */
    .rmdir = NULL,      /* Read-only */
    .chmod = NULL,      /* Read-only */
    .chown = NULL,      /* Read-only */
    .rename = NULL,     /* Read-only */
};

/**
 * Initialize NTFS driver
 */
void ntfs_init(void) {
    g_ntfs_mounted = false;
    g_ntfs_mount.device = NULL;
}

/**
 * Read MFT record
 */
int ntfs_read_mft_record(ntfs_mount_t* mount, uint64_t mft_num, void* buffer) {
    if (!mount || !buffer) {
        return -1;
    }
    
    /* Calculate MFT record location */
    uint64_t mft_offset = mft_num * mount->mft_record_size;
    uint64_t cluster = mount->mft_cluster;
    uint64_t sector = cluster * mount->sectors_per_cluster;
    
    /* Read MFT record from disk */
    /* In real implementation, would call storage driver to read sectors */
    /* This is simplified - would need to handle large MFT records */
    
    return 0;
}

/**
 * Find attribute in MFT record
 */
void* ntfs_find_attribute(ntfs_mft_record_t* mft, uint32_t attr_type) {
    if (!mft) {
        return NULL;
    }
    
    /* Get first attribute */
    uint8_t* attr_ptr = (uint8_t*)mft + mft->first_attr_offset;
    
    /* Iterate through attributes */
    while (*(uint32_t*)attr_ptr != NTFS_ATTR_END) {
        ntfs_attr_resident_t* attr = (ntfs_attr_resident_t*)attr_ptr;
        
        if (attr->type == attr_type) {
            return attr;
        }
        
        /* Move to next attribute */
        attr_ptr += attr->length;
        
        /* Bounds check */
        if ((uintptr_t)attr_ptr >= (uintptr_t)mft + mft->allocated_size) {
            break;
        }
    }
    
    return NULL;
}

/**
 * Read attribute data
 */
int ntfs_read_attribute(ntfs_mount_t* mount, ntfs_mft_record_t* mft, 
                        uint32_t attr_type, void* buffer, size_t size) {
    if (!mount || !mft || !buffer) {
        return -1;
    }
    
    /* Find attribute */
    ntfs_attr_resident_t* attr = (ntfs_attr_resident_t*)ntfs_find_attribute(mft, attr_type);
    if (!attr) {
        return -1;
    }
    
    /* Handle resident attribute */
    if (attr->non_resident == 0) {
        uint32_t data_size = attr->value_length;
        if (data_size > size) {
            data_size = size;
        }
        
        uint8_t* data_ptr = (uint8_t*)attr + attr->value_offset;
        for (uint32_t i = 0; i < data_size; i++) {
            ((uint8_t*)buffer)[i] = data_ptr[i];
        }
        
        return (int)data_size;
    }
    
    /* Handle non-resident attribute */
    ntfs_attr_nonresident_t* nr_attr = (ntfs_attr_nonresident_t*)attr;
    
    /* Read data runs (simplified implementation) */
    /* Real implementation would parse data run list and read clusters */
    
    return 0;
}

/**
 * Parse data runs for non-resident attributes
 */
static uint64_t ntfs_parse_data_runs(uint8_t* run_list, uint64_t vcn, 
                                     uint64_t* lcn, uint64_t* length) {
    /* Data run format: 
     * First byte: high nibble = length of length, low nibble = length of offset
     * Followed by length bytes (little endian)
     * Followed by offset bytes (little endian, signed)
     */
    
    uint8_t header = *run_list++;
    if (header == 0) {
        return 0;  /* End of run list */
    }
    
    uint8_t length_bytes = header & 0x0F;
    uint8_t offset_bytes = (header >> 4) & 0x0F;
    
    /* Read length */
    uint64_t run_length = 0;
    for (int i = 0; i < length_bytes; i++) {
        run_length |= ((uint64_t)run_list[i]) << (i * 8);
    }
    run_list += length_bytes;
    
    /* Read offset */
    int64_t run_offset = 0;
    for (int i = 0; i < offset_bytes; i++) {
        run_offset |= ((int64_t)run_list[i]) << (i * 8);
    }
    
    /* Sign extend if necessary */
    if (offset_bytes > 0 && (run_list[offset_bytes - 1] & 0x80)) {
        for (int i = offset_bytes; i < 8; i++) {
            run_offset |= 0xFFLL << (i * 8);
        }
    }
    
    *length = run_length;
    *lcn = (uint64_t)((int64_t)*lcn + run_offset);
    
    return run_length;
}

/**
 * Convert NTFS filename to UTF-8
 */
static void ntfs_filename_to_utf8(const uint16_t* utf16, uint8_t length, char* output) {
    /* Simplified UTF-16 to UTF-8 conversion */
    /* Only handles basic ASCII characters for compatibility */
    for (uint8_t i = 0; i < length && i < MAX_FILENAME_LENGTH - 1; i++) {
        if (utf16[i] < 0x80) {
            output[i] = (char)utf16[i];
        } else {
            output[i] = '?';  /* Non-ASCII character */
        }
    }
    output[length] = '\0';
}

/**
 * Mount NTFS file system
 */
static int ntfs_mount(const char* device) {
    if (g_ntfs_mounted) {
        return -1;  /* Already mounted */
    }
    
    /* Read boot sector */
    /* In real implementation, would call storage driver to read sector 0 */
    ntfs_boot_sector_t* boot_sector = (ntfs_boot_sector_t*)sector_buffer;
    
    /* Verify NTFS signature */
    bool valid = true;
    for (int i = 0; i < 8; i++) {
        if (boot_sector->oem_id[i] != NTFS_SIGNATURE[i]) {
            valid = false;
            break;
        }
    }
    
    if (!valid || boot_sector->end_marker != 0xAA55) {
        return -1;  /* Invalid NTFS volume */
    }
    
    /* Store mount information */
    g_ntfs_mount.bytes_per_sector = boot_sector->bytes_per_sector;
    g_ntfs_mount.sectors_per_cluster = boot_sector->sectors_per_cluster;
    g_ntfs_mount.bytes_per_cluster = g_ntfs_mount.bytes_per_sector * 
                                      g_ntfs_mount.sectors_per_cluster;
    g_ntfs_mount.mft_cluster = boot_sector->mft_cluster;
    g_ntfs_mount.mft_mirror_cluster = boot_sector->mft_mirror_cluster;
    g_ntfs_mount.clusters_per_mft_record = boot_sector->clusters_per_mft_record;
    
    /* Calculate MFT record size */
    if (g_ntfs_mount.clusters_per_mft_record >= 0) {
        g_ntfs_mount.mft_record_size = g_ntfs_mount.clusters_per_mft_record * 
                                        g_ntfs_mount.bytes_per_cluster;
    } else {
        g_ntfs_mount.mft_record_size = 1 << (-g_ntfs_mount.clusters_per_mft_record);
    }
    
    g_ntfs_mount.device = (void*)device;
    g_ntfs_mounted = true;
    
    return 0;
}

/**
 * Unmount NTFS file system
 */
static int ntfs_unmount(void) {
    if (!g_ntfs_mounted) {
        return -1;
    }
    
    g_ntfs_mounted = false;
    g_ntfs_mount.device = NULL;
    
    return 0;
}

/**
 * Lookup file or directory
 */
static inode_t* ntfs_lookup(const char* path) {
    if (!g_ntfs_mounted || !path) {
        return NULL;
    }
    
    /* Root directory is MFT record 5 */
    uint64_t mft_num = 5;
    
    /* Handle root directory */
    if (path[0] == '/' && path[1] == '\0') {
        inode_t* root_inode = (inode_t*)kmalloc(sizeof(inode_t));
        if (!root_inode) return NULL;
        
        root_inode->ino = mft_num;
        root_inode->type = FILE_TYPE_DIRECTORY;
        root_inode->size = 0;
        root_inode->mode = S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
        root_inode->fs_data = (void*)(uintptr_t)mft_num;
        
        return root_inode;
    }
    
    /* Parse path and traverse directories */
    /* Simplified implementation - would need full path parsing and index traversal */
    
    return NULL;  /* Not found */
}

/**
 * Read directory entries
 */
static int ntfs_readdir(inode_t* dir, dirent_t* entry, uint32_t index) {
    if (!g_ntfs_mounted || !dir || !entry) {
        return -1;
    }
    
    if (dir->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Get directory MFT number */
    uint64_t mft_num = (uint64_t)(uintptr_t)dir->fs_data;
    
    /* Read MFT record */
    uint8_t mft_buffer[NTFS_MFT_ENTRY_SIZE];
    if (ntfs_read_mft_record(&g_ntfs_mount, mft_num, mft_buffer) != 0) {
        return -1;
    }
    
    /* Parse directory index */
    /* Simplified - would need to parse INDEX_ROOT and INDEX_ALLOCATION attributes */
    
    return -1;  /* End of directory */
}

/**
 * Get NTFS file system operations
 */
fs_ops_t* ntfs_get_ops(void) {
    return &ntfs_ops;
}

/**
 * Mount NTFS device (convenience function)
 */
int ntfs_mount_device(const char* device) {
    return ntfs_mount(device);
}

/**
 * Unmount NTFS device (convenience function)
 */
int ntfs_unmount_device(void) {
    return ntfs_unmount();
}
