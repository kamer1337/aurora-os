/**
 * Aurora OS - Disk Partition Manager Implementation
 */

#include "partition.h"
#include <stddef.h>

/* Maximum supported disks */
#define MAX_DISKS 4

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
    
    /* Check if new size fits */
    if (part->start_lba + new_size_sectors > disk->total_sectors) {
        return -3;  /* New size too large */
    }
    
    /* In a real implementation, would check for conflicts with adjacent partitions */
    
    part->size_sectors = new_size_sectors;
    part->size_bytes = (uint64_t)new_size_sectors * disk->sector_size;
    
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
    
    /* In a real implementation, would write MBR to disk */
    
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
    
    /* In a real implementation, would write GPT to disk */
    
    return 0;
}

/**
 * Read partition table from disk
 */
int partition_read_table(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    /* In a real implementation, would read partition table from disk */
    /* For now, use partition_scan_disk */
    return partition_scan_disk(disk_id);
}

/**
 * Write partition table to disk
 */
int partition_write_table(uint8_t disk_id) {
    if (!initialized || disk_id >= MAX_DISKS) {
        return -1;
    }
    
    /* In a real implementation, would write partition table to disk */
    
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
