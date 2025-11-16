/**
 * Aurora OS - NVMe Driver
 * 
 * Non-Volatile Memory Express (NVMe) driver for high-performance SSDs
 */

#ifndef NVME_H
#define NVME_H

#include <stdint.h>
#include "pci.h"

// NVMe Controller Registers (offset from BAR0)
#define NVME_REG_CAP        0x00  // Controller Capabilities
#define NVME_REG_VS         0x08  // Version
#define NVME_REG_INTMS      0x0C  // Interrupt Mask Set
#define NVME_REG_INTMC      0x10  // Interrupt Mask Clear
#define NVME_REG_CC         0x14  // Controller Configuration
#define NVME_REG_CSTS       0x1C  // Controller Status
#define NVME_REG_AQA        0x24  // Admin Queue Attributes
#define NVME_REG_ASQ        0x28  // Admin Submission Queue Base Address
#define NVME_REG_ACQ        0x30  // Admin Completion Queue Base Address

// NVMe Controller Configuration bits
#define NVME_CC_EN          (1 << 0)   // Enable
#define NVME_CC_CSS_NVM     (0 << 4)   // NVM Command Set
#define NVME_CC_MPS_SHIFT   7          // Memory Page Size
#define NVME_CC_AMS_RR      (0 << 11)  // Round Robin arbitration
#define NVME_CC_SHN_NONE    (0 << 14)  // No shutdown notification
#define NVME_CC_IOSQES      (6 << 16)  // I/O Submission Queue Entry Size (2^6 = 64 bytes)
#define NVME_CC_IOCQES      (4 << 20)  // I/O Completion Queue Entry Size (2^4 = 16 bytes)

// NVMe Controller Status bits
#define NVME_CSTS_RDY       (1 << 0)   // Ready
#define NVME_CSTS_CFS       (1 << 1)   // Controller Fatal Status
#define NVME_CSTS_SHST_MASK (3 << 2)   // Shutdown Status

// NVMe Admin Commands
#define NVME_ADMIN_DELETE_SQ    0x00  // Delete I/O Submission Queue
#define NVME_ADMIN_CREATE_SQ    0x01  // Create I/O Submission Queue
#define NVME_ADMIN_DELETE_CQ    0x04  // Delete I/O Completion Queue
#define NVME_ADMIN_CREATE_CQ    0x05  // Create I/O Completion Queue
#define NVME_ADMIN_IDENTIFY     0x06  // Identify
#define NVME_ADMIN_SET_FEATURES 0x09  // Set Features
#define NVME_ADMIN_GET_FEATURES 0x0A  // Get Features

// NVMe I/O Commands
#define NVME_CMD_FLUSH          0x00  // Flush
#define NVME_CMD_WRITE          0x01  // Write
#define NVME_CMD_READ           0x02  // Read

// Queue sizes
#define NVME_ADMIN_QUEUE_SIZE   64
#define NVME_IO_QUEUE_SIZE      256

// NVMe Submission Queue Entry (SQE)
typedef struct {
    uint32_t cdw0;      // Command Dword 0 (opcode, flags, etc.)
    uint32_t nsid;      // Namespace ID
    uint64_t reserved;
    uint64_t mptr;      // Metadata Pointer
    uint64_t prp1;      // PRP Entry 1
    uint64_t prp2;      // PRP Entry 2
    uint32_t cdw10;     // Command Dword 10
    uint32_t cdw11;     // Command Dword 11
    uint32_t cdw12;     // Command Dword 12
    uint32_t cdw13;     // Command Dword 13
    uint32_t cdw14;     // Command Dword 14
    uint32_t cdw15;     // Command Dword 15
} nvme_sqe_t;

// NVMe Completion Queue Entry (CQE)
typedef struct {
    uint32_t result;    // Command-specific result
    uint32_t reserved;
    uint16_t sq_head;   // Submission Queue Head Pointer
    uint16_t sq_id;     // Submission Queue Identifier
    uint16_t cid;       // Command Identifier
    uint16_t status;    // Status Field
} nvme_cqe_t;

