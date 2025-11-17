/**
 * Aurora OS - NVMe Driver Implementation
 */

#include "nvme.h"
#include "pci.h"
#include "../memory/memory.h"
#include <stddef.h>

#define MAX_NVME_CONTROLLERS 4

// NVMe controllers
static nvme_controller_t controllers[MAX_NVME_CONTROLLERS];
static int controller_count = 0;

// Helper function to read 64-bit register
static uint64_t nvme_read64(volatile uint8_t* base, uint32_t offset) {
    volatile uint32_t* ptr = (volatile uint32_t*)(base + offset);
    uint32_t low = ptr[0];
    uint32_t high = ptr[1];
    return ((uint64_t)high << 32) | low;
}

// Helper function to write 64-bit register
static void nvme_write64(volatile uint8_t* base, uint32_t offset, uint64_t value) {
    volatile uint32_t* ptr = (volatile uint32_t*)(base + offset);
    ptr[0] = (uint32_t)(value & 0xFFFFFFFF);
    ptr[1] = (uint32_t)(value >> 32);
}

// Helper function to read 32-bit register
static uint32_t nvme_read32(volatile uint8_t* base, uint32_t offset) {
    return *(volatile uint32_t*)(base + offset);
}

// Helper function to write 32-bit register
static void nvme_write32(volatile uint8_t* base, uint32_t offset, uint32_t value) {
    *(volatile uint32_t*)(base + offset) = value;
}

int nvme_init(void) {
    controller_count = 0;
    return 0;
}

int nvme_detect_controllers(void) {
    // Find NVMe controllers via PCI
    pci_device_t pci_devices[16];
    int count = pci_find_devices_by_class(PCI_CLASS_STORAGE, pci_devices, 16);
    
    for (int i = 0; i < count && controller_count < MAX_NVME_CONTROLLERS; i++) {
        // Check if it's an NVMe controller
        if (pci_devices[i].subclass == PCI_SUBCLASS_STORAGE_NVME) {
            nvme_controller_t* ctrl = &controllers[controller_count];
            ctrl->pci_device = pci_devices[i];
            
            // Get BAR0 (controller registers)
            uint32_t bar0 = pci_devices[i].bar[0];
            if ((bar0 & 0x1) == 0) {  // Memory-mapped
                ctrl->bar0 = (volatile uint8_t*)(bar0 & ~0xF);
                
                // Initialize controller
                if (nvme_controller_init(ctrl) == 0) {
                    controller_count++;
                }
            }
        }
    }
    
    return controller_count;
}

nvme_controller_t* nvme_get_controller(int index) {
    if (index < 0 || index >= controller_count) {
        return NULL;
    }
    return &controllers[index];
}

int nvme_controller_init(nvme_controller_t* ctrl) {
    if (!ctrl || !ctrl->bar0) return -1;
    
    // Read controller capabilities
    uint64_t cap = nvme_read64(ctrl->bar0, NVME_REG_CAP);
    (void)cap;  // Capabilities would be parsed here
    
    // Disable controller first
    uint32_t cc = nvme_read32(ctrl->bar0, NVME_REG_CC);
    cc &= ~NVME_CC_EN;
    nvme_write32(ctrl->bar0, NVME_REG_CC, cc);
    
    // Wait for controller to be disabled
    uint32_t timeout = 1000000;
    while ((nvme_read32(ctrl->bar0, NVME_REG_CSTS) & NVME_CSTS_RDY) && timeout--) {
        // Wait
    }
    if (timeout == 0) return -1;
    
    // Create admin queue pair
    if (nvme_create_queue_pair(ctrl, &ctrl->admin_queue, 0, NVME_ADMIN_QUEUE_SIZE) != 0) {
        return -1;
    }
    
    // Set admin queue addresses
    nvme_write64(ctrl->bar0, NVME_REG_ASQ, ctrl->admin_queue.sq_phys);
    nvme_write64(ctrl->bar0, NVME_REG_ACQ, ctrl->admin_queue.cq_phys);
    
    // Set admin queue size
    uint32_t aqa = ((NVME_ADMIN_QUEUE_SIZE - 1) << 16) | (NVME_ADMIN_QUEUE_SIZE - 1);
    nvme_write32(ctrl->bar0, NVME_REG_AQA, aqa);
    
    // Configure and enable controller
    cc = NVME_CC_EN | NVME_CC_CSS_NVM | NVME_CC_AMS_RR | 
         NVME_CC_SHN_NONE | NVME_CC_IOSQES | NVME_CC_IOCQES |
         (0 << NVME_CC_MPS_SHIFT);  // 4KB page size
    nvme_write32(ctrl->bar0, NVME_REG_CC, cc);
    
    // Wait for controller to be ready
    timeout = 1000000;
    while (!(nvme_read32(ctrl->bar0, NVME_REG_CSTS) & NVME_CSTS_RDY) && timeout--) {
        // Wait
    }
    if (timeout == 0) return -1;
    
    // Initialize I/O queue pairs (create 1 for now)
    ctrl->io_queue_count = 0;
    if (nvme_create_queue_pair(ctrl, &ctrl->io_queues[0], 1, NVME_IO_QUEUE_SIZE) == 0) {
        ctrl->io_queue_count = 1;
    }
    
    return 0;
}

