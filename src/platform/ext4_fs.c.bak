/**
 * @file ext4_fs.c
 * @brief ext4 Filesystem Implementation for Aurora OS
 *
 * Implements ext4 filesystem driver for Android system/data partitions
 */

#include <stdint.h>
#include <stdbool.h>
#include "../../include/platform/platform_util.h"

/* ============================================================================
 * EXT4 FILESYSTEM STRUCTURES
 * ============================================================================ */

/* Ext4 superblock magic */
#define EXT4_SUPER_MAGIC        0xEF53

/* Block sizes */
#define EXT4_MIN_BLOCK_SIZE     1024
#define EXT4_MAX_BLOCK_SIZE     65536

/* Inode constants */
#define EXT4_ROOT_INO           2
#define EXT4_BAD_INO            1
#define EXT4_USR_QUOTA_INO      3
#define EXT4_GRP_QUOTA_INO      4
#define EXT4_BOOT_LOADER_INO    5
#define EXT4_UNDEL_DIR_INO      6
#define EXT4_RESIZE_INO         7
#define EXT4_JOURNAL_INO        8

/* File types in directory entries */
#define EXT4_FT_UNKNOWN         0
#define EXT4_FT_REG_FILE        1
#define EXT4_FT_DIR             2
#define EXT4_FT_CHRDEV          3
#define EXT4_FT_BLKDEV          4
#define EXT4_FT_FIFO            5
#define EXT4_FT_SOCK            6
#define EXT4_FT_SYMLINK         7

/* File mode flags */
#define EXT4_S_IFSOCK           0xC000  /* Socket */
#define EXT4_S_IFLNK            0xA000  /* Symbolic link */
#define EXT4_S_IFREG            0x8000  /* Regular file */
#define EXT4_S_IFBLK            0x6000  /* Block device */
#define EXT4_S_IFDIR            0x4000  /* Directory */
#define EXT4_S_IFCHR            0x2000  /* Character device */
#define EXT4_S_IFIFO            0x1000  /* FIFO */

/* Feature flags */
#define EXT4_FEATURE_COMPAT_HAS_JOURNAL     0x0004
#define EXT4_FEATURE_INCOMPAT_EXTENTS       0x0040
#define EXT4_FEATURE_INCOMPAT_64BIT         0x0080
#define EXT4_FEATURE_INCOMPAT_FLEX_BG       0x0200