// NVMe Queue Pair structure
typedef struct {
    uint16_t queue_id;
    uint16_t queue_size;
    nvme_sqe_t* sq;          // Submission Queue
    nvme_cqe_t* cq;          // Completion Queue
    uint32_t sq_phys;        // Submission Queue physical address
    uint32_t cq_phys;        // Completion Queue physical address
    volatile uint32_t* sq_doorbell;  // Submission Queue doorbell register
    volatile uint32_t* cq_doorbell;  // Completion Queue doorbell register
    uint16_t sq_tail;        // Submission Queue tail pointer
    uint16_t cq_head;        // Completion Queue head pointer
    uint8_t cq_phase;        // Completion Queue phase bit
} nvme_queue_pair_t;

// NVMe Controller structure
typedef struct {
    pci_device_t pci_device;
    volatile uint8_t* bar0;      // Controller registers (memory-mapped)
    uint32_t num_namespaces;
    uint32_t max_transfer_size;
    
    // Admin queue pair
    nvme_queue_pair_t admin_queue;
    
    // I/O queue pairs
    nvme_queue_pair_t io_queues[16];
    int io_queue_count;
} nvme_controller_t;

/**
 * Initialize NVMe subsystem
 * @return 0 on success, -1 on failure
 */
int nvme_init(void);

/**
 * Detect and initialize NVMe controllers
 * @return Number of controllers found
 */
int nvme_detect_controllers(void);

/**
 * Get NVMe controller by index
 * @param index Controller index
 * @return Pointer to controller or NULL
 */
nvme_controller_t* nvme_get_controller(int index);

/**
 * Initialize NVMe controller
 * @param ctrl Controller to initialize
 * @return 0 on success, -1 on failure
 */
int nvme_controller_init(nvme_controller_t* ctrl);

/**
 * Create queue pair (submission + completion queues)
 * @param ctrl Controller
 * @param qp Queue pair structure to initialize
 * @param queue_id Queue ID (0 for admin, 1+ for I/O)
 * @param queue_size Number of entries in queue
 * @return 0 on success, -1 on failure
 */
int nvme_create_queue_pair(nvme_controller_t* ctrl, nvme_queue_pair_t* qp,
                           uint16_t queue_id, uint16_t queue_size);

/**
 * Delete queue pair
 * @param ctrl Controller
 * @param qp Queue pair to delete
 * @return 0 on success, -1 on failure
 */
int nvme_delete_queue_pair(nvme_controller_t* ctrl, nvme_queue_pair_t* qp);

/**
 * Submit command to queue
 * @param qp Queue pair
 * @param sqe Submission queue entry
 * @return 0 on success, -1 on failure
 */
int nvme_submit_command(nvme_queue_pair_t* qp, nvme_sqe_t* sqe);

/**
 * Wait for command completion
 * @param qp Queue pair
 * @param cqe Completion queue entry output
 * @param timeout Timeout in microseconds
 * @return 0 on success, -1 on failure/timeout
 */
int nvme_wait_completion(nvme_queue_pair_t* qp, nvme_cqe_t* cqe, uint32_t timeout);

/**
 * Read sectors from NVMe device
 * @param ctrl Controller
 * @param namespace_id Namespace ID
 * @param start_lba Starting LBA
 * @param count Number of sectors to read
 * @param buffer Buffer to read into
 * @return 0 on success, -1 on failure
 */
int nvme_read_sectors(nvme_controller_t* ctrl, uint32_t namespace_id,
                      uint64_t start_lba, uint32_t count, void* buffer);

/**
 * Write sectors to NVMe device
 * @param ctrl Controller
 * @param namespace_id Namespace ID
 * @param start_lba Starting LBA
 * @param count Number of sectors to write
 * @param buffer Buffer to write from
 * @return 0 on success, -1 on failure
 */
int nvme_write_sectors(nvme_controller_t* ctrl, uint32_t namespace_id,
                       uint64_t start_lba, uint32_t count, const void* buffer);

/**
 * Identify controller
 * @param ctrl Controller
 * @param buffer Buffer to store identify data (4KB)
 * @return 0 on success, -1 on failure
 */
int nvme_identify_controller(nvme_controller_t* ctrl, void* buffer);

/**
 * Identify namespace
 * @param ctrl Controller
 * @param namespace_id Namespace ID
 * @param buffer Buffer to store identify data (4KB)
 * @return 0 on success, -1 on failure
 */
int nvme_identify_namespace(nvme_controller_t* ctrl, uint32_t namespace_id, void* buffer);

#endif // NVME_H
