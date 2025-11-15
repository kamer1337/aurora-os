/**
 * Aurora OS - Ramdisk File System Header
 * 
 * Simple in-memory file system for initial ramdisk
 */

#ifndef AURORA_RAMDISK_H
#define AURORA_RAMDISK_H

#include <stdint.h>
#include <stddef.h>
#include "../vfs/vfs.h"

/* Ramdisk configuration */
#define RAMDISK_MAX_FILES 128
#define RAMDISK_BLOCK_SIZE 512
#define RAMDISK_MAX_BLOCKS 2048

/* Ramdisk inode structure */
typedef struct ramdisk_inode {
    uint32_t ino;
    file_type_t type;
    uint32_t size;
    uint32_t blocks;
    uint32_t block_list[32]; /* Direct blocks */
    uint8_t used;
} ramdisk_inode_t;

/* Ramdisk superblock */
typedef struct ramdisk_superblock {
    uint32_t magic;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t total_inodes;
    uint32_t free_inodes;
} ramdisk_superblock_t;

/* Ramdisk initialization */
void ramdisk_init(void);
int ramdisk_create(size_t size);

/* Get ramdisk file system operations */
fs_ops_t* ramdisk_get_ops(void);

#endif /* AURORA_RAMDISK_H */
