/**
 * Aurora OS - Disk Partition Manager Implementation
 */

#include "partition.h"
#include "storage.h"
#include <stddef.h>

/* Maximum supported disks */
#define MAX_DISKS 4

/* Partition table location on disk (LBA 1) */
#define PARTITION_TABLE_LBA 1

/* Aurora OS partition table magic signature */
#define AURORA_PART_MAGIC 0x41555250  /* "AURP" */

/* Sector size */
#define SECTOR_SIZE 512

/* Persistent partition table structure (must fit in 512 bytes) */
typedef struct {
    uint32_t magic;                 /* Magic signature */
    uint32_t version;               /* Table version */
    uint32_t checksum;              /* CRC32 checksum */
    uint8_t table_type;             /* MBR or GPT */
    uint8_t partition_count;        /* Number of partitions */
    uint8_t reserved[6];            /* Reserved for alignment */
    partition_t partitions[MAX_PARTITIONS];  /* Partition entries (max 7 to fit in 512 bytes) */
} persistent_partition_table_t;

/* Compile-time check to ensure structure fits in a sector */
_Static_assert(sizeof(persistent_partition_table_t) <= SECTOR_SIZE, 
               "Partition table structure exceeds 512-byte sector size");

/* Global disk information */
static disk_info_t disks[MAX_DISKS];
static int initialized = 0;

/* Helper function to copy string safely */
static void safe_strcpy(char* dest, const char* src, size_t max_len) {
    size_t i;
    for (i = 0; i < max_len - 1 && src && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';
}

/**
 * Calculate simple CRC32 checksum
 */
static uint32_t calculate_checksum(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
        }
    }
    
    return ~crc;
}

/**
 * Clear memory block
 */
static void memset_zero(void* ptr, size_t size) {
    uint8_t* p = (uint8_t*)ptr;
    for (size_t i = 0; i < size; i++) {
        p[i] = 0;
    }
}

/**
 * Copy memory block
 */
static void memcpy_block(void* dest, const void* src, size_t size) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < size; i++) {
        d[i] = s[i];
    }
}

/**
 * Initialize partition manager
 */
void partition_init(void) {
    /* Clear disk information */
    for (int i = 0; i < MAX_DISKS; i++) {
        disks[i].total_sectors = 0;
        disks[i].sector_size = 512;  /* Default sector size */
        disks[i].total_size = 0;
        disks[i].table_type = PART_TABLE_MBR;
        disks[i].partition_count = 0;
    }
    
    initialized = 1;
}

/**
 * Scan disk for partitions
 */
int partition_scan_disk(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    /* In a real implementation, this would read the partition table
     * from the disk using the storage driver */
    
    /* For now, simulate a disk with some default values */
    disk_info_t* disk = &disks[disk_id];
    disk->total_sectors = 2097152;  /* 1GB with 512-byte sectors */
    disk->sector_size = 512;
    disk->total_size = (uint64_t)disk->total_sectors * disk->sector_size;
    disk->table_type = PART_TABLE_MBR;
    disk->partition_count = 0;
    
    return 0;
}

/**
 * Get disk information
 */
const disk_info_t* partition_get_disk_info(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return NULL;
    }
    
    return &disks[disk_id];
}

/**
 * Create partition
 */