/* Ext4 superblock structure */
typedef struct {
    uint32_t s_inodes_count;        /* Total inode count */
    uint32_t s_blocks_count_lo;     /* Total block count (low 32 bits) */
    uint32_t s_r_blocks_count_lo;   /* Reserved block count (low 32 bits) */
    uint32_t s_free_blocks_count_lo; /* Free block count (low 32 bits) */
    uint32_t s_free_inodes_count;   /* Free inode count */
    uint32_t s_first_data_block;    /* First data block */
    uint32_t s_log_block_size;      /* Block size (log2(block_size) - 10) */
    uint32_t s_log_cluster_size;    /* Cluster size */
    uint32_t s_blocks_per_group;    /* Blocks per group */
    uint32_t s_clusters_per_group;  /* Clusters per group */
    uint32_t s_inodes_per_group;    /* Inodes per group */
    uint32_t s_mtime;               /* Mount time */
    uint32_t s_wtime;               /* Write time */
    uint16_t s_mnt_count;           /* Mount count */
    int16_t s_max_mnt_count;        /* Max mount count */
    uint16_t s_magic;               /* Magic signature (0xEF53) */
    uint16_t s_state;               /* Filesystem state */
    uint16_t s_errors;              /* Error handling behavior */
    uint16_t s_minor_rev_level;     /* Minor revision level */
    uint32_t s_lastcheck;           /* Last check time */
    uint32_t s_checkinterval;       /* Check interval */
    uint32_t s_creator_os;          /* OS that created filesystem */
    uint32_t s_rev_level;           /* Revision level */
    uint16_t s_def_resuid;          /* Default UID for reserved blocks */
    uint16_t s_def_resgid;          /* Default GID for reserved blocks */
    /* Extended superblock fields */
    uint32_t s_first_ino;           /* First non-reserved inode */
    uint16_t s_inode_size;          /* Inode size */
    uint16_t s_block_group_nr;      /* Block group number of this superblock */
    uint32_t s_feature_compat;      /* Compatible feature set */
    uint32_t s_feature_incompat;    /* Incompatible feature set */
    uint32_t s_feature_ro_compat;   /* Read-only compatible feature set */
    uint8_t s_uuid[16];             /* Volume UUID */
    char s_volume_name[16];         /* Volume name */
    char s_last_mounted[64];        /* Last mount path */
    uint32_t s_algorithm_usage_bitmap;
    /* Performance hints */
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t s_reserved_gdt_blocks;
    /* Journaling support */
    uint8_t s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    /* Directory indexing */
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t s_jnl_backup_type;
    uint16_t s_desc_size;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;
    uint32_t s_mkfs_time;
    uint32_t s_jnl_blocks[17];
    /* 64-bit support */
    uint32_t s_blocks_count_hi;
    uint32_t s_r_blocks_count_hi;
    uint32_t s_free_blocks_count_hi;
    uint16_t s_min_extra_isize;
    uint16_t s_want_extra_isize;
    uint32_t s_flags;
    uint16_t s_raid_stride;
    uint16_t s_mmp_interval;
    uint64_t s_mmp_block;
    uint32_t s_raid_stripe_width;
    uint8_t s_log_groups_per_flex;
    uint8_t s_checksum_type;
    uint16_t s_reserved_pad;
    uint64_t s_kbytes_written;
    uint32_t s_snapshot_inum;
    uint32_t s_snapshot_id;
    uint64_t s_snapshot_r_blocks_count;
    uint32_t s_snapshot_list;
    uint32_t s_error_count;
    uint32_t s_first_error_time;
    uint32_t s_first_error_ino;
    uint64_t s_first_error_block;
    uint8_t s_first_error_func[32];
    uint32_t s_first_error_line;
    uint32_t s_last_error_time;
    uint32_t s_last_error_ino;
    uint32_t s_last_error_line;
    uint64_t s_last_error_block;
    uint8_t s_last_error_func[32];
    uint8_t s_mount_opts[64];
    uint32_t s_usr_quota_inum;
    uint32_t s_grp_quota_inum;
    uint32_t s_overhead_blocks;
    uint32_t s_backup_bgs[2];
    uint8_t s_encrypt_algos[4];
    uint8_t s_encrypt_pw_salt[16];
    uint32_t s_lpf_ino;
    uint32_t s_prj_quota_inum;
    uint32_t s_checksum_seed;
    uint32_t s_reserved[98];
    uint32_t s_checksum;
} ext4_super_block_t;

/* Block group descriptor (64-bit) */
typedef struct {
    uint32_t bg_block_bitmap_lo;    /* Block bitmap block (low 32 bits) */
    uint32_t bg_inode_bitmap_lo;    /* Inode bitmap block (low 32 bits) */
    uint32_t bg_inode_table_lo;     /* Inode table block (low 32 bits) */
    uint16_t bg_free_blocks_count_lo;
    uint16_t bg_free_inodes_count_lo;
    uint16_t bg_used_dirs_count_lo;
    uint16_t bg_flags;
    uint32_t bg_exclude_bitmap_lo;
    uint16_t bg_block_bitmap_csum_lo;
    uint16_t bg_inode_bitmap_csum_lo;
    uint16_t bg_itable_unused_lo;
    uint16_t bg_checksum;
    /* 64-bit fields */
    uint32_t bg_block_bitmap_hi;
    uint32_t bg_inode_bitmap_hi;
    uint32_t bg_inode_table_hi;
    uint16_t bg_free_blocks_count_hi;
    uint16_t bg_free_inodes_count_hi;
    uint16_t bg_used_dirs_count_hi;
    uint16_t bg_itable_unused_hi;
    uint32_t bg_exclude_bitmap_hi;
    uint16_t bg_block_bitmap_csum_hi;
    uint16_t bg_inode_bitmap_csum_hi;
    uint32_t bg_reserved;
} ext4_group_desc_t;

