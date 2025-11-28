/**
 * Aurora OS - Virtual File System Implementation
 * 
 * VFS layer implementation
 */

#include "vfs.h"
#include "../cache/file_cache.h"
#include "../../kernel/memory/memory.h"
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
    
    /* Initialize file cache */
    file_cache_init();
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
    
    /* Copy inode information including new permission fields */
    stat->ino = inode->ino;
    stat->type = inode->type;
    stat->size = inode->size;
    stat->links = inode->links;
    stat->blocks = inode->blocks;
    stat->mode = inode->mode;
    stat->uid = inode->uid;
    stat->gid = inode->gid;
    stat->atime = inode->atime;
    stat->mtime = inode->mtime;
    stat->ctime = inode->ctime;
    stat->parent_ino = inode->parent_ino;
    stat->fs_data = inode->fs_data;
    
    return 0;
}

/**
 * Cache a file in RAM
 */
int vfs_cache_file(const char* path) {
    if (!path) {
        return -1;
    }
    
    /* Check if already cached */
    if (file_cache_exists(path)) {
        return 0;
    }
    
    /* Get file information */
    inode_t stat;
    if (vfs_stat(path, &stat) != 0) {
        return -1;
    }
    
    /* Only cache regular files */
    if (stat.type != FILE_TYPE_REGULAR) {
        return -1;
    }
    
    /* Open the file */
    int fd = vfs_open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    /* Allocate temporary buffer for file data */
    uint8_t* temp_buffer = (uint8_t*)kmalloc(stat.size);
    if (!temp_buffer) {
        vfs_close(fd);
        return -1;
    }
    
    /* Read file content (simplified - actual read would use fs-specific ops) */
    /* For now, just attempt to cache with placeholder data */
    int result = file_cache_store(path, temp_buffer, stat.size);
    
    kfree(temp_buffer);
    vfs_close(fd);
    
    return result;
}

/**
 * Remove file from cache
 */
int vfs_uncache_file(const char* path) {
    if (!path) {
        return -1;
    }
    
    return file_cache_remove(path);
}

/**
 * Check if file is cached
 */
int vfs_is_cached(const char* path) {
    if (!path) {
        return 0;
    }
    
    return file_cache_exists(path);
}

/* ============================================================================
 * ADVANCED FILE SYSTEM FEATURES
 * ============================================================================ */

/* Current working directory */
static char g_cwd[MAX_PATH_LENGTH] = "/";

/**
 * Create a directory with specific permissions
 */
int vfs_mkdir_mode(const char* path, uint16_t mode) {
    if (!path || !root_fs || !root_fs->ops) {
        return -1;
    }
    
    /* Use mkdir operation if available */
    if (root_fs->ops->mkdir) {
        return root_fs->ops->mkdir(path, mode);
    }
    
    /* Fall back to create with directory type */
    if (root_fs->ops->create) {
        int result = root_fs->ops->create(path, FILE_TYPE_DIRECTORY);
        if (result == 0 && root_fs->ops->chmod) {
            /* Set permissions after creation */
            root_fs->ops->chmod(path, mode);
        }
        return result;
    }
    
    return -1;
}

/**
 * Create a file with specific permissions
 */
int vfs_create_mode(const char* path, uint16_t mode) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->create) {
        return -1;
    }
    
    int result = root_fs->ops->create(path, FILE_TYPE_REGULAR);
    if (result == 0 && root_fs->ops->chmod) {
        /* Set permissions after creation */
        root_fs->ops->chmod(path, mode);
    }
    
    return result;
}

/**
 * Open a directory for reading
 */
int vfs_opendir(const char* path) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->lookup) {
        return -1;
    }
    
    /* Lookup directory inode */
    inode_t* inode = root_fs->ops->lookup(path);
    if (!inode || inode->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Allocate file descriptor */
    int fd = alloc_fd();
    if (fd < 0) {
        return -1;
    }
    
    /* Initialize file descriptor for directory */
    fd_table[fd].inode = inode;
    fd_table[fd].offset = 0;
    fd_table[fd].flags = O_RDONLY;
    
    return fd;
}

