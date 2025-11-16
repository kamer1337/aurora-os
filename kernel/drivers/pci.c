/**
 * Aurora OS - PCI Bus Enumeration Implementation
 */

#include "pci.h"
#include "../core/port_io.h"
#include <stddef.h>

// PCI Configuration Address Port
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

#define MAX_PCI_DEVICES 64

// Detected PCI devices
static pci_device_t pci_devices[MAX_PCI_DEVICES];
static int pci_device_count = 0;

uint32_t pci_config_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = (1 << 31) |  // Enable bit
                      ((uint32_t)bus << 16) |
                      ((uint32_t)device << 11) |
                      ((uint32_t)function << 8) |
                      (offset & 0xFC);  // Align to 4-byte boundary
    
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = (1 << 31) |
                      ((uint32_t)bus << 16) |
                      ((uint32_t)device << 11) |
                      ((uint32_t)function << 8) |
                      (offset & 0xFC);
    
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

static uint16_t pci_read_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t value = pci_config_read(bus, device, function, PCI_CONFIG_VENDOR_ID);
    return (uint16_t)(value & 0xFFFF);
}

static uint16_t pci_read_device_id(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t value = pci_config_read(bus, device, function, PCI_CONFIG_DEVICE_ID);
    return (uint16_t)(value & 0xFFFF);
}

void pci_init(void) {
    pci_device_count = 0;
}

int pci_enumerate_devices(void) {
    pci_device_count = 0;
    
    // Scan all buses, devices, and functions
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint16_t vendor_id = pci_read_vendor_id(bus, device, function);
                
                // 0xFFFF means no device
                if (vendor_id == 0xFFFF) {
                    // If function 0 doesn't exist, no need to check other functions
                    if (function == 0) break;
                    continue;
                }
                
                // Found a device
                if (pci_device_count < MAX_PCI_DEVICES) {
                    pci_device_t* dev = &pci_devices[pci_device_count++];
                    
                    dev->bus = bus;
                    dev->device = device;
                    dev->function = function;
                    dev->vendor_id = vendor_id;
                    dev->device_id = pci_read_device_id(bus, device, function);
                    
                    // Read class codes
                    uint32_t class_info = pci_config_read(bus, device, function, PCI_CONFIG_REVISION_ID);
                    dev->revision_id = (uint8_t)(class_info & 0xFF);
                    dev->prog_if = (uint8_t)((class_info >> 8) & 0xFF);
                    dev->subclass = (uint8_t)((class_info >> 16) & 0xFF);
                    dev->class_code = (uint8_t)((class_info >> 24) & 0xFF);
                    
                    // Read BARs
                    for (int i = 0; i < 6; i++) {
                        dev->bar[i] = pci_config_read(bus, device, function, PCI_CONFIG_BAR0 + (i * 4));
                    }
                    
                    // Read interrupt line
                    uint32_t int_info = pci_config_read(bus, device, function, PCI_CONFIG_INTERRUPT_LINE);
                    dev->interrupt_line = (uint8_t)(int_info & 0xFF);
                }
                
                // If this is a single-function device, no need to check other functions
                if (function == 0) {
                    uint32_t header_type = pci_config_read(bus, device, function, PCI_CONFIG_HEADER_TYPE);
                    if ((header_type & 0x80) == 0) {
                        break;  // Not a multi-function device
                    }
                }
            }
        }
    }
    
    return pci_device_count;
}

int pci_get_device_count(void) {
    return pci_device_count;
}

pci_device_t* pci_get_device(int index) {
    if (index < 0 || index >= pci_device_count) {
        return NULL;
    }
    return &pci_devices[index];
}

int pci_find_devices_by_class(uint8_t class_code, pci_device_t* out_devices, int max_devices) {
    int found = 0;
    
    for (int i = 0; i < pci_device_count && found < max_devices; i++) {
        if (pci_devices[i].class_code == class_code) {
            out_devices[found++] = pci_devices[i];
        }
    }
    
    return found;
}

int pci_find_devices_by_id(uint16_t vendor_id, uint16_t device_id,
                            pci_device_t* out_devices, int max_devices) {
    int found = 0;
    
    for (int i = 0; i < pci_device_count && found < max_devices; i++) {
        if (pci_devices[i].vendor_id == vendor_id && 
            pci_devices[i].device_id == device_id) {
            out_devices[found++] = pci_devices[i];
        }
    }
    
    return found;
}

const char* pci_get_class_name(uint8_t class_code) {
    switch (class_code) {
        case PCI_CLASS_STORAGE: return "Storage Controller";
        case PCI_CLASS_NETWORK: return "Network Controller";
        case PCI_CLASS_DISPLAY: return "Display Controller";
        case PCI_CLASS_MULTIMEDIA: return "Multimedia Device";
        case PCI_CLASS_MEMORY: return "Memory Controller";
        case PCI_CLASS_BRIDGE: return "Bridge Device";
        case PCI_CLASS_SERIAL_BUS: return "Serial Bus Controller";
        default: return "Unknown Device";
    }
}

const char* pci_get_subclass_name(uint8_t class_code, uint8_t subclass) {
    if (class_code == PCI_CLASS_STORAGE) {
        switch (subclass) {
            case PCI_SUBCLASS_STORAGE_SCSI: return "SCSI Controller";
            case PCI_SUBCLASS_STORAGE_IDE: return "IDE Controller";
            case PCI_SUBCLASS_STORAGE_FLOPPY: return "Floppy Controller";
            case PCI_SUBCLASS_STORAGE_RAID: return "RAID Controller";
            case PCI_SUBCLASS_STORAGE_ATA: return "ATA Controller";
            case PCI_SUBCLASS_STORAGE_SATA: return "SATA Controller";
            case PCI_SUBCLASS_STORAGE_SAS: return "SAS Controller";
            case PCI_SUBCLASS_STORAGE_NVME: return "NVMe Controller";
            default: return "Storage Controller";
        }
    }
    
    return "Unknown";
}
