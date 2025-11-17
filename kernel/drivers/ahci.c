/**
 * Aurora OS - SATA AHCI Controller Driver Implementation
 */

#include "ahci.h"
#include "pci.h"
#include "../memory/memory.h"
#include <stddef.h>

#define MAX_AHCI_CONTROLLERS 4

// ATA commands
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_IDENTIFY        0xEC

// AHCI Command Table structures
typedef struct {
    uint8_t fis_type;       // FIS_TYPE_REG_H2D
    uint8_t pm_port:4;      // Port multiplier
    uint8_t rsv0:3;         // Reserved
    uint8_t c:1;            // 1: Command, 0: Control
    uint8_t command;        // Command register
    uint8_t featurel;       // Feature register low
    
    uint8_t lba0;           // LBA low
    uint8_t lba1;           // LBA mid
    uint8_t lba2;           // LBA high
    uint8_t device;         // Device register
    
    uint8_t lba3;           // LBA (continued)
    uint8_t lba4;
    uint8_t lba5;
    uint8_t featureh;       // Feature register high
    
    uint8_t countl;         // Count low
    uint8_t counth;         // Count high
    uint8_t icc;            // Isochronous command completion
    uint8_t control;        // Control register
    
    uint8_t rsv1[4];        // Reserved
} __attribute__((packed)) fis_reg_h2d_t;

// Physical Region Descriptor Table entry
typedef struct {
    uint32_t dba;           // Data base address
    uint32_t dbau;          // Data base address upper 32 bits
    uint32_t rsv0;          // Reserved
    uint32_t dbc:22;        // Byte count, 4M max
    uint32_t rsv1:9;        // Reserved
    uint32_t i:1;           // Interrupt on completion
} __attribute__((packed)) ahci_prdt_entry_t;

// Command table
typedef struct {
    uint8_t cfis[64];       // Command FIS
    uint8_t acmd[16];       // ATAPI command
    uint8_t rsv[48];        // Reserved
    ahci_prdt_entry_t prdt_entry[1]; // Physical region descriptor table entries
} __attribute__((packed)) ahci_cmd_tbl_t;

// Command header
typedef struct {
    uint8_t cfl:5;          // Command FIS length in DWORDS
    uint8_t a:1;            // ATAPI
    uint8_t w:1;            // Write, 1: H2D, 0: D2H
    uint8_t p:1;            // Prefetchable
    
    uint8_t r:1;            // Reset
    uint8_t b:1;            // BIST
    uint8_t c:1;            // Clear busy upon R_OK
    uint8_t rsv0:1;         // Reserved
    uint8_t pmp:4;          // Port multiplier port
    
    uint16_t prdtl;         // Physical region descriptor table length
    uint32_t prdbc;         // Physical region descriptor byte count
    uint32_t ctba;          // Command table descriptor base address
    uint32_t ctbau;         // Command table descriptor base address upper 32 bits
    uint32_t rsv1[4];       // Reserved
} __attribute__((packed)) ahci_cmd_header_t;

// AHCI controllers
static ahci_controller_t controllers[MAX_AHCI_CONTROLLERS];
static int controller_count = 0;

int ahci_init(void) {
    controller_count = 0;
    return 0;
}

int ahci_detect_controllers(void) {
    // Find AHCI controllers via PCI
    pci_device_t pci_devices[16];
    int count = pci_find_devices_by_class(PCI_CLASS_STORAGE, pci_devices, 16);
    
    for (int i = 0; i < count && controller_count < MAX_AHCI_CONTROLLERS; i++) {
        // Check if it's an AHCI controller (SATA with AHCI prog_if)
        if (pci_devices[i].subclass == PCI_SUBCLASS_STORAGE_SATA &&
            pci_devices[i].prog_if == 0x01) {  // AHCI interface
            
            ahci_controller_t* ctrl = &controllers[controller_count];
            ctrl->pci_device = pci_devices[i];
            
            // Get AHCI Base Address Register (BAR5)
            uint32_t abar = pci_devices[i].bar[5];
            if ((abar & 0x1) == 0) {  // Memory-mapped
                ctrl->abar = (volatile uint32_t*)(abar & ~0xF);
                
                // Read ports implemented
                ctrl->ports_impl = ctrl->abar[AHCI_HBA_PI / 4];
                
                // Initialize ports
                ctrl->port_count = 0;
                for (int port = 0; port < 32; port++) {
                    if (ctrl->ports_impl & (1 << port)) {
                        ahci_port_t* p = &ctrl->ports[ctrl->port_count++];
                        p->port_num = port;
                        
                        // Port registers start at offset 0x100 + (port * 0x80)
                        p->regs = (volatile uint32_t*)((uint8_t*)ctrl->abar + 0x100 + (port * 0x80));
                        
                        // Check device type by signature
                        uint32_t sig = p->regs[AHCI_PORT_SIG / 4];
                        switch (sig) {
                            case AHCI_SIG_ATA:
                                p->type = AHCI_DEV_SATA;
                                break;
                            case AHCI_SIG_ATAPI:
                                p->type = AHCI_DEV_SATAPI;
                                break;
                            case AHCI_SIG_SEMB:
                                p->type = AHCI_DEV_SEMB;
                                break;
                            case AHCI_SIG_PM:
                                p->type = AHCI_DEV_PM;
                                break;
                            default:
                                p->type = AHCI_DEV_NULL;
                                break;
                        }
                        
                        // Initialize port if it has a device
                        if (p->type != AHCI_DEV_NULL) {
                            ahci_port_init(p);
                        }
                    }
                }
                
                controller_count++;
            }
        }
    }
    
    return controller_count;
}