/* Inode structure */
typedef struct {
    uint16_t i_mode;                /* File mode */
    uint16_t i_uid;                 /* Owner UID (low 16 bits) */
    uint32_t i_size_lo;             /* Size in bytes (low 32 bits) */
    uint32_t i_atime;               /* Access time */
    uint32_t i_ctime;               /* Change time */
    uint32_t i_mtime;               /* Modification time */
    uint32_t i_dtime;               /* Deletion time */
    uint16_t i_gid;                 /* Group ID (low 16 bits) */
    uint16_t i_links_count;         /* Hard link count */
    uint32_t i_blocks_lo;           /* Block count (low 32 bits) */
    uint32_t i_flags;               /* File flags */
    uint32_t i_osd1;                /* OS-dependent value 1 */
    uint32_t i_block[15];           /* Block pointers / extent tree */
    uint32_t i_generation;          /* File version */
    uint32_t i_file_acl_lo;         /* Extended attribute block (low 32 bits) */
    uint32_t i_size_high;           /* Size in bytes (high 32 bits) */
    uint32_t i_obso_faddr;          /* Obsolete fragment address */
    uint8_t i_osd2[12];             /* OS-dependent value 2 */
    uint16_t i_extra_isize;         /* Extra inode size */
    uint16_t i_checksum_hi;         /* Checksum (high 16 bits) */
    uint32_t i_ctime_extra;         /* Extra change time (nanoseconds) */
    uint32_t i_mtime_extra;         /* Extra modification time */
    uint32_t i_atime_extra;         /* Extra access time */
    uint32_t i_crtime;              /* Creation time */
    uint32_t i_crtime_extra;        /* Extra creation time */
    uint32_t i_version_hi;          /* Version (high 32 bits) */
    uint32_t i_projid;              /* Project ID */
} ext4_inode_t;

/* Directory entry */
typedef struct {
    uint32_t inode;                 /* Inode number */
    uint16_t rec_len;               /* Directory entry length */
    uint8_t name_len;               /* Name length */
    uint8_t file_type;              /* File type */
    char name[255];                 /* File name */
} ext4_dir_entry_t;

/* Extent header */
typedef struct {
    uint16_t eh_magic;              /* Magic number (0xF30A) */
    uint16_t eh_entries;            /* Number of valid entries */
    uint16_t eh_max;                /* Capacity of entries */
    uint16_t eh_depth;              /* Tree depth (0 for leaf nodes) */
    uint32_t eh_generation;         /* Generation number */
} ext4_extent_header_t;

/* Extent leaf node */
typedef struct {
    uint32_t ee_block;              /* First logical block */
    uint16_t ee_len;                /* Number of blocks */
    uint16_t ee_start_hi;           /* Physical block (high 16 bits) */
    uint32_t ee_start_lo;           /* Physical block (low 32 bits) */
} ext4_extent_t;

/* Extent index node */
typedef struct {
    uint32_t ei_block;              /* Logical block covered by index */
    uint32_t ei_leaf_lo;            /* Physical block of next level (low 32 bits) */
    uint16_t ei_leaf_hi;            /* Physical block (high 16 bits) */
    uint16_t ei_unused;
} ext4_extent_idx_t;

/* ============================================================================
 * EXT4 FILESYSTEM STATE
 * ============================================================================ */

#define EXT4_MAX_MOUNTS 4
#define EXT4_MAX_OPEN_FILES 64
#define EXT4_BLOCK_CACHE_SIZE 32

typedef struct {
    bool mounted;
    uint8_t* device_data;           /* Device/image data */
    uint32_t device_size;           /* Device size */
    ext4_super_block_t superblock;  /* Cached superblock */
    uint32_t block_size;            /* Block size */
    uint32_t blocks_per_group;      /* Blocks per group */
    uint32_t inodes_per_group;      /* Inodes per group */
    uint32_t inode_size;            /* Inode size */
    uint32_t group_count;           /* Number of block groups */
    uint32_t desc_size;             /* Group descriptor size */
    bool is_64bit;                  /* 64-bit mode */
    char mount_point[64];           /* Mount point path */
} ext4_mount_t;

typedef struct {
    bool open;
    uint32_t mount_id;
    uint32_t inode;
    uint64_t position;
    uint64_t size;
    uint16_t mode;
    uint32_t flags;
} ext4_file_t;

