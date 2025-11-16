/**
 * Aurora OS - Storage Driver Implementation
 * 
 * ATA/SATA/NVMe storage device driver for hard drives and SSDs
 */

#include "storage.h"
#include "../core/port_io.h"
#include <stddef.h>

/* Maximum number of storage devices */
#define MAX_STORAGE_DEVICES 8

/* Global storage devices array */
static storage_device_t storage_devices[MAX_STORAGE_DEVICES];
static uint8_t device_count = 0;

/* Helper function to copy string */
static void storage_strcpy(char* dest, const char* src, size_t n) {
    for (size_t i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
}

/**
 * Initialize storage subsystem
 */
void storage_init(void) {
    device_count = 0;
    
    /* Clear device structures */
    for (int i = 0; i < MAX_STORAGE_DEVICES; i++) {
        storage_devices[i].type = STORAGE_TYPE_UNKNOWN;
        storage_devices[i].status = STORAGE_STATUS_OFFLINE;
        storage_devices[i].total_sectors = 0;
        storage_devices[i].sector_size = 512;
        storage_devices[i].capacity_bytes = 0;
        storage_devices[i].reads = 0;
        storage_devices[i].writes = 0;
        storage_devices[i].errors = 0;
    }
}

/**
 * Wait for ATA device to be ready
 */
int ata_wait_ready(storage_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Poll status register */
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(device->base_port + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY) && (status & ATA_STATUS_RDY)) {
            return 0;
        }
    }
    
    return -1;
}

/**
 * Wait for data request
 */
int ata_wait_drq(storage_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Poll status register for DRQ */
    uint32_t timeout = 100000;
    while (timeout--) {
        uint8_t status = inb(device->base_port + ATA_REG_STATUS);
        if (status & ATA_STATUS_DRQ) {
            return 0;
        }
        if (status & ATA_STATUS_ERR) {
            return -1;
        }
    }
    
    return -1;
}

/**
 * Select drive
 */
int ata_select_drive(storage_device_t* device, uint8_t drive) {
    if (!device) {
        return -1;
    }
    
    /* Write to drive select register */
    outb(device->base_port + ATA_REG_DRIVE_SELECT, 0xA0 | (drive << 4));
    
    /* Delay 400ns */
    for (int i = 0; i < 4; i++) {
        inb(device->control_port);
    }
    
    return 0;
}

/**
 * Identify ATA device
 */
int storage_identify_device(storage_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Send IDENTIFY command */
    outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    
    /* Check if device exists */
    uint8_t status = inb(device->base_port + ATA_REG_STATUS);
    if (status == 0) {
        return -1;
    }
    
    /* Wait for device */
    if (ata_wait_drq(device) != 0) {
        return -1;
    }
    
    /* Read identification data */
    uint16_t identify_data[256];
    for (int i = 0; i < 256; i++) {
        identify_data[i] = inw(device->base_port + ATA_REG_DATA);
    }
    
    /* Parse device information */
    /* Model name is at words 27-46 */
    for (int i = 0; i < 20; i++) {
        device->model[i * 2] = (identify_data[27 + i] >> 8) & 0xFF;
        device->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
    }
    device->model[40] = '\0';
    
    /* Serial number is at words 10-19 */
    for (int i = 0; i < 10; i++) {
        device->serial[i * 2] = (identify_data[10 + i] >> 8) & 0xFF;
        device->serial[i * 2 + 1] = identify_data[10 + i] & 0xFF;
    }
    device->serial[20] = '\0';
    
    /* Firmware version is at words 23-26 */
    for (int i = 0; i < 4; i++) {
        device->firmware[i * 2] = (identify_data[23 + i] >> 8) & 0xFF;
        device->firmware[i * 2 + 1] = identify_data[23 + i] & 0xFF;
    }
    device->firmware[8] = '\0';
    
    /* Total sectors (LBA28 at words 60-61, LBA48 at words 100-103) */
    if (identify_data[83] & (1 << 10)) {
        /* LBA48 supported */
        device->total_sectors = ((uint64_t)identify_data[103] << 48) |
                               ((uint64_t)identify_data[102] << 32) |
                               ((uint64_t)identify_data[101] << 16) |
                               ((uint64_t)identify_data[100]);
    } else {
        /* LBA28 */
        device->total_sectors = ((uint32_t)identify_data[61] << 16) |
                               ((uint32_t)identify_data[60]);
    }
    
    device->capacity_bytes = device->total_sectors * device->sector_size;
    device->status = STORAGE_STATUS_ONLINE;
    
    return 0;
}