ahci_controller_t* ahci_get_controller(int index) {
    if (index < 0 || index >= controller_count) {
        return NULL;
    }
    return &controllers[index];
}

int ahci_port_init(ahci_port_t* port) {
    if (!port) return -1;
    
    // Stop the port first
    ahci_port_stop(port);
    
    // Allocate command list (1KB aligned)
    port->clb = (uint32_t*)kmalloc(1024);
    if (!port->clb) return -1;
    port->clb_phys = (uint32_t)(uintptr_t)port->clb;  // In a real OS, convert to physical
    
    // Allocate FIS receive area (256 bytes aligned)
    port->fb = (uint32_t*)kmalloc(256);
    if (!port->fb) {
        kfree(port->clb);
        return -1;
    }
    port->fb_phys = (uint32_t)(uintptr_t)port->fb;
    
    // Set command list and FIS base addresses
    port->regs[AHCI_PORT_CLB / 4] = port->clb_phys;
    port->regs[AHCI_PORT_CLBU / 4] = 0;  // Upper 32 bits for 64-bit systems
    port->regs[AHCI_PORT_FB / 4] = port->fb_phys;
    port->regs[AHCI_PORT_FBU / 4] = 0;
    
    // Clear interrupt status
    port->regs[AHCI_PORT_IS / 4] = 0xFFFFFFFF;
    
    // Enable FIS receive
    port->regs[AHCI_PORT_CMD / 4] |= AHCI_PORT_CMD_FRE;
    
    // Start command processing
    ahci_port_start(port);
    
    return 0;
}

int ahci_port_start(ahci_port_t* port) {
    if (!port) return -1;
    
    // Wait until CR (Command List Running) is cleared
    uint32_t timeout = 1000000;
    while ((port->regs[AHCI_PORT_CMD / 4] & AHCI_PORT_CMD_CR) && timeout--) {
        // Wait
    }
    
    if (timeout == 0) return -1;
    
    // Set ST (Start) bit
    port->regs[AHCI_PORT_CMD / 4] |= AHCI_PORT_CMD_ST;
    
    return 0;
}

int ahci_port_stop(ahci_port_t* port) {
    if (!port) return -1;
    
    // Clear ST (Start) bit
    port->regs[AHCI_PORT_CMD / 4] &= ~AHCI_PORT_CMD_ST;
    
    // Wait until CR (Command List Running) is cleared
    uint32_t timeout = 1000000;
    while ((port->regs[AHCI_PORT_CMD / 4] & AHCI_PORT_CMD_CR) && timeout--) {
        // Wait
    }
    
    if (timeout == 0) return -1;
    
    // Clear FRE (FIS Receive Enable)
    port->regs[AHCI_PORT_CMD / 4] &= ~AHCI_PORT_CMD_FRE;
    
    // Wait until FR (FIS Receive Running) is cleared
    timeout = 1000000;
    while ((port->regs[AHCI_PORT_CMD / 4] & AHCI_PORT_CMD_FR) && timeout--) {
        // Wait
    }
    
    return (timeout == 0) ? -1 : 0;
}

/**
 * Find a free command slot
 */
static int ahci_find_cmdslot(ahci_port_t* port) {
    // Find a free command slot
    // Check the command issue register (CI) and SATA active register (SACT)
    uint32_t slots = port->regs[AHCI_PORT_CI / 4] | port->regs[AHCI_PORT_SACT / 4];
    
    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0) {
            return i;
        }
    }
    
    return -1;  // No free slot
}

