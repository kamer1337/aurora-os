/**
 * Aurora OS - PCI Bus Enumeration
 * 
 * Provides PCI bus enumeration and device detection for hardware support
 */

#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI Configuration Space Registers
#define PCI_CONFIG_VENDOR_ID    0x00
#define PCI_CONFIG_DEVICE_ID    0x02
#define PCI_CONFIG_COMMAND      0x04
#define PCI_CONFIG_STATUS       0x06
#define PCI_CONFIG_REVISION_ID  0x08
#define PCI_CONFIG_PROG_IF      0x09
#define PCI_CONFIG_SUBCLASS     0x0A
#define PCI_CONFIG_CLASS_CODE   0x0B
#define PCI_CONFIG_HEADER_TYPE  0x0E
#define PCI_CONFIG_BAR0         0x10
#define PCI_CONFIG_BAR1         0x14
#define PCI_CONFIG_BAR2         0x18
#define PCI_CONFIG_BAR3         0x1C
#define PCI_CONFIG_BAR4         0x20
#define PCI_CONFIG_BAR5         0x24
#define PCI_CONFIG_INTERRUPT_LINE 0x3C

// PCI Device Classes
#define PCI_CLASS_STORAGE       0x01
#define PCI_CLASS_NETWORK       0x02
#define PCI_CLASS_DISPLAY       0x03
#define PCI_CLASS_MULTIMEDIA    0x04
#define PCI_CLASS_MEMORY        0x05
#define PCI_CLASS_BRIDGE        0x06
#define PCI_CLASS_SERIAL_BUS    0x0C

// PCI Storage Subclasses
#define PCI_SUBCLASS_STORAGE_SCSI       0x00
#define PCI_SUBCLASS_STORAGE_IDE        0x01
#define PCI_SUBCLASS_STORAGE_FLOPPY     0x02
#define PCI_SUBCLASS_STORAGE_RAID       0x04
#define PCI_SUBCLASS_STORAGE_ATA        0x05
#define PCI_SUBCLASS_STORAGE_SATA       0x06
#define PCI_SUBCLASS_STORAGE_SAS        0x07
#define PCI_SUBCLASS_STORAGE_NVME       0x08

// PCI Device structure
typedef struct {
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t revision_id;
    uint32_t bar[6];
    uint8_t interrupt_line;
} pci_device_t;

/**
 * Initialize PCI bus enumeration
 */
void pci_init(void);

/**
 * Enumerate all PCI devices on the system
 * @return Number of devices found
 */
int pci_enumerate_devices(void);

/**
 * Get number of detected PCI devices
 * @return Number of devices
 */
int pci_get_device_count(void);

/**
 * Get PCI device by index
 * @param index Device index
 * @return Pointer to device or NULL if not found
 */
pci_device_t* pci_get_device(int index);

/**
 * Find PCI devices by class code
 * @param class_code Class code to search for
 * @param out_devices Output array for devices
 * @param max_devices Maximum number of devices to return
 * @return Number of devices found
 */
int pci_find_devices_by_class(uint8_t class_code, pci_device_t* out_devices, int max_devices);

/**
 * Find PCI devices by vendor and device ID
 * @param vendor_id Vendor ID
 * @param device_id Device ID
 * @param out_devices Output array for devices
 * @param max_devices Maximum number of devices to return
 * @return Number of devices found
 */
int pci_find_devices_by_id(uint16_t vendor_id, uint16_t device_id, 
                            pci_device_t* out_devices, int max_devices);

/**
 * Read configuration space register
 * @param bus PCI bus number
 * @param device PCI device number
 * @param function PCI function number
 * @param offset Register offset
 * @return Register value
 */
uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * Write configuration space register
 * @param bus PCI bus number
 * @param device PCI device number
 * @param function PCI function number
 * @param offset Register offset
 * @param value Value to write
 */
void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

/**
 * Get device class name
 * @param class_code Class code
 * @return Class name string
 */
const char* pci_get_class_name(uint8_t class_code);

/**
 * Get device subclass name
 * @param class_code Class code
 * @param subclass Subclass code
 * @return Subclass name string
 */
const char* pci_get_subclass_name(uint8_t class_code, uint8_t subclass);

#endif // PCI_H