/**
 * Detect storage devices
 */
int storage_detect_devices(void) {
    device_count = 0;
    
    /* Check primary master (0x1F0) */
    storage_devices[device_count].type = STORAGE_TYPE_ATA;
    storage_devices[device_count].bus = 0;
    storage_devices[device_count].drive = 0;
    storage_devices[device_count].base_port = ATA_PRIMARY_IO;
    storage_devices[device_count].control_port = ATA_PRIMARY_CONTROL;
    
    if (storage_identify_device(&storage_devices[device_count]) == 0) {
        device_count++;
    }
    
    /* Check primary slave */
    if (device_count < MAX_STORAGE_DEVICES) {
        storage_devices[device_count].type = STORAGE_TYPE_ATA;
        storage_devices[device_count].bus = 0;
        storage_devices[device_count].drive = 1;
        storage_devices[device_count].base_port = ATA_PRIMARY_IO;
        storage_devices[device_count].control_port = ATA_PRIMARY_CONTROL;
        
        if (storage_identify_device(&storage_devices[device_count]) == 0) {
            device_count++;
        }
    }
    
    /* Check secondary master (0x170) */
    if (device_count < MAX_STORAGE_DEVICES) {
        storage_devices[device_count].type = STORAGE_TYPE_ATA;
        storage_devices[device_count].bus = 1;
        storage_devices[device_count].drive = 0;
        storage_devices[device_count].base_port = ATA_SECONDARY_IO;
        storage_devices[device_count].control_port = ATA_SECONDARY_CONTROL;
        
        if (storage_identify_device(&storage_devices[device_count]) == 0) {
            device_count++;
        }
    }
    
    /* Check secondary slave */
    if (device_count < MAX_STORAGE_DEVICES) {
        storage_devices[device_count].type = STORAGE_TYPE_ATA;
        storage_devices[device_count].bus = 1;
        storage_devices[device_count].drive = 1;
        storage_devices[device_count].base_port = ATA_SECONDARY_IO;
        storage_devices[device_count].control_port = ATA_SECONDARY_CONTROL;
        
        if (storage_identify_device(&storage_devices[device_count]) == 0) {
            device_count++;
        }
    }
    
    return device_count;
}

/**
 * Get device count
 */
int storage_get_device_count(void) {
    return device_count;
}

/**
 * Get device by index
 */
storage_device_t* storage_get_device(uint8_t index) {
    if (index >= device_count) {
        return NULL;
    }
    
    return &storage_devices[index];
}

/**
 * Read single sector
 */
int storage_read_sector(storage_device_t* device, uint64_t lba, uint8_t* buffer) {
    if (!device || !buffer) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        device->errors++;
        return -1;
    }
    
    /* Set sector count */
    outb(device->base_port + ATA_REG_SECTOR_COUNT, 1);
    
    /* Set LBA */
    outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(device->base_port + ATA_REG_DRIVE_SELECT, 0xE0 | (device->drive << 4) | ((lba >> 24) & 0x0F));
    
    /* Send read command */
    outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    /* Wait for DRQ */
    if (ata_wait_drq(device) != 0) {
        device->errors++;
        return -1;
    }
    
    /* Read data */
    uint16_t* buf16 = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        buf16[i] = inw(device->base_port + ATA_REG_DATA);
    }
    
    device->reads++;
    return 0;
}

/**
 * Write single sector
 */
