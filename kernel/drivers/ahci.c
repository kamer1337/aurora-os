/**
 * Aurora OS - SATA AHCI Controller Driver Implementation
 */

#include "ahci.h"
#include "pci.h"
#include "../memory/memory.h"
#include <stddef.h>

#define MAX_AHCI_CONTROLLERS 4

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

int ahci_read_sectors(ahci_port_t* port, uint64_t start_lba, uint32_t count, void* buffer) {
    if (!port || !buffer || count == 0) return -1;
    
    // This is a placeholder for the full AHCI read implementation
    // A complete implementation would:
    // 1. Build command table with READ DMA EXT command
    // 2. Build PRDT (Physical Region Descriptor Table) for buffer
    // 3. Issue command to hardware
    // 4. Wait for completion
    // 5. Check for errors
    
    (void)start_lba;  // Suppress unused parameter warning
    
    return 0;  // Success (placeholder)
}

int ahci_write_sectors(ahci_port_t* port, uint64_t start_lba, uint32_t count, const void* buffer) {
    if (!port || !buffer || count == 0) return -1;
    
    // This is a placeholder for the full AHCI write implementation
    // Similar to read, but uses WRITE DMA EXT command
    
    (void)start_lba;  // Suppress unused parameter warning
    
    return 0;  // Success (placeholder)
}

int ahci_identify(ahci_port_t* port, void* buffer) {
    if (!port || !buffer) return -1;
    
    // This is a placeholder for the IDENTIFY DEVICE command implementation
    // Would build and issue IDENTIFY DEVICE command to get drive info
    
    return 0;  // Success (placeholder)
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