/**
 * Wait for command completion
 */
static int ahci_wait_complete(ahci_port_t* port, int slot) {
    uint32_t timeout = 1000000;
    
    // Wait for the command to complete
    while (timeout--) {
        // Check if command is no longer running
        if ((port->regs[AHCI_PORT_CI / 4] & (1 << slot)) == 0) {
            break;
        }
        
        // Check for errors
        uint32_t is = port->regs[AHCI_PORT_IS / 4];
        if (is & (1 << 30)) {  // Task file error
            return -1;
        }
    }
    
    if (timeout == 0) {
        return -1;  // Timeout
    }
    
    // Clear interrupt status
    port->regs[AHCI_PORT_IS / 4] = 0xFFFFFFFF;
    
    return 0;
}

int ahci_read_sectors(ahci_port_t* port, uint64_t start_lba, uint32_t count, void* buffer) {
    if (!port || !buffer || count == 0) return -1;
    
    // Find a free command slot
    int slot = ahci_find_cmdslot(port);
    if (slot == -1) return -1;
    
    // Get command header
    ahci_cmd_header_t* cmdheader = (ahci_cmd_header_t*)port->clb;
    cmdheader += slot;
    
    // Setup command header
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);  // Command FIS size
    cmdheader->w = 0;  // Read from device
    cmdheader->prdtl = 1;  // One PRDT entry
    
    // Allocate command table
    ahci_cmd_tbl_t* cmdtbl = (ahci_cmd_tbl_t*)kmalloc(sizeof(ahci_cmd_tbl_t) + sizeof(ahci_prdt_entry_t));
    if (!cmdtbl) return -1;
    
    // Clear command table
    for (uint32_t i = 0; i < sizeof(ahci_cmd_tbl_t) + sizeof(ahci_prdt_entry_t); i++) {
        ((uint8_t*)cmdtbl)[i] = 0;
    }
    
    // Set command table base address
    cmdheader->ctba = (uint32_t)(uintptr_t)cmdtbl;
    cmdheader->ctbau = 0;
    
    // Setup PRDT
    cmdtbl->prdt_entry[0].dba = (uint32_t)(uintptr_t)buffer;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1;  // Byte count (0-based)
    cmdtbl->prdt_entry[0].i = 1;  // Interrupt on completion
    
    // Setup command FIS
    fis_reg_h2d_t* cmdfis = (fis_reg_h2d_t*)cmdtbl->cfis;
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_READ_DMA_EXT;
    
    // Setup LBA
    cmdfis->lba0 = (uint8_t)(start_lba & 0xFF);
    cmdfis->lba1 = (uint8_t)((start_lba >> 8) & 0xFF);
    cmdfis->lba2 = (uint8_t)((start_lba >> 16) & 0xFF);
    cmdfis->lba3 = (uint8_t)((start_lba >> 24) & 0xFF);
    cmdfis->lba4 = (uint8_t)((start_lba >> 32) & 0xFF);
    cmdfis->lba5 = (uint8_t)((start_lba >> 40) & 0xFF);
    
    cmdfis->device = (1 << 6);  // LBA mode
    
    // Setup count
    cmdfis->countl = (uint8_t)(count & 0xFF);
    cmdfis->counth = (uint8_t)((count >> 8) & 0xFF);
    
    // Issue command
    port->regs[AHCI_PORT_CI / 4] = (1 << slot);
    
    // Wait for completion
    int result = ahci_wait_complete(port, slot);
    
    // Free command table
    kfree(cmdtbl);
    
    return result;
}

