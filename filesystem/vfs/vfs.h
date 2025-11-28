/**
 * Aurora OS - Virtual File System Header
 * 
 * VFS layer for file system abstraction
 */

#ifndef AURORA_VFS_H
#define AURORA_VFS_H

#include <stdint.h>
#include <stddef.h>

/* Maximum number of open files */
#define MAX_OPEN_FILES 256
#define MAX_PATH_LENGTH 256
#define MAX_FILENAME_LENGTH 64

/* File open flags */
#define O_RDONLY    0x0001
#define O_WRONLY    0x0002
#define O_RDWR      0x0003
#define O_CREAT     0x0100
#define O_APPEND    0x0200
#define O_TRUNC     0x0400
#define O_EXCL      0x0800

/* Seek whence values */
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

/* Access permission check modes for vfs_access */
#define F_OK        0  /* Check existence */
#define R_OK        4  /* Check read permission */
#define W_OK        2  /* Check write permission */
#define X_OK        1  /* Check execute permission */

/* File permission bits (Unix-style) */
#define S_IRUSR     0x0100  /* Owner read */
#define S_IWUSR     0x0080  /* Owner write */
#define S_IXUSR     0x0040  /* Owner execute */
#define S_IRGRP     0x0020  /* Group read */
#define S_IWGRP     0x0010  /* Group write */
#define S_IXGRP     0x0008  /* Group execute */
#define S_IROTH     0x0004  /* Others read */
#define S_IWOTH     0x0002  /* Others write */
#define S_IXOTH     0x0001  /* Others execute */

/* Default permissions */
#define S_IRWXU     (S_IRUSR | S_IWUSR | S_IXUSR)
#define S_IRWXG     (S_IRGRP | S_IWGRP | S_IXGRP)
#define S_IRWXO     (S_IROTH | S_IWOTH | S_IXOTH)
#define DEFAULT_FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DEFAULT_DIR_MODE    (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

/* File types */
typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_DEVICE,
    FILE_TYPE_SYMLINK
} file_type_t;

/* File system type */
typedef struct fs_type {
    const char* name;
    struct fs_ops* ops;
    struct fs_type* next;
} fs_type_t;

/* inode structure with permissions */
typedef struct inode {
    uint32_t ino;
    file_type_t type;
    uint32_t size;
    uint32_t links;
    uint32_t blocks;
    uint16_t mode;         /* Permission bits */
    uint16_t uid;          /* Owner user ID */
    uint16_t gid;          /* Owner group ID */
    uint32_t atime;        /* Last access time */
    uint32_t mtime;        /* Last modification time */
    uint32_t ctime;        /* Creation time */
    uint32_t parent_ino;   /* Parent directory inode */
    void* fs_data;
} inode_t;

/* File descriptor entry */
typedef struct file_descriptor {
    int fd;
    inode_t* inode;
    uint32_t offset;
    int flags;
    int ref_count;
} file_descriptor_t;

/* File operations */
typedef struct file_ops {
    int (*open)(inode_t* inode, int flags);
    int (*close)(inode_t* inode);
    int (*read)(inode_t* inode, void* buffer, size_t size, uint32_t offset);
    int (*write)(inode_t* inode, const void* buffer, size_t size, uint32_t offset);
} file_ops_t;

/* Directory entry structure (forward declaration) */
typedef struct dirent dirent_t;

/* File system operations */
typedef struct fs_ops {
    int (*mount)(const char* device);
    int (*unmount)(void);
    inode_t* (*lookup)(const char* path);
    int (*create)(const char* path, file_type_t type);
    int (*unlink)(const char* path);
    int (*readdir)(inode_t* dir, dirent_t* entry, uint32_t index);
    int (*mkdir)(const char* path, uint16_t mode);
    int (*rmdir)(const char* path);
    int (*chmod)(const char* path, uint16_t mode);
    int (*chown)(const char* path, uint16_t uid, uint16_t gid);
    int (*rename)(const char* oldpath, const char* newpath);
} fs_ops_t;

/* Directory entry structure */
struct dirent {
    uint32_t ino;
    char name[MAX_FILENAME_LENGTH];
    file_type_t type;
};

/* VFS functions */
void vfs_init(void);
int vfs_register_fs(const char* name, fs_ops_t* ops);
int vfs_mount(const char* device, const char* mountpoint, const char* fstype);
int vfs_unmount(const char* mountpoint);

/* File operations */
int vfs_open(const char* path, int flags);
int vfs_close(int fd);
int vfs_read(int fd, void* buffer, size_t size);
int vfs_write(int fd, const void* buffer, size_t size);
int vfs_seek(int fd, long offset, int whence);

/* Directory operations */
int vfs_mkdir(const char* path);
int vfs_mkdir_mode(const char* path, uint16_t mode);
int vfs_rmdir(const char* path);
int vfs_readdir(int fd, dirent_t* entry);
int vfs_opendir(const char* path);
int vfs_closedir(int dirfd);

/* File/directory operations */
int vfs_create(const char* path);
int vfs_create_mode(const char* path, uint16_t mode);
int vfs_unlink(const char* path);
int vfs_stat(const char* path, inode_t* stat);
int vfs_rename(const char* oldpath, const char* newpath);

/* Permission operations */
int vfs_chmod(const char* path, uint16_t mode);
int vfs_chown(const char* path, uint16_t uid, uint16_t gid);
int vfs_access(const char* path, int mode);

/* Path manipulation utilities */
int vfs_getcwd(char* buffer, size_t size);
int vfs_chdir(const char* path);
int vfs_basename(const char* path, char* buffer, size_t size);
int vfs_dirname(const char* path, char* buffer, size_t size);
int vfs_realpath(const char* path, char* resolved_path, size_t size);

/* Cache management functions */
int vfs_cache_file(const char* path);
int vfs_uncache_file(const char* path);
int vfs_is_cached(const char* path);

#endif /* AURORA_VFS_H */
