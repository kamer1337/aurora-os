/**
 * Aurora OS - USB Mass Storage Driver Header
 * 
 * USB Mass Storage Class (MSC) device support using
 * Bulk-Only Transport (BOT) protocol
 */

#ifndef AURORA_USB_STORAGE_H
#define AURORA_USB_STORAGE_H

#include <stdint.h>
#include "usb.h"

/* USB Mass Storage Class codes */
#define USB_MSC_SUBCLASS_SCSI       0x06    /* SCSI transparent command set */
#define USB_MSC_PROTOCOL_BOT        0x50    /* Bulk-Only Transport */

/* USB Mass Storage Bulk-Only Transport (BOT) */
#define USB_MSC_CBW_SIGNATURE       0x43425355  /* "USBC" */
#define USB_MSC_CSW_SIGNATURE       0x53425355  /* "USBS" */

/* Command Status Wrapper (CSW) status codes */
#define USB_MSC_CSW_STATUS_PASSED   0x00
#define USB_MSC_CSW_STATUS_FAILED   0x01
#define USB_MSC_CSW_STATUS_PHASE    0x02

/* SCSI commands */
#define SCSI_CMD_TEST_UNIT_READY    0x00
#define SCSI_CMD_REQUEST_SENSE      0x03
#define SCSI_CMD_INQUIRY            0x12
#define SCSI_CMD_READ_CAPACITY_10   0x25
#define SCSI_CMD_READ_10            0x28
#define SCSI_CMD_WRITE_10           0x2A
#define SCSI_CMD_MODE_SENSE_6       0x1A
#define SCSI_CMD_PREVENT_REMOVAL    0x1E
#define SCSI_CMD_START_STOP_UNIT    0x1B

/* USB storage device status */
typedef enum {
    USB_STORAGE_STATUS_OFFLINE,
    USB_STORAGE_STATUS_ONLINE,
    USB_STORAGE_STATUS_ERROR,
    USB_STORAGE_STATUS_BUSY
} usb_storage_status_t;

/* Command Block Wrapper (CBW) - 31 bytes */
typedef struct {
    uint32_t dCBWSignature;         /* Must be USB_MSC_CBW_SIGNATURE */
    uint32_t dCBWTag;               /* Command block tag */
    uint32_t dCBWDataTransferLength;/* Expected data transfer length */
    uint8_t  bmCBWFlags;            /* Bit 7: 0=OUT, 1=IN */
    uint8_t  bCBWLUN;               /* Logical unit number */
    uint8_t  bCBWCBLength;          /* Length of command block (1-16) */
    uint8_t  CBWCB[16];             /* Command block (SCSI command) */
} __attribute__((packed)) usb_msc_cbw_t;

/* Command Status Wrapper (CSW) - 13 bytes */
typedef struct {
    uint32_t dCSWSignature;         /* Must be USB_MSC_CSW_SIGNATURE */
    uint32_t dCSWTag;               /* Must match dCBWTag */
    uint32_t dCSWDataResidue;       /* Difference in expected vs actual data */
    uint8_t  bCSWStatus;            /* Status: 0=passed, 1=failed, 2=phase error */
} __attribute__((packed)) usb_msc_csw_t;

/* SCSI Inquiry response - 36 bytes minimum */
typedef struct {
    uint8_t  peripheral_type;       /* Device type and qualifier */
    uint8_t  removable;             /* Removable media bit */
    uint8_t  version;               /* SCSI version */
    uint8_t  response_format;       /* Response data format */
    uint8_t  additional_length;     /* Additional data length */
    uint8_t  reserved[3];           /* Reserved bytes */
    uint8_t  vendor_id[8];          /* T10 vendor identification */
    uint8_t  product_id[16];        /* Product identification */
    uint8_t  revision[4];           /* Product revision level */
} __attribute__((packed)) scsi_inquiry_response_t;

/* SCSI Read Capacity (10) response */
typedef struct {
    uint32_t last_lba;              /* Last logical block address (big-endian) */
    uint32_t block_size;            /* Block size in bytes (big-endian) */
} __attribute__((packed)) scsi_read_capacity_10_response_t;

/* USB storage device structure */
typedef struct usb_storage_device {
    usb_device_t* usb_dev;          /* Underlying USB device */
    
    /* Device info */
    usb_storage_status_t status;
    uint8_t lun_count;              /* Number of logical units */
    uint8_t bulk_in_ep;             /* Bulk IN endpoint address */
    uint8_t bulk_out_ep;            /* Bulk OUT endpoint address */
    uint16_t bulk_max_packet;       /* Max packet size for bulk transfers */
    
    /* Storage info */
    uint64_t total_blocks;          /* Total number of blocks */
    uint32_t block_size;            /* Block size in bytes */
    uint64_t capacity_bytes;        /* Total capacity */
    
    /* Device identification */
    char vendor[9];                 /* Vendor name (8 chars + null) */
    char product[17];               /* Product name (16 chars + null) */
    char revision[5];               /* Revision (4 chars + null) */
    
    /* Transfer state */
    uint32_t tag;                   /* Current CBW tag */
    
    /* Statistics */
    uint64_t reads;
    uint64_t writes;
    uint64_t errors;
    
    /* Link to next device */
    struct usb_storage_device* next;
} usb_storage_device_t;

/* Initialize USB mass storage subsystem */
void usb_storage_init(void);

/* Device management */
usb_storage_device_t* usb_storage_attach(usb_device_t* usb_dev);
int usb_storage_detach(usb_storage_device_t* dev);
usb_storage_device_t* usb_storage_get_device(uint8_t index);
int usb_storage_get_device_count(void);

/* SCSI commands */
int usb_storage_test_unit_ready(usb_storage_device_t* dev);
int usb_storage_inquiry(usb_storage_device_t* dev, scsi_inquiry_response_t* response);
int usb_storage_read_capacity(usb_storage_device_t* dev);
int usb_storage_request_sense(usb_storage_device_t* dev, uint8_t* sense_data, uint8_t length);

/* Block I/O operations */
int usb_storage_read_blocks(usb_storage_device_t* dev, uint64_t lba, uint32_t count, uint8_t* buffer);
int usb_storage_write_blocks(usb_storage_device_t* dev, uint64_t lba, uint32_t count, const uint8_t* buffer);

/* Utility functions */
const char* usb_storage_get_status_string(usb_storage_status_t status);
uint64_t usb_storage_get_capacity_mb(usb_storage_device_t* dev);
uint64_t usb_storage_get_capacity_gb(usb_storage_device_t* dev);

#endif /* AURORA_USB_STORAGE_H */
