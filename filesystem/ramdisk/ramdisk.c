/**
 * Aurora OS - Ramdisk File System Implementation
 * 
 * Simple in-memory file system
 */

#include "ramdisk.h"
#include <stddef.h>

/* Ramdisk data structures */
static ramdisk_superblock_t superblock;
static ramdisk_inode_t inodes[RAMDISK_MAX_FILES];
static uint8_t data_blocks[RAMDISK_MAX_BLOCKS][RAMDISK_BLOCK_SIZE];
static uint8_t block_bitmap[RAMDISK_MAX_BLOCKS / 8];

/* Forward declarations */
static int ramdisk_mount(const char* device);
static int ramdisk_unmount(void);
static inode_t* ramdisk_lookup(const char* path);
static int ramdisk_create_file(const char* path, file_type_t type);
static int ramdisk_unlink(const char* path);

/* File system operations */
static fs_ops_t ramdisk_ops = {
    .mount = ramdisk_mount,
    .unmount = ramdisk_unmount,
    .lookup = ramdisk_lookup,
    .create = ramdisk_create_file,
    .unlink = ramdisk_unlink
};

/**
 * Initialize ramdisk subsystem
 */
void ramdisk_init(void) {
    /* Clear all data structures */
    superblock.magic = 0x52414D44; /* 'RAMD' */
    superblock.total_blocks = RAMDISK_MAX_BLOCKS;
    superblock.free_blocks = RAMDISK_MAX_BLOCKS;
    superblock.total_inodes = RAMDISK_MAX_FILES;
    superblock.free_inodes = RAMDISK_MAX_FILES;
    
    /* Initialize inodes */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        inodes[i].used = 0;
        inodes[i].ino = i;
        inodes[i].type = FILE_TYPE_REGULAR;
        inodes[i].size = 0;
        inodes[i].blocks = 0;
    }
    
    /* Clear block bitmap */
    for (int i = 0; i < RAMDISK_MAX_BLOCKS / 8; i++) {
        block_bitmap[i] = 0;
    }
}

/**
 * Create ramdisk with specified size
 */
int ramdisk_create(size_t size) {
    ramdisk_init();
    return 0;
}

/**
 * Get ramdisk operations
 */
fs_ops_t* ramdisk_get_ops(void) {
    return &ramdisk_ops;
}

/**
 * Allocate a free inode
 */
static ramdisk_inode_t* alloc_inode(void) {
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (!inodes[i].used) {
            inodes[i].used = 1;
            superblock.free_inodes--;
            return &inodes[i];
        }
    }
    return NULL;
}

/**
 * Free an inode
 */
static void free_inode(ramdisk_inode_t* inode) {
    if (inode && inode->used) {
        inode->used = 0;
        inode->size = 0;
        inode->blocks = 0;
        superblock.free_inodes++;
    }
}

/**
 * Find inode by path (simplified - no directory support yet)
 */
static ramdisk_inode_t* find_inode_by_path(const char* path) {
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }
    
    /* Search for matching inode (simplified filename matching) */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (inodes[i].used) {
            /* TODO: Store filename with inode */
            /* For now, just return first used inode for testing */
            return &inodes[i];
        }
    }
    return NULL;
}

/**
 * Mount ramdisk
 */
static int ramdisk_mount(const char* device) {
    (void)device; /* Unused for ramdisk */
    
    /* Create root directory if it doesn't exist */
    if (inodes[0].used == 0) {
        inodes[0].used = 1;
        inodes[0].ino = 0;
        inodes[0].type = FILE_TYPE_DIRECTORY;
        inodes[0].size = 0;
        inodes[0].blocks = 0;
        superblock.free_inodes--;
    }
    
    return 0;
}

/**
 * Unmount ramdisk
 */
static int ramdisk_unmount(void) {
    /* Nothing to do for ramdisk */
    return 0;
}

/**
 * Lookup file in ramdisk
 */
static inode_t* ramdisk_lookup(const char* path) {
    if (!path) {
        return NULL;
    }
    
    ramdisk_inode_t* rd_inode = find_inode_by_path(path);
    if (!rd_inode) {
        return NULL;
    }
    
    /* Convert ramdisk inode to generic inode */
    static inode_t inode;
    inode.ino = rd_inode->ino;
    inode.type = rd_inode->type;
    inode.size = rd_inode->size;
    inode.links = 1;
    inode.blocks = rd_inode->blocks;
    inode.fs_data = rd_inode;
    
    return &inode;
}

/**
 * Create file in ramdisk
 */
static int ramdisk_create_file(const char* path, file_type_t type) {
    if (!path) {
        return -1;
    }
    
    /* Check if file already exists */
    if (find_inode_by_path(path)) {
        return -1; /* File exists */
    }
    
    /* Allocate new inode */
    ramdisk_inode_t* inode = alloc_inode();
    if (!inode) {
        return -1; /* No free inodes */
    }
    
    /* Initialize inode */
    inode->type = type;
    inode->size = 0;
    inode->blocks = 0;
    
    return 0;
}

/**
 * Delete file from ramdisk
 */
static int ramdisk_unlink(const char* path) {
    if (!path) {
        return -1;
    }
    
    ramdisk_inode_t* inode = find_inode_by_path(path);
    if (!inode) {
        return -1; /* File not found */
    }
    
    /* Free inode */
    free_inode(inode);
    
    return 0;
}