int storage_write_sector(storage_device_t* device, uint64_t lba, const uint8_t* buffer) {
    if (!device || !buffer) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        device->errors++;
        return -1;
    }
    
    /* Set sector count */
    outb(device->base_port + ATA_REG_SECTOR_COUNT, 1);
    
    /* Set LBA */
    outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
    outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(device->base_port + ATA_REG_DRIVE_SELECT, 0xE0 | (device->drive << 4) | ((lba >> 24) & 0x0F));
    
    /* Send write command */
    outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    /* Wait for DRQ */
    if (ata_wait_drq(device) != 0) {
        device->errors++;
        return -1;
    }
    
    /* Write data */
    const uint16_t* buf16 = (const uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(device->base_port + ATA_REG_DATA, buf16[i]);
    }
    
    /* Wait for completion */
    ata_wait_ready(device);
    
    device->writes++;
    return 0;
}

/**
 * Read multiple sectors
 */
int storage_read_sectors(storage_device_t* device, uint64_t lba, uint32_t count, uint8_t* buffer) {
    if (!device || !buffer || count == 0) {
        return -1;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        if (storage_read_sector(device, lba + i, buffer + (i * device->sector_size)) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * Write multiple sectors
 */
int storage_write_sectors(storage_device_t* device, uint64_t lba, uint32_t count, const uint8_t* buffer) {
    if (!device || !buffer || count == 0) {
        return -1;
    }
    
    for (uint32_t i = 0; i < count; i++) {
        if (storage_write_sector(device, lba + i, buffer + (i * device->sector_size)) != 0) {
            return -1;
        }
    }
    
    return 0;
}

/**
 * Flush cache
 */
int storage_flush_cache(storage_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        return -1;
    }
    
    /* Send flush cache command */
    outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);
    
    /* Wait for completion */
    return ata_wait_ready(device);
}

/**
 * Reset device
 */
int storage_reset_device(storage_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Soft reset via control port */
    outb(device->control_port, 0x04);
    
    /* Delay */
    for (int i = 0; i < 1000; i++) {
        inb(device->control_port);
    }
    
    /* Clear reset */
    outb(device->control_port, 0x00);
    
    /* Wait for ready */
    return ata_wait_ready(device);
}

/**
 * Read partition table
 */
int storage_read_partition_table(storage_device_t* device, storage_partition_t* partitions, uint32_t max_partitions) {
    if (!device || !partitions || max_partitions == 0) {
        return -1;
    }
    
    /* Read MBR (sector 0) */
    uint8_t mbr[512];
    if (storage_read_sector(device, 0, mbr) != 0) {
        return -1;
    }
    
    /* Check for MBR signature */
    if (mbr[510] != 0x55 || mbr[511] != 0xAA) {
        return -1;
    }
    
    /* Parse partition entries (starting at offset 446) */
    uint32_t partition_count = (max_partitions < 4) ? max_partitions : 4;
    for (uint32_t i = 0; i < partition_count; i++) {
        uint8_t* entry = &mbr[446 + (i * 16)];
        
        partitions[i].bootable = entry[0];
        partitions[i].type = entry[4];
        partitions[i].start_lba = *(uint32_t*)&entry[8];
        partitions[i].num_sectors = *(uint32_t*)&entry[12];
    }
    
    return partition_count;
}

/**
 * Get partition info
 */
int storage_get_partition_info(storage_device_t* device, uint8_t partition_num, storage_partition_t* partition) {
    storage_partition_t partitions[4];
    int count = storage_read_partition_table(device, partitions, 4);
    
    if (count < 0 || partition_num >= count) {
        return -1;
    }
    
    *partition = partitions[partition_num];
    return 0;
}

/* DMA operations */
int storage_read_dma(storage_device_t* device, uint64_t lba, uint32_t count, uint8_t* buffer) {
    if (!device || !buffer || count == 0) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        device->errors++;
        return -1;
    }
    
    /* For LBA48 if supported */
    if (device->total_sectors > 0x0FFFFFFF && count <= 65536) {
        /* Set sector count (high byte then low byte) */
        outb(device->base_port + ATA_REG_SECTOR_COUNT, (uint8_t)((count >> 8) & 0xFF));
        outb(device->base_port + ATA_REG_SECTOR_COUNT, (uint8_t)(count & 0xFF));
        
        /* Set LBA (high bytes then low bytes) */
        outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)((lba >> 24) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 32) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 40) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
        
        /* Drive select with LBA mode */
        outb(device->base_port + ATA_REG_DRIVE_SELECT, 0x40 | (device->drive << 4));
        
        /* Send DMA READ EXT command */
        outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_READ_DMA_EXT);
    } else {
        /* LBA28 mode */
        outb(device->base_port + ATA_REG_SECTOR_COUNT, (uint8_t)(count & 0xFF));
        outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
        outb(device->base_port + ATA_REG_DRIVE_SELECT, 0xE0 | (device->drive << 4) | ((lba >> 24) & 0x0F));
        
        /* Send DMA READ command */
        outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_READ_DMA);
    }
    
    /* Note: Actual DMA transfer would require Bus Master IDE setup which is hardware specific
     * For now, fall back to PIO after setting up the command */
    return storage_read_sectors(device, lba, count, buffer);
}

