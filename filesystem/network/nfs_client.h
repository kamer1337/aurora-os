/**
 * Aurora OS - NFS Client Implementation
 * 
 * Network File System client for remote file access
 */

#ifndef AURORA_NFS_CLIENT_H
#define AURORA_NFS_CLIENT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../vfs/vfs.h"

/* NFS protocol versions */
#define NFS_VERSION_2       2
#define NFS_VERSION_3       3
#define NFS_VERSION_4       4

/* NFS port */
#define NFS_PORT            2049
#define NFS_MOUNT_PORT      635

/* NFS procedure numbers (NFSv3) */
#define NFSPROC3_NULL       0
#define NFSPROC3_GETATTR    1
#define NFSPROC3_SETATTR    2
#define NFSPROC3_LOOKUP     3
#define NFSPROC3_ACCESS     4
#define NFSPROC3_READLINK   5
#define NFSPROC3_READ       6
#define NFSPROC3_WRITE      7
#define NFSPROC3_CREATE     8
#define NFSPROC3_MKDIR      9
#define NFSPROC3_REMOVE     12
#define NFSPROC3_RMDIR      13
#define NFSPROC3_RENAME     14
#define NFSPROC3_READDIR    16
#define NFSPROC3_READDIRPLUS 17

/* NFS status codes */
#define NFS3_OK             0
#define NFS3ERR_PERM        1
#define NFS3ERR_NOENT       2
#define NFS3ERR_IO          5
#define NFS3ERR_NXIO        6
#define NFS3ERR_ACCES       13
#define NFS3ERR_EXIST       17
#define NFS3ERR_NODEV       19
#define NFS3ERR_NOTDIR      20
#define NFS3ERR_ISDIR       21
#define NFS3ERR_FBIG        27
#define NFS3ERR_NOSPC       28
#define NFS3ERR_ROFS        30
#define NFS3ERR_NAMETOOLONG 63
#define NFS3ERR_NOTEMPTY    66
#define NFS3ERR_STALE       70

/* File handle size */
#define NFS_FHSIZE          64

/* NFS file handle */
typedef struct {
    uint32_t length;
    uint8_t  data[NFS_FHSIZE];
} nfs_fhandle_t;

/* NFS file attributes */
typedef struct {
    uint32_t type;
    uint32_t mode;
    uint32_t nlink;
    uint32_t uid;
    uint32_t gid;
    uint64_t size;
    uint64_t used;
    uint32_t rdev[2];
    uint64_t fsid;
    uint64_t fileid;
    uint64_t atime_sec;
    uint32_t atime_nsec;
    uint64_t mtime_sec;
    uint32_t mtime_nsec;
    uint64_t ctime_sec;
    uint32_t ctime_nsec;
} nfs_fattr3_t;

/* NFS connection info */
typedef struct {
    char server[256];
    char export_path[256];
    uint32_t server_ip;
    uint16_t port;
    uint32_t version;
    nfs_fhandle_t root_fh;
    int socket;
    bool connected;
} nfs_conn_t;

/* NFS initialization */
void nfs_client_init(void);
int nfs_connect(const char* server, const char* export_path, uint32_t version);
int nfs_disconnect(void);

/* Get NFS file system operations */
fs_ops_t* nfs_get_ops(void);

/* NFS RPC functions */
int nfs_rpc_call(nfs_conn_t* conn, uint32_t procedure, 
                 const void* args, size_t args_size,
                 void* result, size_t result_size);

/* NFS operations */
int nfs_getattr(nfs_conn_t* conn, const nfs_fhandle_t* fh, nfs_fattr3_t* attr);
int nfs_lookup_file(nfs_conn_t* conn, const nfs_fhandle_t* dir_fh, 
                    const char* name, nfs_fhandle_t* file_fh);
int nfs_read_file(nfs_conn_t* conn, const nfs_fhandle_t* fh, 
                  uint64_t offset, uint32_t count, void* buffer);
int nfs_write_file(nfs_conn_t* conn, const nfs_fhandle_t* fh,
                   uint64_t offset, uint32_t count, const void* buffer);
int nfs_create_file(nfs_conn_t* conn, const nfs_fhandle_t* dir_fh,
                    const char* name, uint32_t mode, nfs_fhandle_t* new_fh);
int nfs_remove_file(nfs_conn_t* conn, const nfs_fhandle_t* dir_fh, const char* name);
int nfs_readdir_entries(nfs_conn_t* conn, const nfs_fhandle_t* dir_fh,
                        uint64_t cookie, void* entries, uint32_t count);

#endif /* AURORA_NFS_CLIENT_H */