int nvme_create_queue_pair(nvme_controller_t* ctrl, nvme_queue_pair_t* qp,
                           uint16_t queue_id, uint16_t queue_size) {
    if (!ctrl || !qp) return -1;
    
    qp->queue_id = queue_id;
    qp->queue_size = queue_size;
    qp->sq_tail = 0;
    qp->cq_head = 0;
    qp->cq_phase = 1;
    
    // Allocate submission queue
    size_t sq_size = sizeof(nvme_sqe_t) * queue_size;
    qp->sq = (nvme_sqe_t*)kmalloc(sq_size);
    if (!qp->sq) return -1;
    qp->sq_phys = (uint32_t)(uintptr_t)qp->sq;  // Convert to physical in real OS
    
    // Allocate completion queue
    size_t cq_size = sizeof(nvme_cqe_t) * queue_size;
    qp->cq = (nvme_cqe_t*)kmalloc(cq_size);
    if (!qp->cq) {
        kfree(qp->sq);
        return -1;
    }
    qp->cq_phys = (uint32_t)(uintptr_t)qp->cq;
    
    // Calculate doorbell register offsets
    // Doorbell stride is in CAP register, assume 4 bytes for simplicity
    uint32_t doorbell_stride = 4;
    uint32_t doorbell_base = 0x1000;  // Standard doorbell register offset
    
    qp->sq_doorbell = (volatile uint32_t*)(ctrl->bar0 + doorbell_base + 
                                           (2 * queue_id * doorbell_stride));
    qp->cq_doorbell = (volatile uint32_t*)(ctrl->bar0 + doorbell_base + 
                                           (2 * queue_id + 1) * doorbell_stride);
    
    return 0;
}

int nvme_delete_queue_pair(nvme_controller_t* ctrl, nvme_queue_pair_t* qp) {
    if (!ctrl || !qp) return -1;
    
    // Free queue memory
    if (qp->sq) kfree(qp->sq);
    if (qp->cq) kfree(qp->cq);
    
    return 0;
}

int nvme_submit_command(nvme_queue_pair_t* qp, nvme_sqe_t* sqe) {
    if (!qp || !sqe) return -1;
    
    // Copy command to submission queue
    qp->sq[qp->sq_tail] = *sqe;
    
    // Advance tail pointer
    qp->sq_tail = (qp->sq_tail + 1) % qp->queue_size;
    
    // Ring doorbell
    *qp->sq_doorbell = qp->sq_tail;
    
    return 0;
}

int nvme_wait_completion(nvme_queue_pair_t* qp, nvme_cqe_t* cqe, uint32_t timeout) {
    if (!qp || !cqe) return -1;
    
    // Poll for completion
    while (timeout--) {
        nvme_cqe_t* entry = &qp->cq[qp->cq_head];
        
        // Check phase bit to see if entry is new
        uint8_t phase = (entry->status >> 0) & 1;
        if (phase == qp->cq_phase) {
            // Got completion
            *cqe = *entry;
            
            // Advance head pointer
            qp->cq_head = (qp->cq_head + 1) % qp->queue_size;
            if (qp->cq_head == 0) {
                qp->cq_phase = !qp->cq_phase;  // Flip phase
            }
            
            // Ring doorbell
            *qp->cq_doorbell = qp->cq_head;
            
            return 0;
        }
        
        // Small delay
        for (volatile int i = 0; i < 1000; i++);
    }
    
    return -1;  // Timeout
}

