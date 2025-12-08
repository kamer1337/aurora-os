/**
 * Aurora OS - AuroraFS Advanced File System
 * 
 * Custom file system with deduplication, compression, snapshots, and encryption
 */

#ifndef AURORA_AURORAFS_H
#define AURORA_AURORAFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../vfs/vfs.h"

/* AuroraFS constants */
#define AURORAFS_MAGIC          0x41555246  /* "AURF" */
#define AURORAFS_VERSION        1
#define AURORAFS_BLOCK_SIZE     4096
#define AURORAFS_MAX_SNAPSHOTS  256
#define AURORAFS_HASH_SIZE      32  /* SHA-256 */
#define AURORAFS_MAX_NAME       255

/* Feature flags */
#define AURORAFS_FEAT_DEDUP         0x0001
#define AURORAFS_FEAT_COMPRESS      0x0002
#define AURORAFS_FEAT_SNAPSHOT      0x0004
#define AURORAFS_FEAT_ENCRYPT       0x0008
#define AURORAFS_FEAT_JOURNAL       0x0010
#define AURORAFS_FEAT_QUOTA         0x0020

/* Compression algorithms */
#define AURORAFS_COMPRESS_NONE      0
#define AURORAFS_COMPRESS_LZ4       1
#define AURORAFS_COMPRESS_ZSTD      2
#define AURORAFS_COMPRESS_LZMA      3

/* Encryption algorithms */
#define AURORAFS_ENCRYPT_NONE       0
#define AURORAFS_ENCRYPT_AES256     1
#define AURORAFS_ENCRYPT_CHACHA20   2

/* AuroraFS superblock */
typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint64_t total_blocks;
    uint64_t free_blocks;
    uint64_t total_inodes;
    uint64_t free_inodes;
    uint32_t block_size;
    uint32_t features;
    uint64_t root_inode;
    uint64_t journal_inode;
    uint64_t dedup_table_inode;
    uint64_t snapshot_table_inode;
    uint32_t default_compress;
    uint32_t default_encrypt;
    uint8_t  uuid[16];
    uint8_t  volume_label[64];
    uint64_t creation_time;
    uint64_t last_mount_time;
    uint32_t mount_count;
    uint32_t max_mount_count;
    uint16_t state;
    uint16_t errors;
    uint8_t  reserved[3840];
} aurorafs_superblock_t;

/* AuroraFS inode */
typedef struct __attribute__((packed)) {
    uint64_t ino;
    uint16_t mode;
    uint16_t uid;
    uint16_t gid;
    uint16_t flags;
    uint64_t size;
    uint64_t blocks;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
    uint32_t links;
    uint32_t compress_type;
    uint32_t encrypt_type;
    uint8_t  encryption_key[32];  /* Encrypted with master key */
    uint64_t extent_tree;  /* Root of extent tree */
    uint64_t parent_ino;
    uint8_t  hash[AURORAFS_HASH_SIZE];  /* For deduplication */
    uint8_t  reserved[128];
} aurorafs_inode_t;

/* Extent node for data storage */
typedef struct __attribute__((packed)) {
    uint64_t logical_offset;
    uint64_t physical_block;
    uint64_t length;
    uint16_t flags;
    uint16_t refcount;  /* For deduplication */
    uint8_t  hash[AURORAFS_HASH_SIZE];
    uint32_t compressed_size;
    uint32_t reserved;
} aurorafs_extent_t;

/* Snapshot header */
typedef struct __attribute__((packed)) {
    uint64_t snapshot_id;
    uint64_t parent_snapshot;
    uint64_t root_inode;
    uint64_t creation_time;
    uint64_t space_used;
    uint32_t flags;
    uint8_t  name[AURORAFS_MAX_NAME];
    uint8_t  description[256];
} aurorafs_snapshot_t;

/* Deduplication hash table entry */
typedef struct __attribute__((packed)) {
    uint8_t  hash[AURORAFS_HASH_SIZE];
    uint64_t physical_block;
    uint32_t refcount;
    uint32_t compressed_size;
    uint64_t next_entry;  /* Collision chain */
} aurorafs_dedup_entry_t;

/* Directory entry */
typedef struct __attribute__((packed)) {
    uint64_t inode;
    uint16_t rec_len;
    uint8_t  name_len;
    uint8_t  file_type;
    char     name[1];  /* Variable length */
} aurorafs_dirent_t;

/* Mount information */
typedef struct {
    aurorafs_superblock_t* superblock;
    uint64_t current_snapshot;
    uint8_t  master_key[32];
    bool     encryption_enabled;
    bool     dedup_enabled;
    bool     compress_enabled;
    void*    device;
    void*    dedup_hash_table;
} aurorafs_mount_t;

/* AuroraFS initialization */
void aurorafs_init(void);
int aurorafs_format(const char* device, uint64_t size, uint32_t features);
int aurorafs_mount_device(const char* device, const uint8_t* master_key);
int aurorafs_unmount_device(void);

/* Get AuroraFS file system operations */
fs_ops_t* aurorafs_get_ops(void);

/* Deduplication functions */
int aurorafs_dedup_find_block(aurorafs_mount_t* mount, const uint8_t* hash, 
                              uint64_t* physical_block);
int aurorafs_dedup_add_block(aurorafs_mount_t* mount, const uint8_t* hash, 
                             uint64_t physical_block);
int aurorafs_dedup_inc_refcount(aurorafs_mount_t* mount, uint64_t physical_block);
int aurorafs_dedup_dec_refcount(aurorafs_mount_t* mount, uint64_t physical_block);

/* Compression functions */
int aurorafs_compress_block(const void* input, size_t input_size, 
                            void* output, size_t* output_size, uint32_t algorithm);
int aurorafs_decompress_block(const void* input, size_t input_size,
                              void* output, size_t output_size, uint32_t algorithm);

/* Snapshot functions */
int aurorafs_create_snapshot(aurorafs_mount_t* mount, const char* name, 
                             const char* description);
int aurorafs_delete_snapshot(aurorafs_mount_t* mount, uint64_t snapshot_id);
int aurorafs_list_snapshots(aurorafs_mount_t* mount, aurorafs_snapshot_t* snapshots, 
                            uint32_t* count);
int aurorafs_rollback_snapshot(aurorafs_mount_t* mount, uint64_t snapshot_id);

/* Encryption functions */
int aurorafs_encrypt_block(aurorafs_mount_t* mount, const void* input, 
                           void* output, size_t size, const uint8_t* key);
int aurorafs_decrypt_block(aurorafs_mount_t* mount, const void* input, 
                           void* output, size_t size, const uint8_t* key);
int aurorafs_derive_key(const uint8_t* master_key, const uint8_t* salt, 
                        uint8_t* derived_key);

/* Extent management */
int aurorafs_allocate_extent(aurorafs_mount_t* mount, uint64_t size, 
                             aurorafs_extent_t* extent);
int aurorafs_free_extent(aurorafs_mount_t* mount, const aurorafs_extent_t* extent);
int aurorafs_read_extent(aurorafs_mount_t* mount, const aurorafs_extent_t* extent,
                         void* buffer, size_t offset, size_t size);
int aurorafs_write_extent(aurorafs_mount_t* mount, aurorafs_extent_t* extent,
                          const void* buffer, size_t offset, size_t size);

#endif /* AURORA_AURORAFS_H */
