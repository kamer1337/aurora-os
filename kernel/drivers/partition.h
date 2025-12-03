/**
 * Aurora OS - Disk Partition Manager
 * 
 * Provides disk partitioning support (MBR and GPT)
 */

#ifndef AURORA_PARTITION_H
#define AURORA_PARTITION_H

#include <stdint.h>

/* Partition types */
#define PART_TYPE_EMPTY      0x00
#define PART_TYPE_FAT32      0x0B
#define PART_TYPE_NTFS       0x07
#define PART_TYPE_LINUX      0x83
#define PART_TYPE_LINUX_SWAP 0x82
#define PART_TYPE_AURORA     0xAF

/* Partition table types */
#define PART_TABLE_MBR       0
#define PART_TABLE_GPT       1

/* Maximum partitions */
#define MAX_PARTITIONS       7  /* Fits in 512-byte sector with header */

/* Partition status */
#define PART_STATUS_INACTIVE 0x00
#define PART_STATUS_ACTIVE   0x80

/* Partition structure */
typedef struct {
    uint8_t status;        /* Boot indicator */
    uint8_t type;          /* Partition type */
    uint32_t start_lba;    /* Starting LBA sector */
    uint32_t size_sectors; /* Size in sectors */
    uint64_t size_bytes;   /* Size in bytes */
    uint8_t encrypted;     /* Encryption flag */
    char label[32];        /* Partition label */
} partition_t;

/* Disk information */
typedef struct {
    uint32_t total_sectors;
    uint32_t sector_size;
    uint64_t total_size;
    uint8_t table_type;    /* MBR or GPT */
    uint8_t partition_count;
    partition_t partitions[MAX_PARTITIONS];
} disk_info_t;

/* Initialize partition manager */
void partition_init(void);

/* Disk operations */
int partition_scan_disk(uint8_t disk_id);
const disk_info_t* partition_get_disk_info(uint8_t disk_id);

/* Partition operations */
int partition_create(uint8_t disk_id, uint32_t start_lba, uint32_t size_sectors, uint8_t type);
int partition_delete(uint8_t disk_id, uint8_t partition_id);
int partition_resize(uint8_t disk_id, uint8_t partition_id, uint32_t new_size_sectors);
int partition_format(uint8_t disk_id, uint8_t partition_id, uint8_t fs_type);

/* Partition table operations */
int partition_create_mbr(uint8_t disk_id);
int partition_create_gpt(uint8_t disk_id);
int partition_read_table(uint8_t disk_id);
int partition_write_table(uint8_t disk_id);

/* Utility functions */
uint64_t partition_get_free_space(uint8_t disk_id);
int partition_get_count(uint8_t disk_id);
const partition_t* partition_get_info(uint8_t disk_id, uint8_t partition_id);

#endif /* AURORA_PARTITION_H */