/**
 * Close a directory
 */
int vfs_closedir(int dirfd) {
    return vfs_close(dirfd);
}

/**
 * Change file permissions
 */
int vfs_chmod(const char* path, uint16_t mode) {
    if (!path || !root_fs || !root_fs->ops) {
        return -1;
    }
    
    if (root_fs->ops->chmod) {
        return root_fs->ops->chmod(path, mode);
    }
    
    /* If chmod not supported, try to update inode directly */
    inode_t* inode = root_fs->ops->lookup ? root_fs->ops->lookup(path) : NULL;
    if (inode) {
        inode->mode = mode;
        return 0;
    }
    
    return -1;
}

/**
 * Change file ownership
 */
int vfs_chown(const char* path, uint16_t uid, uint16_t gid) {
    if (!path || !root_fs || !root_fs->ops) {
        return -1;
    }
    
    if (root_fs->ops->chown) {
        return root_fs->ops->chown(path, uid, gid);
    }
    
    /* If chown not supported, try to update inode directly */
    inode_t* inode = root_fs->ops->lookup ? root_fs->ops->lookup(path) : NULL;
    if (inode) {
        inode->uid = uid;
        inode->gid = gid;
        return 0;
    }
    
    return -1;
}

/**
 * Check access permissions
 */
int vfs_access(const char* path, int mode) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->lookup) {
        return -1;
    }
    
    inode_t* inode = root_fs->ops->lookup(path);
    if (!inode) {
        return -1; /* File doesn't exist */
    }
    
    /* Check requested permissions against inode mode */
    uint16_t required = 0;
    if (mode & O_RDONLY) required |= S_IROTH;
    if (mode & O_WRONLY) required |= S_IWOTH;
    
    /* Simple check: if any required permission is set in mode, allow */
    if ((inode->mode & required) == required) {
        return 0;
    }
    
    /* Also allow if owner permissions are set (simplified - no user check) */
    uint16_t owner_req = 0;
    if (mode & O_RDONLY) owner_req |= S_IRUSR;
    if (mode & O_WRONLY) owner_req |= S_IWUSR;
    
    if ((inode->mode & owner_req) == owner_req) {
        return 0;
    }
    
    return -1; /* Permission denied */
}

/**
 * Rename/move file or directory
 */
int vfs_rename(const char* oldpath, const char* newpath) {
    if (!oldpath || !newpath || !root_fs || !root_fs->ops) {
        return -1;
    }
    
    if (root_fs->ops->rename) {
        return root_fs->ops->rename(oldpath, newpath);
    }
    
    return -1; /* Not supported */
}

/**
 * Get current working directory
 */
int vfs_getcwd(char* buffer, size_t size) {
    if (!buffer || size == 0) {
        return -1;
    }
    
    size_t len = 0;
    while (g_cwd[len] && len < size - 1) {
        buffer[len] = g_cwd[len];
        len++;
    }
    buffer[len] = '\0';
    
    return (int)len;
}

/**
 * Change current working directory
 */
int vfs_chdir(const char* path) {
    if (!path || !root_fs || !root_fs->ops || !root_fs->ops->lookup) {
        return -1;
    }
    
    /* Verify path exists and is a directory */
    inode_t* inode = root_fs->ops->lookup(path);
    if (!inode || inode->type != FILE_TYPE_DIRECTORY) {
        return -1;
    }
    
    /* Copy path to current working directory */
    size_t len = 0;
    while (path[len] && len < MAX_PATH_LENGTH - 1) {
        g_cwd[len] = path[len];
        len++;
    }
    g_cwd[len] = '\0';
    
    return 0;
}

/**
 * Helper: string length
 */
static size_t vfs_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

/**
 * Get base name from path
 */