int partition_create(uint8_t disk_id, uint32_t start_lba, uint32_t size_sectors, uint8_t type) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    disk_info_t* disk = &disks[disk_id];
    
    if (disk->partition_count >= MAX_PARTITIONS) {
        return -2;  /* Too many partitions */
    }
    
    /* Check if partition fits on disk */
    if (start_lba + size_sectors > disk->total_sectors) {
        return -3;  /* Partition too large */
    }
    
    /* Check for overlaps with existing partitions */
    for (int i = 0; i < disk->partition_count; i++) {
        partition_t* p = &disk->partitions[i];
        uint32_t p_end = p->start_lba + p->size_sectors;
        uint32_t new_end = start_lba + size_sectors;
        
        if ((start_lba >= p->start_lba && start_lba < p_end) ||
            (new_end > p->start_lba && new_end <= p_end) ||
            (start_lba <= p->start_lba && new_end >= p_end)) {
            return -4;  /* Partition overlap */
        }
    }
    
    /* Create partition */
    partition_t* new_part = &disk->partitions[disk->partition_count];
    new_part->status = PART_STATUS_INACTIVE;
    new_part->type = type;
    new_part->start_lba = start_lba;
    new_part->size_sectors = size_sectors;
    new_part->size_bytes = (uint64_t)size_sectors * disk->sector_size;
    new_part->encrypted = 0;
    safe_strcpy(new_part->label, "New Partition", 32);
    
    disk->partition_count++;
    
    /* Persist partition table to disk */
    int result = partition_write_table(disk_id);
    if (result != 0) {
        /* Rollback on write failure */
        disk->partition_count--;
        return -5;  /* Failed to persist */
    }
    
    return disk->partition_count - 1;  /* Return partition ID */
}

/**
 * Delete partition
 */
int partition_delete(uint8_t disk_id, uint8_t partition_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    disk_info_t* disk = &disks[disk_id];
    
    if (partition_id >= disk->partition_count) {
        return -2;  /* Invalid partition */
    }
    
    /* Shift partitions down */
    for (int i = partition_id; i < disk->partition_count - 1; i++) {
        disk->partitions[i] = disk->partitions[i + 1];
    }
    
    disk->partition_count--;
    
    /* Persist partition table to disk */
    int result = partition_write_table(disk_id);
    if (result != 0) {
        /* Rollback on write failure */
        disk->partition_count++;
        return -3;  /* Failed to persist */
    }
    
    return 0;
}

/**
 * Resize partition
 */
int partition_resize(uint8_t disk_id, uint8_t partition_id, uint32_t new_size_sectors) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    disk_info_t* disk = &disks[disk_id];
    
    if (partition_id >= disk->partition_count) {
        return -2;  /* Invalid partition */
    }
    
    partition_t* part = &disk->partitions[partition_id];
    uint32_t old_size = part->size_sectors;
    
    /* Check if new size fits */
    if (part->start_lba + new_size_sectors > disk->total_sectors) {
        return -3;  /* New size too large */
    }
    
    /* In a real implementation, would check for conflicts with adjacent partitions */
    
    part->size_sectors = new_size_sectors;
    part->size_bytes = (uint64_t)new_size_sectors * disk->sector_size;
    
    /* Persist partition table to disk */
    int result = partition_write_table(disk_id);
    if (result != 0) {
        /* Rollback on write failure */
        part->size_sectors = old_size;
        part->size_bytes = (uint64_t)old_size * disk->sector_size;
        return -4;  /* Failed to persist */
    }
    
    return 0;
}

/**
 * Format partition
 */
int partition_format(uint8_t disk_id, uint8_t partition_id, uint8_t fs_type) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    disk_info_t* disk = &disks[disk_id];
    
    if (partition_id >= disk->partition_count) {
        return -2;  /* Invalid partition */
    }
    
    /* In a real implementation, would format the partition with the specified filesystem */
    /* For now, just update the partition type */
    disk->partitions[partition_id].type = fs_type;
    
    return 0;
}

/**
 * Create MBR partition table
 */
int partition_create_mbr(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    disk_info_t* disk = &disks[disk_id];
    
    /* Clear existing partitions */
    disk->partition_count = 0;
    disk->table_type = PART_TABLE_MBR;
    
    /* Persist empty partition table to disk */
    int result = partition_write_table(disk_id);
    if (result != 0) {
        return -2;  /* Failed to persist */
    }
    
    return 0;
}

/**
 * Create GPT partition table
 */
int partition_create_gpt(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    disk_info_t* disk = &disks[disk_id];
    
    /* Clear existing partitions */
    disk->partition_count = 0;
    disk->table_type = PART_TABLE_GPT;
    
    /* Persist empty partition table to disk */
    int result = partition_write_table(disk_id);
    if (result != 0) {
        return -2;  /* Failed to persist */
    }
    
    return 0;
}