static ext4_mount_t g_ext4_mounts[EXT4_MAX_MOUNTS];
static ext4_file_t g_ext4_files[EXT4_MAX_OPEN_FILES];
static bool g_ext4_initialized = false;

/* ============================================================================
 * EXT4 HELPER FUNCTIONS
 * ============================================================================ */

/**
 * Calculate block size from superblock
 */
static uint32_t ext4_block_size(ext4_super_block_t* sb) {
    return 1024 << sb->s_log_block_size;
}

/**
 * Get 64-bit block count
 */
static uint64_t ext4_blocks_count(ext4_super_block_t* sb) {
    return ((uint64_t)sb->s_blocks_count_hi << 32) | sb->s_blocks_count_lo;
}

/**
 * Read block from device
 */
static int ext4_read_block(ext4_mount_t* mount, uint64_t block, void* buffer) {
    if (!mount || !mount->mounted || !buffer) {
        return -1;
    }
    
    uint64_t offset = block * mount->block_size;
    if (offset + mount->block_size > mount->device_size) {
        return -1;
    }
    
    platform_memcpy(buffer, mount->device_data + offset, mount->block_size);
    return 0;
}

/**
 * Get block group descriptor
 */
static int ext4_get_group_desc(ext4_mount_t* mount, uint32_t group, ext4_group_desc_t* desc) {
    if (!mount || !desc) {
        return -1;
    }
    
    /* Group descriptors start at block 1 (or 0 for 1KB blocks) */
    uint32_t desc_block = (mount->block_size == 1024) ? 2 : 1;
    uint32_t offset = group * mount->desc_size;
    desc_block += offset / mount->block_size;
    uint32_t block_offset = offset % mount->block_size;
    
    uint8_t block_buf[4096];
    if (ext4_read_block(mount, desc_block, block_buf) != 0) {
        return -1;
    }
    
    platform_memcpy(desc, block_buf + block_offset, 
                   (mount->desc_size < sizeof(ext4_group_desc_t)) ? 
                   mount->desc_size : sizeof(ext4_group_desc_t));
    return 0;
}

/**
 * Read inode from filesystem
 */
static int ext4_read_inode(ext4_mount_t* mount, uint32_t inode_num, ext4_inode_t* inode) {
    if (!mount || !inode || inode_num == 0) {
        return -1;
    }
    
    /* Calculate which block group the inode is in */
    uint32_t group = (inode_num - 1) / mount->inodes_per_group;
    uint32_t local_inode = (inode_num - 1) % mount->inodes_per_group;
    
    /* Get block group descriptor */
    ext4_group_desc_t desc;
    if (ext4_get_group_desc(mount, group, &desc) != 0) {
        return -1;
    }
    
    /* Calculate inode table block */
    uint64_t inode_table = desc.bg_inode_table_lo;
    if (mount->is_64bit) {
        inode_table |= ((uint64_t)desc.bg_inode_table_hi << 32);
    }
    
    /* Calculate which block in inode table */
    uint32_t inodes_per_block = mount->block_size / mount->inode_size;
    uint32_t block = local_inode / inodes_per_block;
    uint32_t offset = (local_inode % inodes_per_block) * mount->inode_size;
    
    /* Read the block containing the inode */
    uint8_t block_buf[4096];
    if (ext4_read_block(mount, inode_table + block, block_buf) != 0) {
        return -1;
    }
    
    platform_memcpy(inode, block_buf + offset, 
                   (mount->inode_size < sizeof(ext4_inode_t)) ?
                   mount->inode_size : sizeof(ext4_inode_t));
    return 0;
}

/* ============================================================================
 * EXT4 PUBLIC API
 * ============================================================================ */

/**
 * Initialize ext4 filesystem subsystem
 */
int ext4_init(void) {
    if (g_ext4_initialized) {
        return 0;
    }
    
    platform_memset(g_ext4_mounts, 0, sizeof(g_ext4_mounts));
    platform_memset(g_ext4_files, 0, sizeof(g_ext4_files));
    
    g_ext4_initialized = true;
    return 0;
}

/**
 * Mount ext4 filesystem
 */