int vfs_basename(const char* path, char* buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        return -1;
    }
    
    size_t len = vfs_strlen(path);
    if (len == 0) {
        buffer[0] = '\0';
        return 0;
    }
    
    /* Skip trailing slashes */
    while (len > 0 && path[len - 1] == '/') {
        len--;
    }
    
    /* Find last slash */
    size_t start = len;
    while (start > 0 && path[start - 1] != '/') {
        start--;
    }
    
    /* Copy basename */
    size_t i = 0;
    while (start < len && i < size - 1) {
        buffer[i++] = path[start++];
    }
    buffer[i] = '\0';
    
    return (int)i;
}

/**
 * Get directory name from path
 */
int vfs_dirname(const char* path, char* buffer, size_t size) {
    if (!path || !buffer || size == 0) {
        return -1;
    }
    
    size_t len = vfs_strlen(path);
    if (len == 0) {
        buffer[0] = '.';
        buffer[1] = '\0';
        return 1;
    }
    
    /* Skip trailing slashes */
    while (len > 0 && path[len - 1] == '/') {
        len--;
    }
    
    /* Find last slash */
    while (len > 0 && path[len - 1] != '/') {
        len--;
    }
    
    /* Handle root directory */
    if (len == 0) {
        buffer[0] = '.';
        buffer[1] = '\0';
        return 1;
    }
    
    /* Skip trailing slashes in directory part */
    while (len > 1 && path[len - 1] == '/') {
        len--;
    }
    
    /* Copy dirname */
    if (len > size - 1) {
        len = size - 1;
    }
    
    for (size_t i = 0; i < len; i++) {
        buffer[i] = path[i];
    }
    buffer[len] = '\0';
    
    return (int)len;
}

/**
 * Resolve path to absolute path (handles . and ..)
 */
int vfs_realpath(const char* path, char* resolved_path, size_t size) {
    if (!path || !resolved_path || size == 0) {
        return -1;
    }
    
    char result[MAX_PATH_LENGTH];
    size_t result_len = 0;
    
    /* Start with root or current directory */
    if (path[0] == '/') {
        result[0] = '/';
        result_len = 1;
    } else {
        /* Start with current directory */
        size_t cwd_len = vfs_strlen(g_cwd);
        for (size_t i = 0; i < cwd_len && i < MAX_PATH_LENGTH - 1; i++) {
            result[i] = g_cwd[i];
        }
        result_len = cwd_len;
        if (result_len > 0 && result[result_len - 1] != '/') {
            result[result_len++] = '/';
        }
    }
    
    /* Process path components */
    size_t i = (path[0] == '/') ? 1 : 0;
    while (path[i]) {
        /* Skip consecutive slashes */
        while (path[i] == '/') i++;
        if (!path[i]) break;
        
        /* Find end of component */
        size_t comp_start = i;
        while (path[i] && path[i] != '/') i++;
        size_t comp_len = i - comp_start;
        
        /* Handle . (current directory) */
        if (comp_len == 1 && path[comp_start] == '.') {
            continue;
        }
        
        /* Handle .. (parent directory) */
        if (comp_len == 2 && path[comp_start] == '.' && path[comp_start + 1] == '.') {
            /* Go up one directory */
            if (result_len > 1) {
                result_len--; /* Remove trailing slash */
                while (result_len > 0 && result[result_len - 1] != '/') {
                    result_len--;
                }
            }
            continue;
        }
        
        /* Add component */
        if (result_len > 0 && result[result_len - 1] != '/') {
            result[result_len++] = '/';
        }
        
        for (size_t j = 0; j < comp_len && result_len < MAX_PATH_LENGTH - 1; j++) {
            result[result_len++] = path[comp_start + j];
        }
    }
    
    /* Ensure at least root */
    if (result_len == 0) {
        result[0] = '/';
        result_len = 1;
    }
    
    result[result_len] = '\0';
    
    /* Copy to output buffer */
    size_t copy_len = result_len < size - 1 ? result_len : size - 1;
    for (size_t j = 0; j < copy_len; j++) {
        resolved_path[j] = result[j];
    }
    resolved_path[copy_len] = '\0';
    
    return (int)copy_len;
}
