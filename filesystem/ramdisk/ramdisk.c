/**
 * Aurora OS - Ramdisk File System Implementation
 * 
 * Simple in-memory file system
 */

#include "ramdisk.h"
#include "../../kernel/memory/memory.h"
#include <stddef.h>

/* Ramdisk data structures */
static ramdisk_superblock_t superblock;
static ramdisk_inode_t inodes[RAMDISK_MAX_FILES];
static uint8_t* data_blocks = NULL;
static uint8_t block_bitmap[RAMDISK_MAX_BLOCKS / 8];

/* File name storage */
#define MAX_FILENAME 64
typedef struct ramdisk_file {
    uint32_t inode_num;
    char name[MAX_FILENAME];
    uint8_t used;
} ramdisk_file_t;

static ramdisk_file_t file_table[RAMDISK_MAX_FILES];

/* Forward declarations */
static int ramdisk_mount(const char* device);
static int ramdisk_unmount(void);
static inode_t* ramdisk_lookup(const char* path);
static int ramdisk_create_file(const char* path, file_type_t type);
static int ramdisk_unlink(const char* path);
static int ramdisk_read(inode_t* inode, void* buffer, size_t size, uint32_t offset);
static int ramdisk_write(inode_t* inode, const void* buffer, size_t size, uint32_t offset);
static int ramdisk_readdir(inode_t* dir, dirent_t* entry, uint32_t index);

/* File operations */
static file_ops_t ramdisk_file_ops = {
    .open = NULL,
    .close = NULL,
    .read = ramdisk_read,
    .write = ramdisk_write
};

/* File system operations */
static fs_ops_t ramdisk_ops = {
    .mount = ramdisk_mount,
    .unmount = ramdisk_unmount,
    .lookup = ramdisk_lookup,
    .create = ramdisk_create_file,
    .unlink = ramdisk_unlink,
    .readdir = ramdisk_readdir
};

/**
 * Allocate a data block
 */
static int alloc_block(void) {
    for (uint32_t i = 0; i < RAMDISK_MAX_BLOCKS / 8; i++) {
        if (block_bitmap[i] != 0xFF) {
            for (uint32_t j = 0; j < 8; j++) {
                if (!(block_bitmap[i] & (1 << j))) {
                    block_bitmap[i] |= (1 << j);
                    superblock.free_blocks--;
                    return i * 8 + j;
                }
            }
        }
    }
    return -1;
}

/**
 * Free a data block
 */
static void free_block(uint32_t block_num) {
    if (block_num >= RAMDISK_MAX_BLOCKS) {
        return;
    }
    uint32_t byte = block_num / 8;
    uint32_t bit = block_num % 8;
    block_bitmap[byte] &= ~(1 << bit);
    superblock.free_blocks++;
}

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
        for (int j = 0; j < 32; j++) {
            inodes[i].block_list[j] = 0;
        }
    }
    
    /* Clear block bitmap */
    for (int i = 0; i < RAMDISK_MAX_BLOCKS / 8; i++) {
        block_bitmap[i] = 0;
    }
    
    /* Initialize file table */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        file_table[i].used = 0;
        file_table[i].inode_num = 0;
        for (int j = 0; j < MAX_FILENAME; j++) {
            file_table[i].name[j] = 0;
        }
    }
}

/**
 * Create ramdisk with specified size
 */
int ramdisk_create(size_t size) {
    ramdisk_init();
    
    /* Allocate data blocks memory */
    data_blocks = (uint8_t*)kmalloc(RAMDISK_MAX_BLOCKS * RAMDISK_BLOCK_SIZE);
    if (!data_blocks) {
        return -1;
    }
    
    /* Clear data blocks */
    for (uint32_t i = 0; i < RAMDISK_MAX_BLOCKS * RAMDISK_BLOCK_SIZE; i++) {
        data_blocks[i] = 0;
    }
    
    return 0;
}

/**
 * Get ramdisk operations
 */
fs_ops_t* ramdisk_get_ops(void) {
    return &ramdisk_ops;
}

/**
 * Get ramdisk file operations
 */
file_ops_t* ramdisk_get_file_ops(void) {
    return &ramdisk_file_ops;
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
        /* Free all data blocks */
        for (uint32_t i = 0; i < inode->blocks && i < 32; i++) {
            free_block(inode->block_list[i]);
        }
        
        inode->used = 0;
        inode->size = 0;
        inode->blocks = 0;
        superblock.free_inodes++;
    }
}

/**
 * String comparison helper
 */
static int str_equal(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b;
}

/**
 * String copy helper
 */
static void str_copy(char* dest, const char* src, int max_len) {
    int i = 0;
    while (src[i] && i < max_len - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/**
 * Find inode by path
 */
static ramdisk_inode_t* find_inode_by_path(const char* path) {
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }
    
    /* If empty path, return root */
    if (path[0] == '\0') {
        if (inodes[0].used) {
            return &inodes[0];
        }
        return NULL;
    }
    
    /* Search for matching file name */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (file_table[i].used && str_equal(file_table[i].name, path)) {
            return &inodes[file_table[i].inode_num];
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
    
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
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
    
    /* Find free file table entry */
    int file_idx = -1;
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (!file_table[i].used) {
            file_idx = i;
            break;
        }
    }
    
    if (file_idx == -1) {
        free_inode(inode);
        return -1;
    }
    
    /* Store file name */
    file_table[file_idx].used = 1;
    file_table[file_idx].inode_num = inode->ino;
    str_copy(file_table[file_idx].name, path, MAX_FILENAME);
    
    return 0;
}

/**
 * Delete file from ramdisk
 */
static int ramdisk_unlink(const char* path) {
    if (!path) {
        return -1;
    }
    
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }
    
    ramdisk_inode_t* inode = find_inode_by_path(path);
    if (!inode) {
        return -1; /* File not found */
    }
    
    /* Remove from file table */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (file_table[i].used && str_equal(file_table[i].name, path)) {
            file_table[i].used = 0;
            break;
        }
    }
    
    /* Free inode */
    free_inode(inode);
    
    return 0;
}

