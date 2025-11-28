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
    uint32_t parent_ino;     /* Parent directory inode */
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
static int ramdisk_mkdir(const char* path, uint16_t mode);
static int ramdisk_rmdir(const char* path);
static int ramdisk_chmod(const char* path, uint16_t mode);
static int ramdisk_chown(const char* path, uint16_t uid, uint16_t gid);
static int ramdisk_rename(const char* oldpath, const char* newpath);

/* File operations */
static file_ops_t ramdisk_file_ops = {
    .open = NULL,
    .close = NULL,
    .read = ramdisk_read,
    .write = ramdisk_write
};

/* File system operations with extended support */
static fs_ops_t ramdisk_ops = {
    .mount = ramdisk_mount,
    .unmount = ramdisk_unmount,
    .lookup = ramdisk_lookup,
    .create = ramdisk_create_file,
    .unlink = ramdisk_unlink,
    .readdir = ramdisk_readdir,
    .mkdir = ramdisk_mkdir,
    .rmdir = ramdisk_rmdir,
    .chmod = ramdisk_chmod,
    .chown = ramdisk_chown,
    .rename = ramdisk_rename
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
    
    /* Initialize inodes with permission fields */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        inodes[i].used = 0;
        inodes[i].ino = i;
        inodes[i].type = FILE_TYPE_REGULAR;
        inodes[i].size = 0;
        inodes[i].blocks = 0;
        inodes[i].mode = DEFAULT_FILE_MODE;
        inodes[i].uid = 0;
        inodes[i].gid = 0;
        inodes[i].parent_ino = 0;
        inodes[i].atime = 0;
        inodes[i].mtime = 0;
        inodes[i].ctime = 0;
        inodes[i].child_count = 0;
        for (int j = 0; j < 32; j++) {
            inodes[i].block_list[j] = 0;
        }
        for (int j = 0; j < 64; j++) {
            inodes[i].children[j] = 0;
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
        file_table[i].parent_ino = 0;
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
        inodes[0].mode = DEFAULT_DIR_MODE;
        inodes[0].uid = 0;
        inodes[0].gid = 0;
        inodes[0].parent_ino = 0; /* Root's parent is itself */
        inodes[0].child_count = 0;
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
    
    /* Convert ramdisk inode to generic inode with all fields */
    static inode_t inode;
    inode.ino = rd_inode->ino;
    inode.type = rd_inode->type;
    inode.size = rd_inode->size;
    inode.links = 1;
    inode.blocks = rd_inode->blocks;
    inode.mode = rd_inode->mode;
    inode.uid = rd_inode->uid;
    inode.gid = rd_inode->gid;
    inode.atime = rd_inode->atime;
    inode.mtime = rd_inode->mtime;
    inode.ctime = rd_inode->ctime;
    inode.parent_ino = rd_inode->parent_ino;
    inode.fs_data = rd_inode;
    
    return &inode;
}

/**
 * Helper: Find parent directory for a path
 */
static ramdisk_inode_t* find_parent_directory(const char* path, char* child_name, size_t child_name_size) {
    if (!path || !child_name || child_name_size == 0) {
        return NULL;
    }
    
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }
    
    /* Find last slash to separate parent from child */
    size_t len = 0;
    size_t last_slash = 0;
    int has_slash = 0;
    
    while (path[len]) {
        if (path[len] == '/') {
            last_slash = len;
            has_slash = 1;
        }
        len++;
    }
    
    if (!has_slash) {
        /* No subdirectory, parent is root */
        size_t i = 0;
        while (path[i] && i < child_name_size - 1) {
            child_name[i] = path[i];
            i++;
        }
        child_name[i] = '\0';
        return &inodes[0]; /* Root directory */
    }
    
    /* Extract child name */
    size_t child_start = last_slash + 1;
    size_t j = 0;
    while (path[child_start + j] && j < child_name_size - 1) {
        child_name[j] = path[child_start + j];
        j++;
    }
    child_name[j] = '\0';
    
    /* Look up parent directory path */
    char parent_path[MAX_FILENAME];
    for (size_t i = 0; i < last_slash && i < MAX_FILENAME - 1; i++) {
        parent_path[i] = path[i];
    }
    parent_path[last_slash] = '\0';
    
    return find_inode_by_path(parent_path);
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
    
    /* Find parent directory */
    char child_name[MAX_FILENAME];
    ramdisk_inode_t* parent = find_parent_directory(path, child_name, MAX_FILENAME);
    if (!parent || parent->type != FILE_TYPE_DIRECTORY) {
        return -1; /* Parent doesn't exist or is not a directory */
    }
    
    /* Allocate new inode */
    ramdisk_inode_t* inode = alloc_inode();
    if (!inode) {
        return -1; /* No free inodes */
    }
    
    /* Initialize inode with permissions */
    inode->type = type;
    inode->size = 0;
    inode->blocks = 0;
    inode->mode = (type == FILE_TYPE_DIRECTORY) ? DEFAULT_DIR_MODE : DEFAULT_FILE_MODE;
    inode->uid = 0;
    inode->gid = 0;
    inode->parent_ino = parent->ino;
    inode->child_count = 0;
    
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
    
    /* Store file name and parent reference */
    file_table[file_idx].used = 1;
    file_table[file_idx].inode_num = inode->ino;
    file_table[file_idx].parent_ino = parent->ino;
    str_copy(file_table[file_idx].name, path, MAX_FILENAME);
    
    /* Add to parent's children list */
    if (parent->child_count < 64) {
        parent->children[parent->child_count++] = inode->ino;
    }
    
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
    
    /* Don't allow unlinking directories - use rmdir */
    if (inode->type == FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Remove from parent's children list */
    ramdisk_inode_t* parent = &inodes[inode->parent_ino];
    if (parent->used) {
        for (uint32_t i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == inode->ino) {
                /* Shift remaining children */
                for (uint32_t j = i; j < parent->child_count - 1; j++) {
                    parent->children[j] = parent->children[j + 1];
                }
                parent->child_count--;
                break;
            }
        }
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
    
    /* For non-root directories, use the children list */
    ramdisk_inode_t* rd_dir = (ramdisk_inode_t*)dir->fs_data;
    if (!rd_dir || rd_dir->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    if (index >= rd_dir->child_count) {
        return -1; /* Index out of range */
    }
    
    uint32_t child_ino = rd_dir->children[index];
    if (child_ino >= RAMDISK_MAX_FILES || !inodes[child_ino].used) {
        return -1;
    }
    
    /* Find file table entry for this inode */
    for (uint32_t i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (file_table[i].used && file_table[i].inode_num == child_ino) {
            entry->ino = child_ino;
            entry->type = inodes[child_ino].type;
            
            /* Extract just the filename (last component of path) */
            const char* full_path = file_table[i].name;
            const char* last_slash = NULL;
            const char* p = full_path;
            while (*p) {
                if (*p == '/') last_slash = p;
                p++;
            }
            
            const char* filename = last_slash ? last_slash + 1 : full_path;
            int j = 0;
            while (filename[j] && j < MAX_FILENAME_LENGTH - 1) {
                entry->name[j] = filename[j];
                j++;
            }
            entry->name[j] = '\0';
            
            return 0;
        }
    }
    
    return -1;
}

/**
 * Create directory with permissions
 */
static int ramdisk_mkdir(const char* path, uint16_t mode) {
    if (!path) {
        return -1;
    }
    
    /* Create directory using create_file */
    int result = ramdisk_create_file(path, FILE_TYPE_DIRECTORY);
    if (result != 0) {
        return result;
    }
    
    /* Update mode if different from default */
    if (mode != DEFAULT_DIR_MODE) {
        return ramdisk_chmod(path, mode);
    }
    
    return 0;
}

/**
 * Remove directory
 */
static int ramdisk_rmdir(const char* path) {
    if (!path) {
        return -1;
    }
    
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }
    
    /* Can't remove root directory */
    if (path[0] == '\0') {
        return -1;
    }
    
    ramdisk_inode_t* inode = find_inode_by_path(path);
    if (!inode) {
        return -1; /* Directory not found */
    }
    
    /* Verify it's a directory */
    if (inode->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Check if directory is empty */
    if (inode->child_count > 0) {
        return -1; /* Directory not empty */
    }
    
    /* Remove from parent's children list */
    ramdisk_inode_t* parent = &inodes[inode->parent_ino];
    if (parent->used) {
        for (uint32_t i = 0; i < parent->child_count; i++) {
            if (parent->children[i] == inode->ino) {
                /* Shift remaining children */
                for (uint32_t j = i; j < parent->child_count - 1; j++) {
                    parent->children[j] = parent->children[j + 1];
                }
                parent->child_count--;
                break;
            }
        }
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
 * Change file permissions
 */
static int ramdisk_chmod(const char* path, uint16_t mode) {
    if (!path) {
        return -1;
    }
    
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }
    
    ramdisk_inode_t* inode = find_inode_by_path(path);
    if (!inode) {
        return -1;
    }
    
    inode->mode = mode;
    return 0;
}

/**
 * Change file ownership
 */
static int ramdisk_chown(const char* path, uint16_t uid, uint16_t gid) {
    if (!path) {
        return -1;
    }
    
    /* Skip leading slash */
    if (path[0] == '/') {
        path++;
    }
    
    ramdisk_inode_t* inode = find_inode_by_path(path);
    if (!inode) {
        return -1;
    }
    
    inode->uid = uid;
    inode->gid = gid;
    return 0;
}

/**
 * Rename file or directory
 */
static int ramdisk_rename(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath) {
        return -1;
    }
    
    /* Skip leading slashes */
    if (oldpath[0] == '/') {
        oldpath++;
    }
    if (newpath[0] == '/') {
        newpath++;
    }
    
    /* Check that old path exists */
    ramdisk_inode_t* inode = find_inode_by_path(oldpath);
    if (!inode) {
        return -1;
    }
    
    /* Check that new path doesn't exist */
    if (find_inode_by_path(newpath)) {
        return -1;
    }
    
    /* Update file table entry with new name */
    for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
        if (file_table[i].used && str_equal(file_table[i].name, oldpath)) {
            str_copy(file_table[i].name, newpath, MAX_FILENAME);
            
            /* Update parent if necessary */
            char child_name[MAX_FILENAME];
            ramdisk_inode_t* new_parent = find_parent_directory(newpath, child_name, MAX_FILENAME);
            if (new_parent && new_parent->ino != inode->parent_ino) {
                /* Remove from old parent */
                ramdisk_inode_t* old_parent = &inodes[inode->parent_ino];
                for (uint32_t j = 0; j < old_parent->child_count; j++) {
                    if (old_parent->children[j] == inode->ino) {
                        for (uint32_t k = j; k < old_parent->child_count - 1; k++) {
                            old_parent->children[k] = old_parent->children[k + 1];
                        }
                        old_parent->child_count--;
                        break;
                    }
                }
                
                /* Add to new parent */
                if (new_parent->child_count < 64) {
                    new_parent->children[new_parent->child_count++] = inode->ino;
                }
                
                inode->parent_ino = new_parent->ino;
                file_table[i].parent_ino = new_parent->ino;
            }
            
            return 0;
        }
    }
    
    return -1;
}