/**
 * Read partition table from disk
 */
int partition_read_table(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    /* Get storage device */
    storage_device_t* device = storage_get_device(disk_id);
    if (!device) {
        return -2;  /* No storage device */
    }
    
    /* Allocate buffer for reading partition table */
    uint8_t buffer[512];
    
    /* Read partition table from LBA 1 */
    if (storage_read_sector(device, PARTITION_TABLE_LBA, buffer) != 0) {
        return -3;  /* Read error */
    }
    
    /* Parse persistent partition table */
    persistent_partition_table_t* table = (persistent_partition_table_t*)buffer;
    
    /* Verify magic signature */
    if (table->magic != AURORA_PART_MAGIC) {
        /* No valid partition table found, initialize empty */
        disks[disk_id].partition_count = 0;
        disks[disk_id].table_type = PART_TABLE_MBR;
        return 0;
    }
    
    /* Calculate and verify checksum */
    uint32_t saved_checksum = table->checksum;
    table->checksum = 0;
    uint32_t calculated_checksum = calculate_checksum(buffer, SECTOR_SIZE);
    
    if (saved_checksum != calculated_checksum) {
        return -4;  /* Checksum mismatch */
    }
    
    /* Load partition table into memory */
    disk_info_t* disk = &disks[disk_id];
    disk->table_type = table->table_type;
    disk->partition_count = table->partition_count;
    
    /* Copy partition entries */
    for (int i = 0; i < table->partition_count && i < MAX_PARTITIONS; i++) {
        memcpy_block(&disk->partitions[i], &table->partitions[i], sizeof(partition_t));
    }
    
    return 0;
}

/**
 * Write partition table to disk
 */
int partition_write_table(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    /* Get storage device */
    storage_device_t* device = storage_get_device(disk_id);
    if (!device) {
        return -2;  /* No storage device */
    }
    
    /* Prepare persistent partition table */
    uint8_t buffer[512];
    memset_zero(buffer, 512);
    
    persistent_partition_table_t* table = (persistent_partition_table_t*)buffer;
    disk_info_t* disk = &disks[disk_id];
    
    /* Set header fields */
    table->magic = AURORA_PART_MAGIC;
    table->version = 1;
    table->table_type = disk->table_type;
    table->partition_count = disk->partition_count;
    
    /* Copy partition entries */
    for (int i = 0; i < disk->partition_count && i < MAX_PARTITIONS; i++) {
        memcpy_block(&table->partitions[i], &disk->partitions[i], sizeof(partition_t));
    }
    
    /* Calculate checksum */
    table->checksum = 0;
    table->checksum = calculate_checksum(buffer, SECTOR_SIZE);
    
    /* Write partition table to LBA 1 */
    if (storage_write_sector(device, PARTITION_TABLE_LBA, buffer) != 0) {
        return -3;  /* Write error */
    }
    
    /* Flush cache to ensure data is written */
    storage_flush_cache(device);
    
    return 0;
}

/**
 * Get free space on disk
 */
uint64_t partition_get_free_space(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return 0;
    }
    
    disk_info_t* disk = &disks[disk_id];
    uint64_t used_sectors = 0;
    
    for (int i = 0; i < disk->partition_count; i++) {
        used_sectors += disk->partitions[i].size_sectors;
    }
    
    uint64_t free_sectors = disk->total_sectors - used_sectors;
    return free_sectors * disk->sector_size;
}

/**
 * Get partition count
 */
int partition_get_count(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    return disks[disk_id].partition_count;
}

/**
 * Get partition information
 */
const partition_t* partition_get_info(uint8_t disk_id, uint8_t partition_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return NULL;
    }
    
    disk_info_t* disk = &disks[disk_id];
    
    if (partition_id >= disk->partition_count) {
        return NULL;
    }
    
    return &disk->partitions[partition_id];
}
