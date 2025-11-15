/**
 * Aurora OS - Virtual File System Implementation
 * 
 * VFS layer implementation
 */

#include "vfs.h"
#include <stddef.h>

/* File descriptor table */
static file_descriptor_t fd_table[MAX_OPEN_FILES];
static int next_fd = 0;

/* File system type registry */
static fs_type_t* fs_types = NULL;

/* Root file system */
static fs_type_t* root_fs = NULL;

/**
 * Initialize VFS subsystem
 */
void vfs_init(void) {
    /* Initialize file descriptor table */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        fd_table[i].fd = -1;
        fd_table[i].inode = NULL;
        fd_table[i].offset = 0;
        fd_table[i].flags = 0;
        fd_table[i].ref_count = 0;
    }
    next_fd = 0;
    fs_types = NULL;
    root_fs = NULL;
}

/**
 * Register a file system type
 */
int vfs_register_fs(const char* name, fs_ops_t* ops) {
    if (!name || !ops) {
        return -1;
    }
    
    /* Allocate new file system type (simplified - in real kernel would use kmalloc) */
    fs_type_t* fs_type = NULL; /* TODO: Use kmalloc when available */
    if (!fs_type) {
        return -1;
    }
    
    fs_type->name = name;
    fs_type->ops = ops;
    fs_type->next = fs_types;
    fs_types = fs_type;
    
    return 0;
}

/**
 * Find file system type by name
 */
static fs_type_t* find_fs_type(const char* name) {
    fs_type_t* current = fs_types;
    while (current) {
        /* Simple string comparison (would use strcmp in real implementation) */
        const char* a = current->name;
        const char* b = name;
        int match = 1;
        while (*a && *b) {
            if (*a != *b) {
                match = 0;
                break;
            }
            a++;
            b++;
        }
        if (match && *a == *b) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

/**
 * Mount a file system
 */
int vfs_mount(const char* device, const char* mountpoint, const char* fstype) {
    if (!device || !mountpoint || !fstype) {
        return -1;
    }
    
    /* Find file system type */
    fs_type_t* fs = find_fs_type(fstype);
    if (!fs || !fs->ops || !fs->ops->mount) {
        return -1;
    }
    
    /* Mount file system */
    int result = fs->ops->mount(device);
    if (result == 0) {
        /* If mounting at root, set as root file system */
        if (mountpoint[0] == '/' && mountpoint[1] == '\0') {
            root_fs = fs;
        }
    }
    
    return result;
}

/**
 * Unmount a file system
 */
int vfs_unmount(const char* mountpoint) {
    if (!mountpoint || !root_fs || !root_fs->ops || !root_fs->ops->unmount) {
        return -1;
    }
    
    /* Simple implementation - only supports unmounting root */
    if (mountpoint[0] == '/' && mountpoint[1] == '\0') {
        int result = root_fs->ops->unmount();
        if (result == 0) {
            root_fs = NULL;
        }
        return result;
    }
    
    return -1;
}

/**
 * Allocate a file descriptor
 */
static int alloc_fd(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (fd_table[i].fd == -1) {
            fd_table[i].fd = i;
            fd_table[i].ref_count = 1;
            return i;
        }
    }
    return -1;
}

/**
 * Get file descriptor entry
 */
static file_descriptor_t* get_fd(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || fd_table[fd].fd == -1) {
        return NULL;
    }
    return &fd_table[fd];
}

/**
 * Open a file
 */
int vfs_open(const char* path, int flags) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->lookup) {
        return -1;
    }
    
    /* Allocate file descriptor */
    int fd = alloc_fd();
    if (fd < 0) {
        return -1;
    }
    
    /* Lookup inode */
    inode_t* inode = root_fs->ops->lookup(path);
    
    /* If file doesn't exist and O_CREAT is set, create it */
    if (!inode && (flags & O_CREAT)) {
        if (root_fs->ops->create) {
            int result = root_fs->ops->create(path, FILE_TYPE_REGULAR);
            if (result == 0) {
                inode = root_fs->ops->lookup(path);
            }
        }
    }
    
    if (!inode) {
        fd_table[fd].fd = -1;
        return -1;
    }
    
    /* Initialize file descriptor */
    fd_table[fd].inode = inode;
    fd_table[fd].offset = (flags & O_APPEND) ? inode->size : 0;
    fd_table[fd].flags = flags;
    
    return fd;
}

/**
 * Close a file
 */
int vfs_close(int fd) {
    file_descriptor_t* file = get_fd(fd);
    if (!file) {
        return -1;
    }
    
    /* Decrement reference count */
    file->ref_count--;
    
    /* If no more references, free the descriptor */
    if (file->ref_count <= 0) {
        file->fd = -1;
        file->inode = NULL;
        file->offset = 0;
        file->flags = 0;
        file->ref_count = 0;
    }
    
    return 0;
}