int ahci_write_sectors(ahci_port_t* port, uint64_t start_lba, uint32_t count, const void* buffer) {
    if (!port || !buffer || count == 0) return -1;
    
    // Find a free command slot
    int slot = ahci_find_cmdslot(port);
    if (slot == -1) return -1;
    
    // Get command header
    ahci_cmd_header_t* cmdheader = (ahci_cmd_header_t*)port->clb;
    cmdheader += slot;
    
    // Setup command header
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);  // Command FIS size
    cmdheader->w = 1;  // Write to device
    cmdheader->prdtl = 1;  // One PRDT entry
    
    // Allocate command table
    ahci_cmd_tbl_t* cmdtbl = (ahci_cmd_tbl_t*)kmalloc(sizeof(ahci_cmd_tbl_t) + sizeof(ahci_prdt_entry_t));
    if (!cmdtbl) return -1;
    
    // Clear command table
    for (uint32_t i = 0; i < sizeof(ahci_cmd_tbl_t) + sizeof(ahci_prdt_entry_t); i++) {
        ((uint8_t*)cmdtbl)[i] = 0;
    }
    
    // Set command table base address
    cmdheader->ctba = (uint32_t)(uintptr_t)cmdtbl;
    cmdheader->ctbau = 0;
    
    // Setup PRDT
    cmdtbl->prdt_entry[0].dba = (uint32_t)(uintptr_t)buffer;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = (count * 512) - 1;  // Byte count (0-based)
    cmdtbl->prdt_entry[0].i = 1;  // Interrupt on completion
    
    // Setup command FIS
    fis_reg_h2d_t* cmdfis = (fis_reg_h2d_t*)cmdtbl->cfis;
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_WRITE_DMA_EXT;
    
    // Setup LBA
    cmdfis->lba0 = (uint8_t)(start_lba & 0xFF);
    cmdfis->lba1 = (uint8_t)((start_lba >> 8) & 0xFF);
    cmdfis->lba2 = (uint8_t)((start_lba >> 16) & 0xFF);
    cmdfis->lba3 = (uint8_t)((start_lba >> 24) & 0xFF);
    cmdfis->lba4 = (uint8_t)((start_lba >> 32) & 0xFF);
    cmdfis->lba5 = (uint8_t)((start_lba >> 40) & 0xFF);
    
    cmdfis->device = (1 << 6);  // LBA mode
    
    // Setup count
    cmdfis->countl = (uint8_t)(count & 0xFF);
    cmdfis->counth = (uint8_t)((count >> 8) & 0xFF);
    
    // Issue command
    port->regs[AHCI_PORT_CI / 4] = (1 << slot);
    
    // Wait for completion
    int result = ahci_wait_complete(port, slot);
    
    // Free command table
    kfree(cmdtbl);
    
    return result;
}

int ahci_identify(ahci_port_t* port, void* buffer) {
    if (!port || !buffer) return -1;
    
    // Find a free command slot
    int slot = ahci_find_cmdslot(port);
    if (slot == -1) return -1;
    
    // Get command header
    ahci_cmd_header_t* cmdheader = (ahci_cmd_header_t*)port->clb;
    cmdheader += slot;
    
    // Setup command header
    cmdheader->cfl = sizeof(fis_reg_h2d_t) / sizeof(uint32_t);  // Command FIS size
    cmdheader->w = 0;  // Read from device
    cmdheader->prdtl = 1;  // One PRDT entry
    
    // Allocate command table
    ahci_cmd_tbl_t* cmdtbl = (ahci_cmd_tbl_t*)kmalloc(sizeof(ahci_cmd_tbl_t) + sizeof(ahci_prdt_entry_t));
    if (!cmdtbl) return -1;
    
    // Clear command table
    for (uint32_t i = 0; i < sizeof(ahci_cmd_tbl_t) + sizeof(ahci_prdt_entry_t); i++) {
        ((uint8_t*)cmdtbl)[i] = 0;
    }
    
    // Set command table base address
    cmdheader->ctba = (uint32_t)(uintptr_t)cmdtbl;
    cmdheader->ctbau = 0;
    
    // Setup PRDT - IDENTIFY returns 512 bytes
    cmdtbl->prdt_entry[0].dba = (uint32_t)(uintptr_t)buffer;
    cmdtbl->prdt_entry[0].dbau = 0;
    cmdtbl->prdt_entry[0].dbc = 511;  // 512 bytes (0-based)
    cmdtbl->prdt_entry[0].i = 1;  // Interrupt on completion
    
    // Setup command FIS
    fis_reg_h2d_t* cmdfis = (fis_reg_h2d_t*)cmdtbl->cfis;
    cmdfis->fis_type = FIS_TYPE_REG_H2D;
    cmdfis->c = 1;  // Command
    cmdfis->command = ATA_CMD_IDENTIFY;
    
    cmdfis->device = 0;  // Device register
    
    // Issue command
    port->regs[AHCI_PORT_CI / 4] = (1 << slot);
    
    // Wait for completion
    int result = ahci_wait_complete(port, slot);
    
    // Free command table
    kfree(cmdtbl);
    
    return result;
}

const char* ahci_get_device_type_name(ahci_device_type_t type) {
    switch (type) {
        case AHCI_DEV_SATA: return "SATA Drive";
        case AHCI_DEV_SATAPI: return "SATAPI Drive";
        case AHCI_DEV_SEMB: return "Enclosure Management Bridge";
        case AHCI_DEV_PM: return "Port Multiplier";
        default: return "Unknown";
    }
}
