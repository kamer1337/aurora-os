/**
 * Aurora OS - FAT32 File System Driver Implementation
 * 
 * FAT32 file system driver for compatibility with Windows and removable media
 */

#include "fat32.h"
#include "../vfs/vfs.h"
#include "../../kernel/memory/memory.h"
#include <stddef.h>
#include <stdbool.h>

/* Global FAT32 mount information */
static fat32_mount_t g_fat32_mount = {0};
static bool g_fat32_mounted = false;

/* Buffer for sector reads */
static uint8_t sector_buffer[FAT32_SECTOR_SIZE];

/* Forward declarations */
static int fat32_mount(const char* device);
static int fat32_unmount(void);
static inode_t* fat32_lookup(const char* path);
static int fat32_create(const char* path, file_type_t type);
static int fat32_unlink(const char* path);
static int fat32_readdir(inode_t* dir, dirent_t* entry, uint32_t index);
static int fat32_mkdir(const char* path, uint16_t mode);
static int fat32_rmdir(const char* path);
static int fat32_rename(const char* oldpath, const char* newpath);

/* File system operations */
static fs_ops_t fat32_ops = {
    .mount = fat32_mount,
    .unmount = fat32_unmount,
    .lookup = fat32_lookup,
    .create = fat32_create,
    .unlink = fat32_unlink,
    .readdir = fat32_readdir,
    .mkdir = fat32_mkdir,
    .rmdir = fat32_rmdir,
    .chmod = NULL,  /* FAT32 doesn't support Unix permissions */
    .chown = NULL,  /* FAT32 doesn't support ownership */
    .rename = fat32_rename,
};

/**
 * Initialize FAT32 driver
 */
void fat32_init(void) {
    g_fat32_mounted = false;
    g_fat32_mount.device = NULL;
}

/**
 * Convert cluster number to sector number
 */
uint32_t fat32_cluster_to_sector(fat32_mount_t* mount, uint32_t cluster) {
    if (cluster < 2) {
        return 0;  /* Invalid cluster */
    }
    return mount->first_data_sector + (cluster - 2) * mount->sectors_per_cluster;
}

/**
 * Get FAT entry for a cluster
 */
uint32_t fat32_get_fat_entry(fat32_mount_t* mount, uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = mount->first_fat_sector + (fat_offset / FAT32_SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    /* Read FAT sector (simplified - would use block device driver) */
    /* In real implementation, would call storage driver to read sector */
    
    /* Extract FAT entry (28 bits, mask off top 4 bits) */
    uint32_t entry = *(uint32_t*)(sector_buffer + entry_offset);
    return entry & 0x0FFFFFFF;
}

/**
 * Set FAT entry for a cluster
 */
int fat32_set_fat_entry(fat32_mount_t* mount, uint32_t cluster, uint32_t value) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = mount->first_fat_sector + (fat_offset / FAT32_SECTOR_SIZE);
    uint32_t entry_offset = fat_offset % FAT32_SECTOR_SIZE;
    
    /* Read FAT sector */
    /* In real implementation, would call storage driver */
    
    /* Update FAT entry (preserve top 4 bits) */
    uint32_t* entry_ptr = (uint32_t*)(sector_buffer + entry_offset);
    *entry_ptr = (*entry_ptr & 0xF0000000) | (value & 0x0FFFFFFF);
    
    /* Write FAT sector back */
    /* In real implementation, would call storage driver */
    
    return 0;
}

/**
 * Allocate a new cluster
 */
uint32_t fat32_allocate_cluster(fat32_mount_t* mount) {
    /* Search for free cluster starting from cluster 2 */
    for (uint32_t cluster = 2; cluster < mount->total_clusters; cluster++) {
        uint32_t entry = fat32_get_fat_entry(mount, cluster);
        if (entry == FAT32_FREE_CLUSTER) {
            /* Mark cluster as end of chain */
            fat32_set_fat_entry(mount, cluster, FAT32_EOC_MAX);
            return cluster;
        }
    }
    return 0;  /* No free clusters */
}

/**
 * Free a cluster chain
 */
int fat32_free_cluster_chain(fat32_mount_t* mount, uint32_t start_cluster) {
    uint32_t cluster = start_cluster;
    
    while (cluster >= 2 && cluster < FAT32_EOC_MIN) {
        uint32_t next_cluster = fat32_get_fat_entry(mount, cluster);
        fat32_set_fat_entry(mount, cluster, FAT32_FREE_CLUSTER);
        cluster = next_cluster;
    }
    
    return 0;
}

/**
 * Convert FAT32 attributes to VFS file type
 */
static file_type_t fat32_attr_to_type(uint8_t attributes) {
    if (attributes & FAT32_ATTR_DIRECTORY) {
        return FILE_TYPE_DIRECTORY;
    }
    return FILE_TYPE_REGULAR;
}

/**
 * Parse short filename (8.3 format)
 */
static void fat32_parse_short_name(const uint8_t* short_name, char* output) {
    int out_idx = 0;
    
    /* Copy filename (8 characters) */
    for (int i = 0; i < 8; i++) {
        if (short_name[i] != ' ') {
            output[out_idx++] = short_name[i];
        }
    }
    
    /* Add extension if present */
    if (short_name[8] != ' ') {
        output[out_idx++] = '.';
        for (int i = 8; i < 11; i++) {
            if (short_name[i] != ' ') {
                output[out_idx++] = short_name[i];
            }
        }
    }
    
    output[out_idx] = '\0';
}

/**
 * Mount FAT32 file system
 */
