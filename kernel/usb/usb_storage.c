/**
 * Aurora OS - USB Mass Storage Driver Implementation
 * 
 * USB Mass Storage Class (MSC) device support using
 * Bulk-Only Transport (BOT) protocol with SCSI commands
 */

#include "usb_storage.h"
#include "usb.h"
#include "../memory/memory.h"
#include <stddef.h>

/* Maximum number of USB storage devices */
#define MAX_USB_STORAGE_DEVICES 8

/* USB storage device table */
static usb_storage_device_t usb_storage_devices[MAX_USB_STORAGE_DEVICES];
static uint8_t usb_storage_device_count = 0;
static uint8_t usb_storage_initialized = 0;

/* Helper: Convert 32-bit value from big-endian to host byte order */
static uint32_t be32_to_cpu(uint32_t val) {
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

/* Helper: Copy memory (kernel doesn't have memcpy) */
static void usb_storage_memcpy(void* dest, const void* src, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

/* Helper: Set memory to zero (kernel doesn't have memset) */
static void usb_storage_memset(void* dest, uint8_t val, uint32_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (uint32_t i = 0; i < n; i++) {
        d[i] = val;
    }
}

/**
 * Initialize USB mass storage subsystem
 */
void usb_storage_init(void) {
    if (usb_storage_initialized) {
        return;
    }
    
    /* Clear device table */
    for (int i = 0; i < MAX_USB_STORAGE_DEVICES; i++) {
        usb_storage_devices[i].usb_dev = NULL;
        usb_storage_devices[i].status = USB_STORAGE_STATUS_OFFLINE;
        usb_storage_devices[i].lun_count = 0;
        usb_storage_devices[i].bulk_in_ep = 0;
        usb_storage_devices[i].bulk_out_ep = 0;
        usb_storage_devices[i].bulk_max_packet = 64;
        usb_storage_devices[i].total_blocks = 0;
        usb_storage_devices[i].block_size = 512;
        usb_storage_devices[i].capacity_bytes = 0;
        usb_storage_devices[i].tag = 1;
        usb_storage_devices[i].reads = 0;
        usb_storage_devices[i].writes = 0;
        usb_storage_devices[i].errors = 0;
        usb_storage_devices[i].next = NULL;
    }
    
    usb_storage_device_count = 0;
    usb_storage_initialized = 1;
}

/**
 * Allocate a USB storage device slot
 */
static usb_storage_device_t* alloc_usb_storage_device(void) {
    for (int i = 0; i < MAX_USB_STORAGE_DEVICES; i++) {
        if (usb_storage_devices[i].status == USB_STORAGE_STATUS_OFFLINE &&
            usb_storage_devices[i].usb_dev == NULL) {
            return &usb_storage_devices[i];
        }
    }
    return NULL;
}

/**
 * Send a Command Block Wrapper (CBW) and receive Command Status Wrapper (CSW)
 */
static int usb_storage_bot_transfer(usb_storage_device_t* dev, 
                                     uint8_t* cmd, uint8_t cmd_len,
                                     uint8_t* data, uint32_t data_len,
                                     uint8_t direction) {
    if (!dev || !dev->usb_dev || !cmd || cmd_len > 16) {
        return -1;
    }
    
    /* Build Command Block Wrapper */
    usb_msc_cbw_t cbw;
    usb_storage_memset(&cbw, 0, sizeof(cbw));
    cbw.dCBWSignature = USB_MSC_CBW_SIGNATURE;
    cbw.dCBWTag = dev->tag++;
    cbw.dCBWDataTransferLength = data_len;
    cbw.bmCBWFlags = direction;  /* 0x80 for IN, 0x00 for OUT */
    cbw.bCBWLUN = 0;  /* LUN 0 */
    cbw.bCBWCBLength = cmd_len;
    usb_storage_memcpy(cbw.CBWCB, cmd, cmd_len);
    
    /* Send CBW via bulk OUT endpoint */
    int result = usb_bulk_transfer(dev->usb_dev, dev->bulk_out_ep,
                                   (uint8_t*)&cbw, sizeof(cbw));
    if (result != 0) {
        dev->errors++;
        return -1;
    }
    
    /* Data phase (if needed) */
    if (data_len > 0 && data != NULL) {
        uint8_t ep = (direction == 0x80) ? dev->bulk_in_ep : dev->bulk_out_ep;
        result = usb_bulk_transfer(dev->usb_dev, ep, data, data_len);
        if (result != 0) {
            dev->errors++;
            return -1;
        }
    }
    
    /* Receive CSW via bulk IN endpoint */
    usb_msc_csw_t csw;
    usb_storage_memset(&csw, 0, sizeof(csw));
    result = usb_bulk_transfer(dev->usb_dev, dev->bulk_in_ep,
                               (uint8_t*)&csw, sizeof(csw));
    if (result != 0) {
        dev->errors++;
        return -1;
    }
    
    /* Validate CSW */
    if (csw.dCSWSignature != USB_MSC_CSW_SIGNATURE) {
        dev->errors++;
        return -1;
    }
    
    if (csw.dCSWTag != cbw.dCBWTag) {
        dev->errors++;
        return -1;
    }
    
    if (csw.bCSWStatus != USB_MSC_CSW_STATUS_PASSED) {
        dev->errors++;
        return (csw.bCSWStatus == USB_MSC_CSW_STATUS_FAILED) ? -2 : -3;
    }
    
    return 0;
}

/**
 * Parse endpoint descriptors to find bulk IN and OUT endpoints
 */
static int usb_storage_parse_endpoints(usb_storage_device_t* dev, 
                                        uint8_t* config_data, uint32_t config_len) {
    if (!dev || !config_data || config_len < 9) {
        return -1;
    }
    
    /* Walk through descriptors looking for endpoints */
    uint32_t offset = 0;
    while (offset < config_len) {
        uint8_t desc_len = config_data[offset];
        uint8_t desc_type = config_data[offset + 1];
        
        if (desc_len == 0) {
            break;  /* Invalid descriptor length */
        }
        
        /* Check for endpoint descriptor (type 5) */
        if (desc_type == USB_DESC_ENDPOINT && desc_len >= 7) {
            uint8_t ep_addr = config_data[offset + 2];
            uint8_t ep_attr = config_data[offset + 3];
            uint16_t ep_max_packet = config_data[offset + 4] | 
                                     (config_data[offset + 5] << 8);
            
            /* Check for bulk endpoint (attr bits 0-1 = 2) */
            if ((ep_attr & 0x03) == 0x02) {
                if (ep_addr & 0x80) {
                    /* Bulk IN endpoint */
                    dev->bulk_in_ep = ep_addr;
                    dev->bulk_max_packet = ep_max_packet;
                } else {
                    /* Bulk OUT endpoint */
                    dev->bulk_out_ep = ep_addr;
                }
            }
        }
        
        offset += desc_len;
    }
    
    /* Verify we found both endpoints */
    if (dev->bulk_in_ep == 0 || dev->bulk_out_ep == 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Test Unit Ready - Check if device is ready
 */
int usb_storage_test_unit_ready(usb_storage_device_t* dev) {
    if (!dev || !dev->usb_dev) {
        return -1;
    }
    
    uint8_t cmd[6] = {SCSI_CMD_TEST_UNIT_READY, 0, 0, 0, 0, 0};
    return usb_storage_bot_transfer(dev, cmd, 6, NULL, 0, 0x00);
}

/**
 * SCSI Inquiry - Get device identification
 */
int usb_storage_inquiry(usb_storage_device_t* dev, scsi_inquiry_response_t* response) {
    if (!dev || !dev->usb_dev || !response) {
        return -1;
    }
    
    uint8_t cmd[6] = {
        SCSI_CMD_INQUIRY,
        0,                                      /* LUN (in old format) */
        0,                                      /* Page code */
        0,                                      /* Reserved */
        sizeof(scsi_inquiry_response_t),        /* Allocation length */
        0                                       /* Control */
    };
    
    usb_storage_memset(response, 0, sizeof(*response));
    
    int result = usb_storage_bot_transfer(dev, cmd, 6, 
                                          (uint8_t*)response, sizeof(*response),
                                          0x80);  /* IN */
    if (result == 0) {
        /* Copy identification strings to device structure */
        usb_storage_memcpy(dev->vendor, response->vendor_id, 8);
        dev->vendor[8] = '\0';
        usb_storage_memcpy(dev->product, response->product_id, 16);
        dev->product[16] = '\0';
        usb_storage_memcpy(dev->revision, response->revision, 4);
        dev->revision[4] = '\0';
    }
    
    return result;
}

/**
 * Read Capacity - Get storage capacity
 */
int usb_storage_read_capacity(usb_storage_device_t* dev) {
    if (!dev || !dev->usb_dev) {
        return -1;
    }
    
    uint8_t cmd[10] = {
        SCSI_CMD_READ_CAPACITY_10,
        0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    
    scsi_read_capacity_10_response_t response;
    usb_storage_memset(&response, 0, sizeof(response));
    
    int result = usb_storage_bot_transfer(dev, cmd, 10,
                                          (uint8_t*)&response, sizeof(response),
                                          0x80);  /* IN */
    
    if (result == 0) {
        /* Convert from big-endian */
        uint32_t last_lba = be32_to_cpu(response.last_lba);
        uint32_t block_size = be32_to_cpu(response.block_size);
        
        dev->total_blocks = (uint64_t)last_lba + 1;
        dev->block_size = block_size;
        dev->capacity_bytes = dev->total_blocks * dev->block_size;
    }
    
    return result;
}

/**
 * Request Sense - Get error information
 */
int usb_storage_request_sense(usb_storage_device_t* dev, uint8_t* sense_data, uint8_t length) {
    if (!dev || !dev->usb_dev || !sense_data || length < 18) {
        return -1;
    }
    
    uint8_t cmd[6] = {
        SCSI_CMD_REQUEST_SENSE,
        0,
        0,
        0,
        length,
        0
    };
    
    return usb_storage_bot_transfer(dev, cmd, 6, sense_data, length, 0x80);
}

/**
 * Read blocks from device
 */
int usb_storage_read_blocks(usb_storage_device_t* dev, uint64_t lba, 
                            uint32_t count, uint8_t* buffer) {
    if (!dev || !dev->usb_dev || !buffer || count == 0) {
        return -1;
    }
    
    if (dev->status != USB_STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Check bounds */
    if (lba + count > dev->total_blocks) {
        return -1;
    }
    
    /* Build READ(10) command */
    uint32_t lba32 = (uint32_t)lba;  /* READ(10) supports 32-bit LBA */
    uint8_t cmd[10] = {
        SCSI_CMD_READ_10,
        0,
        (uint8_t)((lba32 >> 24) & 0xFF),
        (uint8_t)((lba32 >> 16) & 0xFF),
        (uint8_t)((lba32 >> 8) & 0xFF),
        (uint8_t)(lba32 & 0xFF),
        0,  /* Reserved/Group */
        (uint8_t)((count >> 8) & 0xFF),
        (uint8_t)(count & 0xFF),
        0   /* Control */
    };
    
    uint32_t transfer_len = count * dev->block_size;
    int result = usb_storage_bot_transfer(dev, cmd, 10, buffer, transfer_len, 0x80);
    
    if (result == 0) {
        dev->reads += count;
    } else {
        dev->errors++;
    }
    
    return result;
}

/**
 * Write blocks to device
 */
int usb_storage_write_blocks(usb_storage_device_t* dev, uint64_t lba,
                             uint32_t count, const uint8_t* buffer) {
    if (!dev || !dev->usb_dev || !buffer || count == 0) {
        return -1;
    }
    
    if (dev->status != USB_STORAGE_STATUS_ONLINE) {
        return -1;
    }
    
    /* Check bounds */
    if (lba + count > dev->total_blocks) {
        return -1;
    }
    
    /* Build WRITE(10) command */
    uint32_t lba32 = (uint32_t)lba;  /* WRITE(10) supports 32-bit LBA */
    uint8_t cmd[10] = {
        SCSI_CMD_WRITE_10,
        0,
        (uint8_t)((lba32 >> 24) & 0xFF),
        (uint8_t)((lba32 >> 16) & 0xFF),
        (uint8_t)((lba32 >> 8) & 0xFF),
        (uint8_t)(lba32 & 0xFF),
        0,  /* Reserved/Group */
        (uint8_t)((count >> 8) & 0xFF),
        (uint8_t)(count & 0xFF),
        0   /* Control */
    };
    
    uint32_t transfer_len = count * dev->block_size;
    /* Note: We cast away const for the transfer, but data direction is OUT */
    int result = usb_storage_bot_transfer(dev, cmd, 10, (uint8_t*)buffer, transfer_len, 0x00);
    
    if (result == 0) {
        dev->writes += count;
    } else {
        dev->errors++;
    }
    
    return result;
}

/**
 * Attach a USB storage device
 */
usb_storage_device_t* usb_storage_attach(usb_device_t* usb_dev) {
    if (!usb_dev || !usb_storage_initialized) {
        return NULL;
    }
    
    /* Verify device class */
    if (usb_dev->descriptor.bDeviceClass != USB_CLASS_MASS_STORAGE &&
        usb_dev->descriptor.bDeviceClass != 0) {
        /* Class 0 means class is interface-specific */
        /* Would need to check interface descriptor */
        return NULL;
    }
    
    /* Allocate device slot */
    usb_storage_device_t* dev = alloc_usb_storage_device();
    if (!dev) {
        return NULL;
    }
    
    /* Initialize device */
    dev->usb_dev = usb_dev;
    dev->status = USB_STORAGE_STATUS_BUSY;
    dev->lun_count = 1;
    dev->tag = 1;
    
    /* Get configuration descriptor to find endpoints */
    uint8_t config_buf[64];
    int result = usb_get_config_descriptor(usb_dev, 0, config_buf, sizeof(config_buf));
    if (result != 0) {
        dev->usb_dev = NULL;
        dev->status = USB_STORAGE_STATUS_OFFLINE;
        return NULL;
    }
    
    /* Parse endpoints from configuration descriptor */
    if (usb_storage_parse_endpoints(dev, config_buf, sizeof(config_buf)) != 0) {
        /* Use default endpoint addresses if parsing fails */
        dev->bulk_in_ep = 0x81;   /* Default bulk IN */
        dev->bulk_out_ep = 0x02;  /* Default bulk OUT */
    }
    
    /* Test if unit is ready */
    for (int retry = 0; retry < 3; retry++) {
        result = usb_storage_test_unit_ready(dev);
        if (result == 0) {
            break;
        }
        /* Small delay between retries */
        for (volatile int i = 0; i < 10000; i++);
    }
    
    /* Get device identification */
    scsi_inquiry_response_t inquiry;
    result = usb_storage_inquiry(dev, &inquiry);
    if (result != 0) {
        /* Set default identification if inquiry fails */
        usb_storage_memcpy(dev->vendor, "Unknown", 8);
        usb_storage_memcpy(dev->product, "USB Storage", 12);
        usb_storage_memcpy(dev->revision, "1.0", 4);
    }
    
    /* Get storage capacity */
    result = usb_storage_read_capacity(dev);
    if (result != 0) {
        dev->total_blocks = 0;
        dev->block_size = 512;
        dev->capacity_bytes = 0;
    }
    
    /* Mark device as online */
    dev->status = USB_STORAGE_STATUS_ONLINE;
    usb_storage_device_count++;
    
    /* Store driver data pointer in USB device */
    usb_dev->driver_data = dev;
    
    return dev;
}

/**
 * Detach a USB storage device
 */
int usb_storage_detach(usb_storage_device_t* dev) {
    if (!dev || !usb_storage_initialized) {
        return -1;
    }
    
    if (dev->status == USB_STORAGE_STATUS_OFFLINE) {
        return -1;  /* Already detached */
    }
    
    /* Clear driver data from USB device */
    if (dev->usb_dev) {
        dev->usb_dev->driver_data = NULL;
    }
    
    /* Reset device structure */
    dev->usb_dev = NULL;
    dev->status = USB_STORAGE_STATUS_OFFLINE;
    dev->bulk_in_ep = 0;
    dev->bulk_out_ep = 0;
    dev->total_blocks = 0;
    dev->capacity_bytes = 0;
    
    if (usb_storage_device_count > 0) {
        usb_storage_device_count--;
    }
    
    return 0;
}

/**
 * Get USB storage device by index
 */
usb_storage_device_t* usb_storage_get_device(uint8_t index) {
    if (index >= MAX_USB_STORAGE_DEVICES) {
        return NULL;
    }
    
    if (usb_storage_devices[index].status == USB_STORAGE_STATUS_OFFLINE) {
        return NULL;
    }
    
    return &usb_storage_devices[index];
}

/**
 * Get count of attached USB storage devices
 */
int usb_storage_get_device_count(void) {
    return usb_storage_device_count;
}

/**
 * Get status string
 */
const char* usb_storage_get_status_string(usb_storage_status_t status) {
    switch (status) {
        case USB_STORAGE_STATUS_ONLINE:
            return "Online";
        case USB_STORAGE_STATUS_OFFLINE:
            return "Offline";
        case USB_STORAGE_STATUS_ERROR:
            return "Error";
        case USB_STORAGE_STATUS_BUSY:
            return "Busy";
        default:
            return "Unknown";
    }
}

/**
 * Get capacity in megabytes
 */
uint64_t usb_storage_get_capacity_mb(usb_storage_device_t* dev) {
    if (!dev) {
        return 0;
    }
    /* Divide by 1MB (2^20) using shift */
    return dev->capacity_bytes >> 20;
}

/**
 * Get capacity in gigabytes
 */
uint64_t usb_storage_get_capacity_gb(usb_storage_device_t* dev) {
    if (!dev) {
        return 0;
    }
    /* Divide by 1GB (2^30) using shift */
    return dev->capacity_bytes >> 30;
}