/**
 * Read from ramdisk file
 */
static int ramdisk_read(inode_t* inode, void* buffer, size_t size, uint32_t offset) {
    if (!inode || !buffer || !inode->fs_data) {
        return -1;
    }
    
    ramdisk_inode_t* rd_inode = (ramdisk_inode_t*)inode->fs_data;
    
    /* Check bounds */
    if (offset >= rd_inode->size) {
        return 0;
    }
    
    /* Adjust size to not read past end of file */
    if (offset + size > rd_inode->size) {
        size = rd_inode->size - offset;
    }
    
    uint8_t* buf = (uint8_t*)buffer;
    size_t bytes_read = 0;
    
    /* Read data from blocks */
    while (bytes_read < size) {
        uint32_t block_idx = (offset + bytes_read) / RAMDISK_BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_read) % RAMDISK_BLOCK_SIZE;
        uint32_t to_read = RAMDISK_BLOCK_SIZE - block_offset;
        
        if (to_read > size - bytes_read) {
            to_read = size - bytes_read;
        }
        
        if (block_idx >= rd_inode->blocks) {
            break;
        }
        
        uint32_t block_num = rd_inode->block_list[block_idx];
        uint8_t* block_data = data_blocks + (block_num * RAMDISK_BLOCK_SIZE);
        
        for (uint32_t i = 0; i < to_read; i++) {
            buf[bytes_read + i] = block_data[block_offset + i];
        }
        
        bytes_read += to_read;
    }
    
    return (int)bytes_read;
}

/**
 * Write to ramdisk file
 */
static int ramdisk_write(inode_t* inode, const void* buffer, size_t size, uint32_t offset) {
    if (!inode || !buffer || !inode->fs_data) {
        return -1;
    }
    
    ramdisk_inode_t* rd_inode = (ramdisk_inode_t*)inode->fs_data;
    const uint8_t* buf = (const uint8_t*)buffer;
    size_t bytes_written = 0;
    
    /* Write data to blocks */
    while (bytes_written < size) {
        uint32_t block_idx = (offset + bytes_written) / RAMDISK_BLOCK_SIZE;
        uint32_t block_offset = (offset + bytes_written) % RAMDISK_BLOCK_SIZE;
        uint32_t to_write = RAMDISK_BLOCK_SIZE - block_offset;
        
        if (to_write > size - bytes_written) {
            to_write = size - bytes_written;
        }
        
        /* Allocate block if needed */
        if (block_idx >= rd_inode->blocks) {
            if (block_idx >= 32) {
                break; /* Maximum blocks reached */
            }
            
            int new_block = alloc_block();
            if (new_block < 0) {
                break; /* No free blocks */
            }
            
            rd_inode->block_list[block_idx] = new_block;
            rd_inode->blocks++;
        }
        
        uint32_t block_num = rd_inode->block_list[block_idx];
        uint8_t* block_data = data_blocks + (block_num * RAMDISK_BLOCK_SIZE);
        
        for (uint32_t i = 0; i < to_write; i++) {
            block_data[block_offset + i] = buf[bytes_written + i];
        }
        
        bytes_written += to_write;
    }
    
    /* Update file size if we wrote past end */
    if (offset + bytes_written > rd_inode->size) {
        rd_inode->size = offset + bytes_written;
        inode->size = rd_inode->size;
    }
    
    return (int)bytes_written;
}

/**
 * Read directory entries
 */
static int ramdisk_readdir(inode_t* dir, dirent_t* entry, uint32_t index) {
    if (!dir || !entry) {
        return -1;
    }
    
    /* Verify this is a directory */
    if (dir->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Special case for root directory - list all files */
    if (dir->ino == 0) {
        uint32_t count = 0;
        
        /* Iterate through file table to find entries */
        for (uint32_t i = 0; i < RAMDISK_MAX_FILES; i++) {
            if (file_table[i].used) {
                if (count == index) {
                    /* Found the entry at the requested index */
                    entry->ino = file_table[i].inode_num;
                    
                    /* Copy filename */
                    int j = 0;
                    while (file_table[i].name[j] && j < MAX_FILENAME_LENGTH - 1) {
                        entry->name[j] = file_table[i].name[j];
                        j++;
                    }
                    entry->name[j] = '\0';
                    
                    /* Get file type from inode */
                    if (inodes[file_table[i].inode_num].used) {
                        entry->type = inodes[file_table[i].inode_num].type;
                    } else {
                        entry->type = FILE_TYPE_REGULAR;
                    }
                    
                    return 0; /* Success */
                }
                count++;
            }
        }
        
        /* Index out of range */
        return -1;
    }
    
    /* For non-root directories, we don't support subdirectories yet */
    return -1;
}
