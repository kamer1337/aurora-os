/**
 * @file linux_vm.h
 * @brief Linux Kernel Virtual Machine Support for Aurora OS
 * 
 * Provides Linux kernel compatibility layer and VM support
 * allowing Aurora OS to run Linux workloads in dedicated workspaces
 */

#ifndef LINUX_VM_H
#define LINUX_VM_H

#include <stdint.h>
#include <stdbool.h>
#include "aurora_vm.h"

/* Linux VM configuration */
#define LINUX_VM_MEMORY_SIZE    (128 * 1024 * 1024)   /* 128MB for Linux */
#define LINUX_VM_KERNEL_BASE    0x100000              /* 1MB kernel load address */
#define LINUX_VM_MAX_PROCESSES  64                    /* Max Linux processes */

/* Linux VM state */
typedef enum {
    LINUX_VM_STATE_UNINITIALIZED = 0,
    LINUX_VM_STATE_INITIALIZED,
    LINUX_VM_STATE_RUNNING,
    LINUX_VM_STATE_PAUSED,
    LINUX_VM_STATE_STOPPED,
    LINUX_VM_STATE_ERROR
} linux_vm_state_t;

/* Linux VM instance */
typedef struct {
    AuroraVM* aurora_vm;              /* Underlying Aurora VM */
    linux_vm_state_t state;           /* Current state */
    uint8_t* kernel_image;            /* Loaded kernel image */
    uint32_t kernel_size;             /* Kernel image size */
    uint32_t kernel_entry;            /* Kernel entry point */
    char kernel_cmdline[256];         /* Kernel command line */
    bool has_initrd;                  /* Has initial ramdisk */
    uint32_t initrd_addr;             /* Initrd load address */
    uint32_t initrd_size;             /* Initrd size */
} LinuxVM;

/* Linux syscall emulation */
typedef enum {
    LINUX_SYSCALL_EXIT = 1,
    LINUX_SYSCALL_FORK = 2,
    LINUX_SYSCALL_READ = 3,
    LINUX_SYSCALL_WRITE = 4,
    LINUX_SYSCALL_OPEN = 5,
    LINUX_SYSCALL_CLOSE = 6,
    LINUX_SYSCALL_GETPID = 20,
    LINUX_SYSCALL_BRK = 45,
    LINUX_SYSCALL_MMAP = 90,
    LINUX_SYSCALL_MUNMAP = 91,
    /* Add more as needed */
} linux_syscall_t;

/**
 * Initialize Linux VM subsystem
 * @return 0 on success, -1 on failure
 */
int linux_vm_init(void);

/**
 * Create a new Linux VM instance
 * @return Pointer to Linux VM or NULL on failure
 */
LinuxVM* linux_vm_create(void);

/**
 * Destroy Linux VM instance
 * @param vm Linux VM instance
 */
void linux_vm_destroy(LinuxVM* vm);

/**
 * Load Linux kernel image
 * @param vm Linux VM instance
 * @param kernel_data Kernel image data
 * @param size Kernel image size
 * @return 0 on success, -1 on failure
 */
int linux_vm_load_kernel(LinuxVM* vm, const uint8_t* kernel_data, uint32_t size);

/**
 * Load initrd (initial ramdisk)
 * @param vm Linux VM instance
 * @param initrd_data Initrd data
 * @param size Initrd size
 * @return 0 on success, -1 on failure
 */
int linux_vm_load_initrd(LinuxVM* vm, const uint8_t* initrd_data, uint32_t size);

/**
 * Set kernel command line
 * @param vm Linux VM instance
 * @param cmdline Command line string
 * @return 0 on success, -1 on failure
 */
int linux_vm_set_cmdline(LinuxVM* vm, const char* cmdline);

/**
 * Start Linux VM execution
 * @param vm Linux VM instance
 * @return 0 on success, -1 on failure
 */
int linux_vm_start(LinuxVM* vm);

/**
 * Pause Linux VM execution
 * @param vm Linux VM instance
 * @return 0 on success, -1 on failure
 */
int linux_vm_pause(LinuxVM* vm);

/**
 * Resume Linux VM execution
 * @param vm Linux VM instance
 * @return 0 on success, -1 on failure
 */
int linux_vm_resume(LinuxVM* vm);

/**
 * Stop Linux VM execution
 * @param vm Linux VM instance
 * @return 0 on success, -1 on failure
 */
int linux_vm_stop(LinuxVM* vm);

/**
 * Get Linux VM state
 * @param vm Linux VM instance
 * @return Current state
 */
linux_vm_state_t linux_vm_get_state(LinuxVM* vm);

/**
 * Handle Linux syscall
 * @param vm Linux VM instance
 * @param syscall_num Syscall number
 * @param args Syscall arguments
 * @return Syscall return value
 */
int32_t linux_vm_handle_syscall(LinuxVM* vm, uint32_t syscall_num, uint32_t* args);

/**
 * Check if Linux VM is available
 * @return true if Linux VM support is enabled
 */
bool linux_vm_is_available(void);

/**
 * Get Linux VM version string
 * @return Version string
 */
const char* linux_vm_get_version(void);

#endif /* LINUX_VM_H */
