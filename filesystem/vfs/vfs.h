/**
 * Aurora OS - Virtual File System Header
 * 
 * VFS layer for file system abstraction
 */

#ifndef AURORA_VFS_H
#define AURORA_VFS_H

#include <stdint.h>
#include <stddef.h>

/* File types */
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_DEVICE,
    FILE_TYPE_SYMLINK
} file_type_t;

/* File operations */
typedef struct file_ops {
    int (*open)(const char* path, int flags);
    int (*close)(int fd);
    int (*read)(int fd, void* buffer, size_t size);
    int (*write)(int fd, const void* buffer, size_t size);
    int (*seek)(int fd, long offset, int whence);
} file_ops_t;

/* VFS functions */
void vfs_init(void);
int vfs_mount(const char* device, const char* mountpoint, const char* fstype);
int vfs_open(const char* path, int flags);
int vfs_close(int fd);
int vfs_read(int fd, void* buffer, size_t size);
int vfs_write(int fd, const void* buffer, size_t size);

#endif /* AURORA_VFS_H */