/**
 * Read from a file
 */
int vfs_read(int fd, void* buffer, size_t size) {
    file_descriptor_t* file = get_fd(fd);
    if (!file || !buffer || !file->inode) {
        return -1;
    }
    
    /* Check if file is open for reading */
    if ((file->flags & O_WRONLY) && !(file->flags & O_RDWR)) {
        return -1;
    }
    
    /* Read from file using file system operations */
    int bytes_read = 0;
    
    /* Get file system specific operations from inode */
    if (root_fs && root_fs->ops) {
        /* Use ramdisk read operation directly */
        /* In a more complete implementation, would have file_ops in inode */
        inode_t* inode = file->inode;
        if (inode && inode->fs_data) {
            /* Call file system specific read (simplified) */
            /* Actual implementation would use function pointers in inode */
            bytes_read = (int)size; /* Simplified */
            if (bytes_read > 0) {
                bytes_read = 0; /* Will be implemented by fs-specific code */
            }
        }
    }
    
    /* Update file offset */
    file->offset += bytes_read;
    
    return bytes_read;
}

/**
 * Write to a file
 */
int vfs_write(int fd, const void* buffer, size_t size) {
    file_descriptor_t* file = get_fd(fd);
    if (!file || !buffer || !file->inode) {
        return -1;
    }
    
    /* Check if file is open for writing */
    if ((file->flags & O_RDONLY) && !(file->flags & O_RDWR)) {
        return -1;
    }
    
    /* Write to file using file system operations */
    int bytes_written = 0;
    
    /* Get file system specific operations from inode */
    if (root_fs && root_fs->ops) {
        /* Use file system specific write operation */
        inode_t* inode = file->inode;
        if (inode && inode->fs_data) {
            /* Call file system specific write (simplified) */
            /* Actual implementation would use function pointers in inode */
            bytes_written = (int)size; /* Simplified */
            if (bytes_written > 0) {
                bytes_written = 0; /* Will be implemented by fs-specific code */
            }
        }
    }
    
    /* Update file offset */
    file->offset += bytes_written;
    
    return bytes_written;
}

/**
 * Seek in a file
 */
int vfs_seek(int fd, long offset, int whence) {
    file_descriptor_t* file = get_fd(fd);
    if (!file || !file->inode) {
        return -1;
    }
    
    long new_offset;
    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset = file->offset + offset;
            break;
        case SEEK_END:
            new_offset = file->inode->size + offset;
            break;
        default:
            return -1;
    }
    
    /* Check bounds */
    if (new_offset < 0) {
        return -1;
    }
    
    file->offset = (uint32_t)new_offset;
    return (int)new_offset;
}

/**
 * Create a directory
 */
int vfs_mkdir(const char* path) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->create) {
        return -1;
    }
    
    return root_fs->ops->create(path, FILE_TYPE_DIRECTORY);
}

/**
 * Remove a directory
 */
int vfs_rmdir(const char* path) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->unlink) {
        return -1;
    }
    
    /* Verify it's a directory */
    inode_t* inode = root_fs->ops->lookup(path);
    if (!inode || inode->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    return root_fs->ops->unlink(path);
}

/**
 * Read directory entry
 */
int vfs_readdir(int fd, dirent_t* entry) {
    file_descriptor_t* file = get_fd(fd);
    if (!file || !entry || !file->inode) {
        return -1;
    }
    
    /* Verify it's a directory */
    if (file->inode->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Use file system specific readdir if available */
    if (root_fs && root_fs->ops && root_fs->ops->readdir) {
        /* Call fs-specific readdir with current offset as index */
        int result = root_fs->ops->readdir(file->inode, entry, file->offset);
        
        if (result == 0) {
            /* Successfully read an entry, increment offset for next read */
            file->offset++;
            return 0;
        }
    }
    
    return -1;
}

/**
 * Create a file
 */
int vfs_create(const char* path) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->create) {
        return -1;
    }
    
    return root_fs->ops->create(path, FILE_TYPE_REGULAR);
}

/**
 * Delete a file
 */
int vfs_unlink(const char* path) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->unlink) {
        return -1;
    }
    
    return root_fs->ops->unlink(path);
}

/**
 * Get file information
 */
int vfs_stat(const char* path, inode_t* stat) {
    if (!path || !stat || !root_fs || !root_fs->ops || !root_fs->ops->lookup) {
        return -1;
    }
    
    inode_t* inode = root_fs->ops->lookup(path);
    if (!inode) {
        return -1;
    }
    
    /* Copy inode information */
    stat->ino = inode->ino;
    stat->type = inode->type;
    stat->size = inode->size;
    stat->links = inode->links;
    stat->blocks = inode->blocks;
    stat->fs_data = inode->fs_data;
    
    return 0;
}
