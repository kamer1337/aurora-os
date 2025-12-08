/**
 * Aurora OS - FAT32 File System Driver
 * 
 * FAT32 file system driver for compatibility with Windows and removable media
 */

#ifndef AURORA_FAT32_H
#define AURORA_FAT32_H

#include <stdint.h>
#include <stddef.h>
#include "../vfs/vfs.h"

/* FAT32 constants */
#define FAT32_SIGNATURE         0xAA55
#define FAT32_BOOT_SIGNATURE    0x29
#define FAT32_FS_TYPE           "FAT32   "
#define FAT32_SECTOR_SIZE       512
#define FAT32_MAX_PATH          260
#define FAT32_MAX_FILENAME      255

/* FAT entry values */
#define FAT32_FREE_CLUSTER      0x00000000
#define FAT32_RESERVED_MIN      0x0FFFFFF0
#define FAT32_BAD_CLUSTER       0x0FFFFFF7
#define FAT32_EOC_MIN           0x0FFFFFF8
#define FAT32_EOC_MAX           0x0FFFFFFF

/* Directory entry attributes */
#define FAT32_ATTR_READ_ONLY    0x01
#define FAT32_ATTR_HIDDEN       0x02
#define FAT32_ATTR_SYSTEM       0x04
#define FAT32_ATTR_VOLUME_ID    0x08
#define FAT32_ATTR_DIRECTORY    0x10
#define FAT32_ATTR_ARCHIVE      0x20
#define FAT32_ATTR_LONG_NAME    0x0F

/* Boot sector structure */
typedef struct __attribute__((packed)) {
    uint8_t  jump_boot[3];
    uint8_t  oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t  num_fats;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    /* FAT32 specific */
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    uint8_t  volume_label[11];
    uint8_t  fs_type[8];
} fat32_boot_sector_t;

/* FSInfo structure */
typedef struct __attribute__((packed)) {
    uint32_t lead_signature;
    uint8_t  reserved1[480];
    uint32_t struct_signature;
    uint32_t free_count;
    uint32_t next_free;
    uint8_t  reserved2[12];
    uint32_t trail_signature;
} fat32_fsinfo_t;

/* Directory entry structure */
typedef struct __attribute__((packed)) {
    uint8_t  name[11];
    uint8_t  attributes;
    uint8_t  nt_reserved;
    uint8_t  create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} fat32_dir_entry_t;

/* Long filename entry */
typedef struct __attribute__((packed)) {
    uint8_t  order;
    uint16_t name1[5];
    uint8_t  attributes;
    uint8_t  type;
    uint8_t  checksum;
    uint16_t name2[6];
    uint16_t first_cluster_low;
    uint16_t name3[2];
} fat32_lfn_entry_t;

/* FAT32 mount information */
typedef struct {
    uint32_t first_data_sector;
    uint32_t first_fat_sector;
    uint32_t root_cluster;
    uint32_t sectors_per_cluster;
    uint32_t bytes_per_cluster;
    uint32_t total_clusters;
    uint32_t fat_size;
    uint8_t  num_fats;
    void*    device;
} fat32_mount_t;

/* FAT32 initialization */
void fat32_init(void);
int fat32_mount_device(const char* device);
int fat32_unmount_device(void);

/* Get FAT32 file system operations */
fs_ops_t* fat32_get_ops(void);

/* FAT32 utility functions */
uint32_t fat32_cluster_to_sector(fat32_mount_t* mount, uint32_t cluster);
uint32_t fat32_get_fat_entry(fat32_mount_t* mount, uint32_t cluster);
int fat32_set_fat_entry(fat32_mount_t* mount, uint32_t cluster, uint32_t value);
uint32_t fat32_allocate_cluster(fat32_mount_t* mount);
int fat32_free_cluster_chain(fat32_mount_t* mount, uint32_t start_cluster);

#endif /* AURORA_FAT32_H */