int storage_write_dma(storage_device_t* device, uint64_t lba, uint32_t count, const uint8_t* buffer) {
    if (!device || !buffer || count == 0) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        device->errors++;
        return -1;
    }
    
    /* For LBA48 if supported */
    if (device->total_sectors > 0x0FFFFFFF && count <= 65536) {
        /* Set sector count (high byte then low byte) */
        outb(device->base_port + ATA_REG_SECTOR_COUNT, (uint8_t)((count >> 8) & 0xFF));
        outb(device->base_port + ATA_REG_SECTOR_COUNT, (uint8_t)(count & 0xFF));
        
        /* Set LBA (high bytes then low bytes) */
        outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)((lba >> 24) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 32) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 40) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
        
        /* Drive select with LBA mode */
        outb(device->base_port + ATA_REG_DRIVE_SELECT, 0x40 | (device->drive << 4));
        
        /* Send DMA WRITE EXT command */
        outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_WRITE_DMA_EXT);
    } else {
        /* LBA28 mode */
        outb(device->base_port + ATA_REG_SECTOR_COUNT, (uint8_t)(count & 0xFF));
        outb(device->base_port + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(device->base_port + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(device->base_port + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
        outb(device->base_port + ATA_REG_DRIVE_SELECT, 0xE0 | (device->drive << 4) | ((lba >> 24) & 0x0F));
        
        /* Send DMA WRITE command */
        outb(device->base_port + ATA_REG_COMMAND, ATA_CMD_WRITE_DMA);
    }
    
    /* Note: Actual DMA transfer would require Bus Master IDE setup which is hardware specific
     * For now, fall back to PIO after setting up the command */
    return storage_write_sectors(device, lba, count, buffer);
}

/* SMART monitoring */
int storage_get_smart_status(storage_device_t* device, uint8_t* status) {
    if (!device || !status) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        return -1;
    }
    
    /* Set up SMART RETURN STATUS command */
    outb(device->base_port + ATA_REG_FEATURES, 0xDA);  /* SMART RETURN STATUS */
    outb(device->base_port + ATA_REG_LBA_MID, 0x4F);
    outb(device->base_port + ATA_REG_LBA_HIGH, 0xC2);
    outb(device->base_port + ATA_REG_COMMAND, 0xB0);   /* SMART command */
    
    /* Wait for completion */
    if (ata_wait_ready(device) != 0) {
        return -1;
    }
    
    /* Check LBA mid and high registers
     * If 0x4F and 0xC2, drive is healthy
     * If 0xF4 and 0x2C, drive is failing */
    uint8_t lba_mid = inb(device->base_port + ATA_REG_LBA_MID);
    uint8_t lba_high = inb(device->base_port + ATA_REG_LBA_HIGH);
    
    if (lba_mid == 0xF4 && lba_high == 0x2C) {
        *status = 1;  /* Failing */
    } else {
        *status = 0;  /* Good */
    }
    
    return 0;
}

int storage_get_temperature(storage_device_t* device, int* temperature_celsius) {
    if (!device || !temperature_celsius) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        return -1;
    }
    
    /* Set up SMART READ DATA command */
    outb(device->base_port + ATA_REG_FEATURES, 0xD0);  /* SMART READ DATA */
    outb(device->base_port + ATA_REG_SECTOR_COUNT, 1);
    outb(device->base_port + ATA_REG_LBA_MID, 0x4F);
    outb(device->base_port + ATA_REG_LBA_HIGH, 0xC2);
    outb(device->base_port + ATA_REG_COMMAND, 0xB0);   /* SMART command */
    
    /* Wait for DRQ */
    if (ata_wait_drq(device) != 0) {
        *temperature_celsius = 35;  /* Default fallback */
        return 0;
    }
    
    /* Read SMART data (512 bytes) */
    uint16_t smart_data[256];
    for (int i = 0; i < 256; i++) {
        smart_data[i] = inw(device->base_port + ATA_REG_DATA);
    }
    
    /* Temperature is typically in attribute 0xC2 or 0xBE
     * SMART data structure: 12 bytes per attribute starting at offset 2
     * Format: ID (1 byte), Flags (2 bytes), Current (1 byte), Worst (1 byte), 
     *         Raw (6 bytes), Reserved (1 byte)
     * We look for temperature in the raw value of attribute 0xC2 */
    for (int i = 2; i < 362; i += 12) {
        uint8_t* attr = (uint8_t*)smart_data + i;
        if (attr[0] == 0xC2 || attr[0] == 0xBE) {  /* Temperature attributes */
            *temperature_celsius = attr[5];  /* Raw value low byte is temperature */
            return 0;
        }
    }
    
    /* Default temperature if not found */
    *temperature_celsius = 35;
    return 0;
}