int ext4_mount(uint8_t* device_data, uint32_t device_size, const char* mount_point) {
    if (!g_ext4_initialized) {
        ext4_init();
    }
    
    if (!device_data || device_size < 2048 || !mount_point) {
        return -1;
    }
    
    /* Find free mount slot */
    int mount_id = -1;
    for (int i = 0; i < EXT4_MAX_MOUNTS; i++) {
        if (!g_ext4_mounts[i].mounted) {
            mount_id = i;
            break;
        }
    }
    
    if (mount_id < 0) {
        return -1; /* No free mount points */
    }
    
    ext4_mount_t* mount = &g_ext4_mounts[mount_id];
    
    /* Read superblock (always at offset 1024) */
    if (device_size < 1024 + sizeof(ext4_super_block_t)) {
        return -1;
    }
    
    platform_memcpy(&mount->superblock, device_data + 1024, sizeof(ext4_super_block_t));
    
    /* Verify magic number */
    if (mount->superblock.s_magic != EXT4_SUPER_MAGIC) {
        return -1; /* Not an ext4 filesystem */
    }
    
    /* Initialize mount structure */
    mount->device_data = device_data;
    mount->device_size = device_size;
    mount->block_size = ext4_block_size(&mount->superblock);
    mount->blocks_per_group = mount->superblock.s_blocks_per_group;
    mount->inodes_per_group = mount->superblock.s_inodes_per_group;
    mount->inode_size = mount->superblock.s_inode_size;
    
    /* Check for 64-bit mode */
    mount->is_64bit = (mount->superblock.s_feature_incompat & EXT4_FEATURE_INCOMPAT_64BIT) != 0;
    mount->desc_size = mount->is_64bit ? mount->superblock.s_desc_size : 32;
    if (mount->desc_size < 32) mount->desc_size = 32;
    
    /* Calculate number of block groups (avoid 64-bit division) */
    /* Use 32-bit calculation since blocks_per_group is 32-bit */
    uint32_t total_blocks_lo = mount->superblock.s_blocks_count_lo;
    uint32_t bpg = mount->blocks_per_group;
    mount->group_count = (total_blocks_lo + bpg - 1) / bpg;
    
    /* Store mount point */
    platform_strncpy(mount->mount_point, mount_point, sizeof(mount->mount_point));
    
    mount->mounted = true;
    
    return mount_id;
}

/**
 * Unmount ext4 filesystem
 */
int ext4_unmount(int mount_id) {
    if (mount_id < 0 || mount_id >= EXT4_MAX_MOUNTS) {
        return -1;
    }
    
    ext4_mount_t* mount = &g_ext4_mounts[mount_id];
    if (!mount->mounted) {
        return -1;
    }
    
    /* Close all files on this mount */
    for (int i = 0; i < EXT4_MAX_OPEN_FILES; i++) {
        if (g_ext4_files[i].open && g_ext4_files[i].mount_id == (uint32_t)mount_id) {
            g_ext4_files[i].open = false;
        }
    }
    
    mount->mounted = false;
    return 0;
}

/**
 * Open file on ext4 filesystem
 */
int ext4_open(int mount_id, const char* path, uint32_t flags) {
    if (mount_id < 0 || mount_id >= EXT4_MAX_MOUNTS || !path) {
        return -1;
    }
    
    ext4_mount_t* mount = &g_ext4_mounts[mount_id];
    if (!mount->mounted) {
        return -1;
    }
    
    /* Find free file slot */
    int fd = -1;
    for (int i = 0; i < EXT4_MAX_OPEN_FILES; i++) {
        if (!g_ext4_files[i].open) {
            fd = i;
            break;
        }
    }
    
    if (fd < 0) {
        return -1; /* No free file descriptors */
    }
    
    /* Start at root inode and traverse path */
    uint32_t current_inode = EXT4_ROOT_INO;
    
    /* For simplicity, just open root for now */
    ext4_inode_t inode;
    if (ext4_read_inode(mount, current_inode, &inode) != 0) {
        return -1;
    }
    
    /* Initialize file structure */
    g_ext4_files[fd].open = true;
    g_ext4_files[fd].mount_id = mount_id;
    g_ext4_files[fd].inode = current_inode;
    g_ext4_files[fd].position = 0;
    g_ext4_files[fd].size = inode.i_size_lo | ((uint64_t)inode.i_size_high << 32);
    g_ext4_files[fd].mode = inode.i_mode;
    g_ext4_files[fd].flags = flags;
    
    return fd;
}

