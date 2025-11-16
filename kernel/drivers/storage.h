/**
 * Aurora OS - Storage Driver Header
 * 
 * ATA/SATA/NVMe storage device driver for hard drives and SSDs
 */

#ifndef AURORA_STORAGE_H
#define AURORA_STORAGE_H

#include <stdint.h>

/* Storage device types */
#define STORAGE_TYPE_UNKNOWN   0
#define STORAGE_TYPE_ATA       1
#define STORAGE_TYPE_ATAPI     2
#define STORAGE_TYPE_SATA      3
#define STORAGE_TYPE_NVME      4
#define STORAGE_TYPE_SCSI      5

/* Storage device status */
typedef enum {
    STORAGE_STATUS_OFFLINE,
    STORAGE_STATUS_ONLINE,
    STORAGE_STATUS_ERROR,
    STORAGE_STATUS_BUSY
} storage_status_t;

/* ATA commands */
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_FLUSH_CACHE     0xE7

/* ATA ports */
#define ATA_PRIMARY_IO          0x1F0
#define ATA_PRIMARY_CONTROL     0x3F6
#define ATA_SECONDARY_IO        0x170
#define ATA_SECONDARY_CONTROL   0x376

/* ATA registers */
#define ATA_REG_DATA            0x00
#define ATA_REG_ERROR           0x01
#define ATA_REG_FEATURES        0x01
#define ATA_REG_SECTOR_COUNT    0x02
#define ATA_REG_LBA_LOW         0x03
#define ATA_REG_LBA_MID         0x04
#define ATA_REG_LBA_HIGH        0x05
#define ATA_REG_DRIVE_SELECT    0x06
#define ATA_REG_STATUS          0x07
#define ATA_REG_COMMAND         0x07

/* ATA status bits */
#define ATA_STATUS_ERR          0x01
#define ATA_STATUS_DRQ          0x08
#define ATA_STATUS_SRV          0x10
#define ATA_STATUS_DF           0x20
#define ATA_STATUS_RDY          0x40
#define ATA_STATUS_BSY          0x80

/* Storage device structure */
typedef struct {
    uint8_t type;
    uint8_t bus;
    uint8_t drive;
    uint16_t base_port;
    uint16_t control_port;
    storage_status_t status;
    
    /* Device information */
    uint64_t total_sectors;
    uint32_t sector_size;
    uint64_t capacity_bytes;
    
    /* Device identification */
    char model[41];
    char serial[21];
    char firmware[9];
    
    /* Statistics */
    uint64_t reads;
    uint64_t writes;
    uint64_t errors;
} storage_device_t;

/* Partition structure */
typedef struct {
    uint8_t bootable;
    uint8_t type;
    uint32_t start_lba;
    uint32_t num_sectors;
} storage_partition_t;

/* Storage subsystem initialization */
void storage_init(void);

/* Device detection and identification */
int storage_detect_devices(void);
int storage_get_device_count(void);
storage_device_t* storage_get_device(uint8_t index);

/* Device operations */
int storage_identify_device(storage_device_t* device);
int storage_reset_device(storage_device_t* device);

/* Read/Write operations */
int storage_read_sector(storage_device_t* device, uint64_t lba, uint8_t* buffer);
int storage_write_sector(storage_device_t* device, uint64_t lba, const uint8_t* buffer);
int storage_read_sectors(storage_device_t* device, uint64_t lba, uint32_t count, uint8_t* buffer);
int storage_write_sectors(storage_device_t* device, uint64_t lba, uint32_t count, const uint8_t* buffer);

/* DMA operations */
int storage_read_dma(storage_device_t* device, uint64_t lba, uint32_t count, uint8_t* buffer);
int storage_write_dma(storage_device_t* device, uint64_t lba, uint32_t count, const uint8_t* buffer);

/* Cache operations */
int storage_flush_cache(storage_device_t* device);

/* Partition management */
int storage_read_partition_table(storage_device_t* device, storage_partition_t* partitions, uint32_t max_partitions);
int storage_get_partition_info(storage_device_t* device, uint8_t partition_num, storage_partition_t* partition);

/* SMART monitoring */
int storage_get_smart_status(storage_device_t* device, uint8_t* status);
int storage_get_temperature(storage_device_t* device, int* temperature_celsius);

/* Utility functions */
const char* storage_get_type_string(uint8_t type);
const char* storage_get_status_string(storage_status_t status);
uint64_t storage_get_capacity_mb(storage_device_t* device);
uint64_t storage_get_capacity_gb(storage_device_t* device);

#endif /* AURORA_STORAGE_H */
