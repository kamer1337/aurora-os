/**
 * Aurora OS - SATA AHCI Controller Driver
 * 
 * Advanced Host Controller Interface (AHCI) driver for SATA storage devices
 */

#ifndef AHCI_H
#define AHCI_H

#include <stdint.h>
#include "pci.h"

// AHCI HBA Memory Registers
#define AHCI_HBA_CAP        0x00  // Host Capabilities
#define AHCI_HBA_GHC        0x04  // Global Host Control
#define AHCI_HBA_IS         0x08  // Interrupt Status
#define AHCI_HBA_PI         0x0C  // Ports Implemented
#define AHCI_HBA_VS         0x10  // Version

// AHCI Port Registers (offset from port base)
#define AHCI_PORT_CLB       0x00  // Command List Base Address
#define AHCI_PORT_CLBU      0x04  // Command List Base Address Upper
#define AHCI_PORT_FB        0x08  // FIS Base Address
#define AHCI_PORT_FBU       0x0C  // FIS Base Address Upper
#define AHCI_PORT_IS        0x10  // Interrupt Status
#define AHCI_PORT_IE        0x14  // Interrupt Enable
#define AHCI_PORT_CMD       0x18  // Command and Status
#define AHCI_PORT_TFD       0x20  // Task File Data
#define AHCI_PORT_SIG       0x24  // Signature
#define AHCI_PORT_SSTS      0x28  // SATA Status
#define AHCI_PORT_SCTL      0x2C  // SATA Control
#define AHCI_PORT_SERR      0x30  // SATA Error
#define AHCI_PORT_SACT      0x34  // SATA Active
#define AHCI_PORT_CI        0x38  // Command Issue

// AHCI Port Command Register bits
#define AHCI_PORT_CMD_ST    (1 << 0)   // Start
#define AHCI_PORT_CMD_SUD   (1 << 1)   // Spin-Up Device
#define AHCI_PORT_CMD_POD   (1 << 2)   // Power On Device
#define AHCI_PORT_CMD_FRE   (1 << 4)   // FIS Receive Enable
#define AHCI_PORT_CMD_FR    (1 << 14)  // FIS Receive Running
#define AHCI_PORT_CMD_CR    (1 << 15)  // Command List Running

// AHCI FIS Types
#define FIS_TYPE_REG_H2D    0x27  // Register FIS - Host to Device
#define FIS_TYPE_REG_D2H    0x34  // Register FIS - Device to Host
#define FIS_TYPE_DMA_ACT    0x39  // DMA Activate FIS
#define FIS_TYPE_DMA_SETUP  0x41  // DMA Setup FIS
#define FIS_TYPE_DATA       0x46  // Data FIS
#define FIS_TYPE_BIST       0x58  // BIST Activate FIS
#define FIS_TYPE_PIO_SETUP  0x5F  // PIO Setup FIS
#define FIS_TYPE_DEV_BITS   0xA1  // Set Device Bits FIS

// Device signatures
#define AHCI_SIG_ATA        0x00000101  // SATA drive
#define AHCI_SIG_ATAPI      0xEB140101  // ATAPI drive
#define AHCI_SIG_SEMB       0xC33C0101  // Enclosure management bridge
#define AHCI_SIG_PM         0x96690101  // Port multiplier

// AHCI device types
typedef enum {
    AHCI_DEV_NULL = 0,
    AHCI_DEV_SATA = 1,
    AHCI_DEV_SATAPI = 2,
    AHCI_DEV_SEMB = 3,
    AHCI_DEV_PM = 4
} ahci_device_type_t;

// AHCI port structure
typedef struct {
    uint8_t port_num;
    ahci_device_type_t type;
    uint32_t* clb;      // Command list base (virtual address)
    uint32_t* fb;       // FIS base (virtual address)
    uint32_t clb_phys;  // Command list base (physical address)
    uint32_t fb_phys;   // FIS base (physical address)
    volatile uint32_t* regs;  // Port registers
} ahci_port_t;

// AHCI controller structure
typedef struct {
    pci_device_t pci_device;
    volatile uint32_t* abar;  // AHCI Base Address Register (memory-mapped)
    uint32_t ports_impl;      // Ports implemented bitmap
    ahci_port_t ports[32];
    int port_count;
} ahci_controller_t;

/**
 * Initialize AHCI subsystem
 * @return 0 on success, -1 on failure
 */
int ahci_init(void);

/**
 * Detect and initialize AHCI controllers
 * @return Number of controllers found
 */
int ahci_detect_controllers(void);

/**
 * Get AHCI controller by index
 * @param index Controller index
 * @return Pointer to controller or NULL
 */
ahci_controller_t* ahci_get_controller(int index);

/**
 * Initialize an AHCI port
 * @param port Port to initialize
 * @return 0 on success, -1 on failure
 */
int ahci_port_init(ahci_port_t* port);

/**
 * Start command processing on a port
 * @param port Port to start
 * @return 0 on success, -1 on failure
 */
int ahci_port_start(ahci_port_t* port);

/**
 * Stop command processing on a port
 * @param port Port to stop
 * @return 0 on success, -1 on failure
 */
int ahci_port_stop(ahci_port_t* port);

/**
 * Read sectors from AHCI device
 * @param port Port to read from
 * @param start_lba Starting LBA
 * @param count Number of sectors to read
 * @param buffer Buffer to read into
 * @return 0 on success, -1 on failure
 */
int ahci_read_sectors(ahci_port_t* port, uint64_t start_lba, uint32_t count, void* buffer);

/**
 * Write sectors to AHCI device
 * @param port Port to write to
 * @param start_lba Starting LBA
 * @param count Number of sectors to write
 * @param buffer Buffer to write from
 * @return 0 on success, -1 on failure
 */
int ahci_write_sectors(ahci_port_t* port, uint64_t start_lba, uint32_t count, const void* buffer);

/**
 * Identify AHCI device
 * @param port Port to identify
 * @param buffer Buffer to store identify data
 * @return 0 on success, -1 on failure
 */
int ahci_identify(ahci_port_t* port, void* buffer);

/**
 * Get device type name
 * @param type Device type
 * @return Type name string
 */
const char* ahci_get_device_type_name(ahci_device_type_t type);

#endif // AHCI_H