/**
 * Close file
 */
int ext4_close(int fd) {
    if (fd < 0 || fd >= EXT4_MAX_OPEN_FILES) {
        return -1;
    }
    
    if (!g_ext4_files[fd].open) {
        return -1;
    }
    
    g_ext4_files[fd].open = false;
    return 0;
}

/**
 * Read from file
 */
int32_t ext4_read(int fd, void* buffer, uint32_t size) {
    if (fd < 0 || fd >= EXT4_MAX_OPEN_FILES || !buffer) {
        return -1;
    }
    
    ext4_file_t* file = &g_ext4_files[fd];
    if (!file->open) {
        return -1;
    }
    
    /* Calculate bytes to read */
    uint64_t remaining = file->size - file->position;
    if ((uint64_t)size > remaining) {
        size = (uint32_t)remaining;
    }
    
    if (size == 0) {
        return 0;
    }
    
    /* For a full implementation, we would:
     * 1. Get inode from mount
     * 2. Parse extent tree or block pointers
     * 3. Read data blocks
     * 4. Copy to buffer
     */
    
    file->position += size;
    return (int32_t)size;
}

/**
 * Write to file
 */
int32_t ext4_write(int fd, const void* buffer, uint32_t size) {
    if (fd < 0 || fd >= EXT4_MAX_OPEN_FILES || !buffer) {
        return -1;
    }
    
    ext4_file_t* file = &g_ext4_files[fd];
    if (!file->open) {
        return -1;
    }
    
    /* For a full implementation, we would:
     * 1. Allocate blocks if needed
     * 2. Update extent tree or block pointers
     * 3. Write data to blocks
     * 4. Update inode
     */
    
    file->position += size;
    if (file->position > file->size) {
        file->size = file->position;
    }
    
    return (int32_t)size;
}

/**
 * Seek in file
 */
int32_t ext4_seek(int fd, int32_t offset, int whence) {
    if (fd < 0 || fd >= EXT4_MAX_OPEN_FILES) {
        return -1;
    }
    
    ext4_file_t* file = &g_ext4_files[fd];
    if (!file->open) {
        return -1;
    }
    
    int64_t new_pos;
    switch (whence) {
        case 0: /* SEEK_SET */
            new_pos = offset;
            break;
        case 1: /* SEEK_CUR */
            new_pos = (int64_t)file->position + offset;
            break;
        case 2: /* SEEK_END */
            new_pos = (int64_t)file->size + offset;
            break;
        default:
            return -1;
    }
    
    if (new_pos < 0) {
        return -1;
    }
    
    file->position = (uint64_t)new_pos;
    return (int32_t)file->position; /* Return lower 32 bits for compatibility */
}

/**
 * Get filesystem statistics
 */
int ext4_statfs(int mount_id, uint64_t* total_blocks, uint64_t* free_blocks,
                uint32_t* block_size, uint64_t* total_inodes, uint64_t* free_inodes) {
    if (mount_id < 0 || mount_id >= EXT4_MAX_MOUNTS) {
        return -1;
    }
    
    ext4_mount_t* mount = &g_ext4_mounts[mount_id];
    if (!mount->mounted) {
        return -1;
    }
    
    if (total_blocks) *total_blocks = ext4_blocks_count(&mount->superblock);
    if (free_blocks) {
        *free_blocks = mount->superblock.s_free_blocks_count_lo |
                      ((uint64_t)mount->superblock.s_free_blocks_count_hi << 32);
    }
    if (block_size) *block_size = mount->block_size;
    if (total_inodes) *total_inodes = mount->superblock.s_inodes_count;
    if (free_inodes) *free_inodes = mount->superblock.s_free_inodes_count;
    
    return 0;
}

/**
 * Check if ext4 filesystem
 */
bool ext4_is_valid(uint8_t* data, uint32_t size) {
    if (!data || size < 2048) {
        return false;
    }
    
    ext4_super_block_t* sb = (ext4_super_block_t*)(data + 1024);
    return sb->s_magic == EXT4_SUPER_MAGIC;
}

/**
 * Get ext4 version string
 */
const char* ext4_get_version(void) {
    return "1.0.0-aurora-ext4";
}