static int fat32_mount(const char* device) {
    if (g_fat32_mounted) {
        return -1;  /* Already mounted */
    }
    
    /* Read boot sector (simplified - would use storage driver) */
    /* In real implementation, would call storage driver to read sector 0 */
    fat32_boot_sector_t* boot_sector = (fat32_boot_sector_t*)sector_buffer;
    
    /* Verify FAT32 signature */
    if (boot_sector->boot_signature != FAT32_BOOT_SIGNATURE) {
        return -1;  /* Invalid boot sector */
    }
    
    /* Calculate file system parameters */
    g_fat32_mount.first_fat_sector = boot_sector->reserved_sector_count;
    g_fat32_mount.fat_size = boot_sector->fat_size_32;
    g_fat32_mount.num_fats = boot_sector->num_fats;
    g_fat32_mount.first_data_sector = g_fat32_mount.first_fat_sector + 
                                      (g_fat32_mount.num_fats * g_fat32_mount.fat_size);
    g_fat32_mount.root_cluster = boot_sector->root_cluster;
    g_fat32_mount.sectors_per_cluster = boot_sector->sectors_per_cluster;
    g_fat32_mount.bytes_per_cluster = boot_sector->sectors_per_cluster * 
                                      boot_sector->bytes_per_sector;
    
    uint32_t total_sectors = boot_sector->total_sectors_32;
    uint32_t data_sectors = total_sectors - g_fat32_mount.first_data_sector;
    g_fat32_mount.total_clusters = data_sectors / g_fat32_mount.sectors_per_cluster;
    
    g_fat32_mount.device = (void*)device;
    g_fat32_mounted = true;
    
    return 0;
}

/**
 * Unmount FAT32 file system
 */
static int fat32_unmount(void) {
    if (!g_fat32_mounted) {
        return -1;
    }
    
    g_fat32_mounted = false;
    g_fat32_mount.device = NULL;
    
    return 0;
}

/**
 * Lookup file or directory
 */
static inode_t* fat32_lookup(const char* path) {
    if (!g_fat32_mounted || !path) {
        return NULL;
    }
    
    /* Start from root directory */
    uint32_t current_cluster = g_fat32_mount.root_cluster;
    
    /* Handle root directory */
    if (path[0] == '/' && path[1] == '\0') {
        inode_t* root_inode = (inode_t*)kmalloc(sizeof(inode_t));
        if (!root_inode) return NULL;
        
        root_inode->ino = current_cluster;
        root_inode->type = FILE_TYPE_DIRECTORY;
        root_inode->size = 0;
        root_inode->mode = DEFAULT_DIR_MODE;
        root_inode->fs_data = (void*)(uintptr_t)current_cluster;
        
        return root_inode;
    }
    
    /* Parse path and traverse directories */
    /* Simplified implementation - would need full path parsing */
    
    return NULL;  /* Not found */
}

/**
 * Create file or directory
 */
static int fat32_create(const char* path, file_type_t type) {
    if (!g_fat32_mounted || !path) {
        return -1;
    }
    
    /* Allocate cluster for new file/directory */
    uint32_t new_cluster = fat32_allocate_cluster(&g_fat32_mount);
    if (new_cluster == 0) {
        return -1;  /* No space */
    }
    
    /* Create directory entry in parent directory */
    /* Simplified - would need to find parent directory and add entry */
    
    return 0;
}

/**
 * Delete file or directory
 */
static int fat32_unlink(const char* path) {
    if (!g_fat32_mounted || !path) {
        return -1;
    }
    
    /* Find file entry */
    inode_t* inode = fat32_lookup(path);
    if (!inode) {
        return -1;
    }
    
    /* Free cluster chain */
    uint32_t cluster = (uint32_t)(uintptr_t)inode->fs_data;
    fat32_free_cluster_chain(&g_fat32_mount, cluster);
    
    /* Remove directory entry from parent */
    /* Simplified - would need to update parent directory */
    
    kfree(inode);
    return 0;
}

/**
 * Read directory entries
 */
static int fat32_readdir(inode_t* dir, dirent_t* entry, uint32_t index) {
    if (!g_fat32_mounted || !dir || !entry) {
        return -1;
    }
    
    if (dir->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Get directory cluster */
    uint32_t cluster = (uint32_t)(uintptr_t)dir->fs_data;
    
    /* Read directory entries from cluster */
    /* Simplified - would need to read directory sectors and parse entries */
    
    return -1;  /* End of directory */
}

/**
 * Create directory
 */
static int fat32_mkdir(const char* path, uint16_t mode) {
    (void)mode;  /* FAT32 doesn't support Unix permissions */
    return fat32_create(path, FILE_TYPE_DIRECTORY);
}

/**
 * Remove directory
 */
static int fat32_rmdir(const char* path) {
    /* Check if directory is empty */
    inode_t* dir = fat32_lookup(path);
    if (!dir || dir->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Verify directory is empty */
    /* Simplified - would need to check for entries */
    
    return fat32_unlink(path);
}

/**
 * Rename file or directory
 */
static int fat32_rename(const char* oldpath, const char* newpath) {
    if (!g_fat32_mounted || !oldpath || !newpath) {
        return -1;
    }
    
    /* Find old entry and update name */
    /* Simplified - would need to update directory entry */
    
    return 0;
}

/**
 * Get FAT32 file system operations
 */
fs_ops_t* fat32_get_ops(void) {
    return &fat32_ops;
}

/**
 * Mount FAT32 device (convenience function)
 */
int fat32_mount_device(const char* device) {
    return fat32_mount(device);
}

/**
 * Unmount FAT32 device (convenience function)
 */
int fat32_unmount_device(void) {
    return fat32_unmount();
}