int nvme_read_sectors(nvme_controller_t* ctrl, uint32_t namespace_id,
                      uint64_t start_lba, uint32_t count, void* buffer) {
    if (!ctrl || !buffer || count == 0) return -1;
    
    // Build read command
    nvme_sqe_t sqe = {0};
    sqe.cdw0 = (NVME_CMD_READ << 0);  // Opcode
    sqe.nsid = namespace_id;
    sqe.prp1 = (uint64_t)(uintptr_t)buffer;  // Physical address of buffer
    sqe.cdw10 = (uint32_t)(start_lba & 0xFFFFFFFF);
    sqe.cdw11 = (uint32_t)(start_lba >> 32);
    sqe.cdw12 = (count - 1);  // Number of logical blocks (0-based)
    
    // Submit command to I/O queue
    if (ctrl->io_queue_count > 0) {
        nvme_queue_pair_t* qp = &ctrl->io_queues[0];
        nvme_submit_command(qp, &sqe);
        
        // Wait for completion
        nvme_cqe_t cqe;
        if (nvme_wait_completion(qp, &cqe, 1000000) == 0) {
            return 0;  // Success
        }
    }
    
    return -1;
}

int nvme_write_sectors(nvme_controller_t* ctrl, uint32_t namespace_id,
                       uint64_t start_lba, uint32_t count, const void* buffer) {
    if (!ctrl || !buffer || count == 0) return -1;
    
    // Build write command
    nvme_sqe_t sqe = {0};
    sqe.cdw0 = (NVME_CMD_WRITE << 0);
    sqe.nsid = namespace_id;
    sqe.prp1 = (uint64_t)(uintptr_t)buffer;
    sqe.cdw10 = (uint32_t)(start_lba & 0xFFFFFFFF);
    sqe.cdw11 = (uint32_t)(start_lba >> 32);
    sqe.cdw12 = (count - 1);
    
    // Submit and wait
    if (ctrl->io_queue_count > 0) {
        nvme_queue_pair_t* qp = &ctrl->io_queues[0];
        nvme_submit_command(qp, &sqe);
        
        nvme_cqe_t cqe;
        if (nvme_wait_completion(qp, &cqe, 1000000) == 0) {
            return 0;
        }
    }
    
    return -1;
}

int nvme_identify_controller(nvme_controller_t* ctrl, void* buffer) {
    if (!ctrl || !buffer) return -1;
    
    // Build identify command
    nvme_sqe_t sqe = {0};
    sqe.cdw0 = (NVME_ADMIN_IDENTIFY << 0);
    sqe.prp1 = (uint64_t)(uintptr_t)buffer;
    sqe.cdw10 = 1;  // CNS = 1 for controller identify
    
    // Submit to admin queue
    nvme_submit_command(&ctrl->admin_queue, &sqe);
    
    nvme_cqe_t cqe;
    if (nvme_wait_completion(&ctrl->admin_queue, &cqe, 1000000) == 0) {
        return 0;
    }
    
    return -1;
}

int nvme_identify_namespace(nvme_controller_t* ctrl, uint32_t namespace_id, void* buffer) {
    if (!ctrl || !buffer) return -1;
    
    nvme_sqe_t sqe = {0};
    sqe.cdw0 = (NVME_ADMIN_IDENTIFY << 0);
    sqe.nsid = namespace_id;
    sqe.prp1 = (uint64_t)(uintptr_t)buffer;
    sqe.cdw10 = 0;  // CNS = 0 for namespace identify
    
    nvme_submit_command(&ctrl->admin_queue, &sqe);
    
    nvme_cqe_t cqe;
    if (nvme_wait_completion(&ctrl->admin_queue, &cqe, 1000000) == 0) {
        return 0;
    }
    
    return -1;
}