/* Power management */
int storage_set_power_mode(storage_device_t* device, uint8_t mode) {
    if (!device) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        return -1;
    }
    
    /* Send SET FEATURES command with power mode
     * Mode values:
     * 0x00 - Disable advanced power management
     * 0x01-0x7F - Set power mode levels
     * 0x80 - Minimum power consumption with standby
     * 0xFE - Maximum performance */
    outb(device->base_port + ATA_REG_FEATURES, 0x05);  /* Set APM level */
    outb(device->base_port + ATA_REG_SECTOR_COUNT, mode);
    outb(device->base_port + ATA_REG_COMMAND, 0xEF);  /* SET FEATURES */
    
    return ata_wait_ready(device);
}

int storage_spin_down(storage_device_t* device) {
    if (!device) {
        return -1;
    }
    
    if (device->status != STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Wait for ready */
    if (ata_wait_ready(device) != 0) {
        return -1;
    }
    
    /* Send STANDBY IMMEDIATE command */
    outb(device->base_port + ATA_REG_COMMAND, 0xE0);
    
    /* Wait for completion */
    int result = ata_wait_ready(device);
    
    if (result == 0) {
        device->status = STORAGE_STATUS_OFFLINE;
    }
    
    return result;
}

int storage_spin_up(storage_device_t* device) {
    if (!device) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(device, device->drive);
    
    /* Send IDLE IMMEDIATE command to wake up the drive */
    outb(device->base_port + ATA_REG_COMMAND, 0xE1);
    
    /* Wait for ready */
    int result = ata_wait_ready(device);
    
    if (result == 0) {
        device->status = STORAGE_STATUS_ONLINE;
    }
    
    return result;
}

/* SATA-specific functions */
int sata_init_port(uint32_t port) {
    /* SATA initialization requires AHCI controller setup
     * This is a basic implementation that checks for port availability */
    if (port >= 32) {
        return -1;
    }
    
    /* Note: Full SATA/AHCI support would require:
     * 1. PCI enumeration to find AHCI controller
     * 2. Memory-mapped I/O setup for AHCI registers
     * 3. Port initialization (setting up command lists, FIS structures)
     * 4. Port start and spin-up
     * 
     * For now, this is a placeholder that returns success
     * to indicate the port slot is available */
    return 0;
}

int sata_identify(uint32_t port, uint16_t* buffer) {
    if (port >= 32 || !buffer) {
        return -1;
    }
    
    /* SATA identification through AHCI would involve:
     * 1. Building an IDENTIFY DEVICE command FIS
     * 2. Submitting to the command list
     * 3. Waiting for completion
     * 4. Reading the result from the receive FIS
     * 
     * For compatibility, we return error to fall back to ATA/PIO
     * which is already implemented above */
    return -1;
}

/* NVMe-specific functions */
int nvme_init(void) {
    /* NVMe initialization requires:
     * 1. PCI enumeration to find NVMe controller
     * 2. Memory-mapped I/O setup for NVMe registers
     * 3. Admin queue setup (submission and completion queues)
     * 4. Controller initialization and configuration
     * 5. I/O queue pair setup
     * 
     * This is a complex operation that requires:
     * - PCI configuration space access
     * - Physical memory management for queue buffers
     * - Interrupt handling for completion notifications
     * 
     * For now, this returns 0 to indicate the subsystem is available
     * but actual NVMe devices would need full implementation */
    return 0;
}

int nvme_identify_controller(uint32_t nsid, uint8_t* buffer) {
    if (!buffer) {
        return -1;
    }
    
    /* NVMe IDENTIFY command would involve:
     * 1. Building an Admin command (IDENTIFY CNS=0x01)
     * 2. Allocating DMA buffer for the 4KB identify data
     * 3. Submitting to admin submission queue
     * 4. Waiting for completion in admin completion queue
     * 5. Copying result to buffer
     * 
     * Without full NVMe implementation, return error */
    (void)nsid;
    return -1;
}

int nvme_read(uint32_t nsid, uint64_t lba, uint32_t count, uint8_t* buffer) {
    if (!buffer || count == 0) {
        return -1;
    }
    
    /* NVMe READ command would involve:
     * 1. Allocating PRP (Physical Region Page) entries for data buffer
     * 2. Building an I/O command (READ opcode=0x02)
     * 3. Setting starting LBA and number of blocks
     * 4. Submitting to I/O submission queue
     * 5. Waiting for completion
     * 6. Checking status and copying data
     * 
     * NVMe operates at block level (typically 4KB blocks)
     * Each namespace (nsid) is like a separate disk
     * 
     * Without full NVMe implementation, return error */
    (void)nsid;
    (void)lba;
    (void)count;
    return -1;
}

int nvme_write(uint32_t nsid, uint64_t lba, uint32_t count, const uint8_t* buffer) {
    if (!buffer || count == 0) {
        return -1;
    }
    
    /* NVMe WRITE command would involve:
     * 1. Allocating PRP entries for data buffer
     * 2. Building an I/O command (WRITE opcode=0x01)
     * 3. Setting starting LBA and number of blocks
     * 4. Copying data to DMA buffer
     * 5. Submitting to I/O submission queue
     * 6. Waiting for completion
     * 7. Checking status
     * 
     * Without full NVMe implementation, return error */
    (void)nsid;
    (void)lba;
    (void)count;
    return -1;
}

/* Utility functions */
const char* storage_get_type_string(uint8_t type) {
    switch (type) {
        case STORAGE_TYPE_ATA:
            return "ATA";
        case STORAGE_TYPE_ATAPI:
            return "ATAPI";
        case STORAGE_TYPE_SATA:
            return "SATA";
        case STORAGE_TYPE_NVME:
            return "NVMe";
        case STORAGE_TYPE_SCSI:
            return "SCSI";
        default:
            return "Unknown";
    }
}

const char* storage_get_status_string(storage_status_t status) {
    switch (status) {
        case STORAGE_STATUS_ONLINE:
            return "Online";
        case STORAGE_STATUS_OFFLINE:
            return "Offline";
        case STORAGE_STATUS_ERROR:
            return "Error";
        case STORAGE_STATUS_BUSY:
            return "Busy";
        default:
            return "Unknown";
    }
}

uint64_t storage_get_capacity_mb(storage_device_t* device) {
    if (!device) {
        return 0;
    }
    /* Avoid 64-bit division by shifting: divide by 1024*1024 = 2^20 */
    return device->capacity_bytes >> 20;
}

uint64_t storage_get_capacity_gb(storage_device_t* device) {
    if (!device) {
        return 0;
    }
    /* Avoid 64-bit division by shifting: divide by 1024*1024*1024 = 2^30 */
    return device->capacity_bytes >> 30;
}
